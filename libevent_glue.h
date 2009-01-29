
void run_legacy_test_fn(void *ptr);
extern int pair[2];
extern int test_ok;
extern int called;

extern const struct testcase_setup_t legacy_setup;

#define LEGACY(name,flags)						\
	{ #name, run_legacy_test_fn, flags, &legacy_setup,		\
	  test_##name }

