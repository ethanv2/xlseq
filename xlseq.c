/* See LICENSE for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <inttypes.h>
#include <errno.h>
#include <wchar.h>

#include "xlseq.h"
#include "util.h"
#include "arg.h"

#define TRY_MATCH(TYPE, FUNC) if (match[TYPE-1]) { match[TYPE-1] = FUNC; }
#define MUST_BOUNDED(N) if (N <= 0) {fprintf(stderr, "%s: bounded sequence: need count (-n flag)\n", argv0); return 1;}

typedef enum {
	/* unbounded ranges */
	StringPattern = 1,	/* common prefix of arbitrary type */
	NumberPattern,		/* common format of double */
	DatePattern,		/* common format of struct tm * */

	/* bounded ranges */
	DaysPattern,
	MonthsPattern,
	ColorsPattern,
	AlphabetPattern,

	UnrecognisedPattern	/* should never happen;
				   used to terminate the buffer */
} PatternType;

struct matcher_state {
	struct alpha_matcher_state alphabet;

	struct buffered_matcher_state days;
	struct buffered_matcher_state months;
	struct buffered_matcher_state colors;
};

char *argv0;

void
usage()
{
	fprintf(stderr, "%s: [-cn count] -u\n", argv0);
	fprintf(stderr, "-c:\tcontinue pattern for n iterations (default: unbounded)\n");
	fprintf(stderr, "-n:\tshow n results in total (default: unbounded)\n");
	fprintf(stderr, "-u:\tthis message\n");
	fprintf(stderr, "-h:\tthis message\n");
	exit(1);
}

PatternType *
_type_detect(const char *text, int ind)
{
	struct matcher_state state;
	int match[UnrecognisedPattern];
	PatternType *buf;
	int used = 0;
	const char *ptr = text;
	wchar_t rune;
	int count, len = strlen(text);

	memset(match, 1, sizeof(match));
	memset(&state, 0, sizeof(state));
	do {
		count = mbtowc(&rune, ptr, len-(ptr-text));
		if (count < 0) {
			fprintf(stderr, "%s: invalid text encoding\n", argv0);
			exit(1);
		}
		ptr += count;

		TRY_MATCH(StringPattern, string_pattern_match());
		TRY_MATCH(NumberPattern, number_pattern_match(rune));
		TRY_MATCH(DaysPattern, buffered_pattern_match(rune, &state.days, days, LENGTH(days)));
		TRY_MATCH(MonthsPattern, buffered_pattern_match(rune, &state.months, months, LENGTH(months)));
		TRY_MATCH(AlphabetPattern, alphabet_pattern_match(rune, &state.alphabet, ind));
		TRY_MATCH(ColorsPattern, buffered_pattern_match(rune, &state.colors, colors, LENGTH(colors)));
	} while (*ptr);
	TRY_MATCH(DatePattern, date_pattern_match(text));

	buf = malloc(sizeof(match));
	for (int i = 0; i < UnrecognisedPattern; i++) {
		if (match[i]) {
			buf[used++] = i+1;
		}
	}

	return buf;
}

PatternType
type_detect(const char *first, const char *second)
{
	PatternType *poss, *tmp;
	PatternType *poss1, *poss2;
	PatternType max = 0;
	poss1 = _type_detect(first, 0);
	poss2 = _type_detect(second, 1);

	/* assumes both buffers are same length */
	for (poss = poss1; *poss != UnrecognisedPattern; poss++) {
		if (*poss > max) {
			for (tmp = poss2; *tmp != UnrecognisedPattern; tmp++) {
				if (*tmp == *poss) {
					max = *poss;
					break;
				}
			}
		}
	}
	free(poss1);
	free(poss2);

	if (!max) {
		fprintf(stderr, "%s: unrecognised pattern\n", argv0);
		exit(1);
	}

	return max;
}

int
run_pattern(PatternType pat, int count, union sample_space samples, struct full_sample full)
{
	switch (pat) {
	case StringPattern:
		MUST_BOUNDED(count);
		string_pattern_run(samples, count);
		break;
	case NumberPattern:
		MUST_BOUNDED(count);
		number_pattern_run(full, count);
		break;
	case DatePattern:
		MUST_BOUNDED(count);
		date_pattern_run(samples, count);
		break;
	case DaysPattern:
		buffered_pattern_run(samples, count, days, LENGTH(days));
		break;
	case MonthsPattern:
		buffered_pattern_run(samples, count, months, LENGTH(months));
		break;
	case AlphabetPattern:
		alphabet_pattern_run(samples, count);
		break;
	case ColorsPattern:
		buffered_pattern_run(samples, count, colors, LENGTH(colors));
		break;
	case UnrecognisedPattern:
		break;
	default:
		fprintf(stderr, "%s: unknown pattern type %d\n", argv0, pat);
		return 2;
	}

	return 0;
}

int
main(int argc, char **argv)
{
	struct full_sample full;
	union sample_space samples;
	PatternType type = UnrecognisedPattern;
	int i, success;
	int subcount = 0, count = -1;

	setlocale(LC_ALL, "");

	ARGBEGIN {
	case 'n':
		subcount = 1;
		/* FALLTHROUGH */
	case 'c':
		errno = 0;
		count = strtoimax(EARGF(usage()), NULL, 10);
		if (count == 0 || errno != 0)
			usage();
		break;
	case 'h':
	case 'u':
		usage();
		break;
	case 'v':
		printf("xlseq v%s\n", VERSION);
		return 1;
	default:
		fprintf(stderr, "%s: unknown flag '%c'\n", argv0, ARGC());
		return 1;
	} ARGEND;
	if (argc < 2) {
		fprintf(stderr, "%s: need pattern of at least two items\n", argv0);
		return 1;
	}

	if (type == UnrecognisedPattern)
		type = type_detect(argv[argc-2], argv[argc-1]);

	if (subcount) {
		count = count-argc;
		if (count <= 0) {
			fprintf(stderr, "%s: net count %d smaller than sequence sample\n", argv0, count);
			return 1;
		}
	}

	full = (struct full_sample){
		.samples = argv,
		.len = argc
	};
	samples = (union sample_space){
		.samples = {
			argv[argc-1],
			argv[argc-2],
			(argc > 2) ? argv[argc-3] : NULL
		}
	};

	/* print sample set */
	for (i = 0; i < argc; i++) {
		printf("%s ", argv[i]);
	}
	success = run_pattern(type, count, samples, full);
	putchar('\n');
	return success;
}
