/* See LICENSE for copyright and license details. */

struct long_short {
	const wchar_t *l, *s;
};
struct full_sample {
	char **samples;
	size_t len;
};
union sample_space {
	struct {
		const char *last;
		const char *middle;
		const char *first;	/* may be NULL if not enough samples */
	} ordered;
	const char *samples[3];
};
struct buffered_matcher_state {
	wchar_t buf[BUFSIZ];
	long bufpos;
};
int buffered_pattern_match(const wchar_t rune,
			   struct buffered_matcher_state *state,
			   const struct long_short *dataset,
			   size_t datalen);
void buffered_pattern_run(union sample_space samples, long count,
			  const struct long_short *dataset,
			  size_t datalen);

/* string pattern */
struct string_pattern_state {
	int common_check;
	const char *common_end;	/* NULL if no common section discovered */
};
int string_pattern_match();
void string_pattern_run(union sample_space samples, unsigned long count);

/* number pattern */
int number_pattern_match(const wchar_t rune);
void number_pattern_run(struct full_sample samples, unsigned long count);

/* date pattern */
static const char *datefmt[] = {
	"%T",
	"%H:%M",
	"%I:%M %p",
	"%r",
	"%F",
};
int date_pattern_match(const char *in);
void date_pattern_run(union sample_space samples, unsigned long count);

/* alphabet pattern */
struct alpha_matcher_state {
	int arg, ind;
};
int alphabet_pattern_match(const wchar_t rune,
			   struct alpha_matcher_state *state, int arg);
void alphabet_pattern_run(union sample_space samples, long count);

/* days pattern */
static const struct long_short days[] = {
	{L"monday", L"mon"},
	{L"tuesday", L"tue"},
	{L"wednesday", L"wed"},
	{L"thursday", L"thur"},
	{L"friday", L"fri"},
	{L"saturday", L"sat"},
	{L"sunday", L"sun"}
};

/* months pattern */
static const struct long_short months[] = {
	{L"january", L"jan"},
	{L"february", L"feb"},
	{L"march", L"mar"},
	{L"april", L"apr"},
	{L"may", L"may"},
	{L"june", L"jun"},
	{L"july", L"jul"},
	{L"august", L"aug"},
	{L"september", L"sept"},
	{L"october", L"oct"},
	{L"november", L"nov"},
	{L"december", L"dec"}
};

/* colors pattern */
static const struct long_short colors[] = {
	{L"red", L"r"},
	{L"orange", L"o"},
	{L"yellow", L"y"},
	{L"green", L"g"},
	{L"blue", L"b"},
	{L"indigo", L"i"},
	{L"violet", L"v"},
};
