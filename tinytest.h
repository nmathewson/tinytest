/* tinytest.h -- Copyright 2009 Nick Mathewson

   Anyone may use, reproduce, or modify this work in any way so long as the
   above copyright notice and this license statement are retained.  There is
   no warranty.
*/

#ifndef _TINYTEST_H
#define _TINYTEST_H

#define TT_FORK  1

typedef void (*testcase_fn)(void *);

struct testcase_t;

struct testcase_setup_t {
	void *(*setup_fn)(const struct testcase_t *);
	int (*cleanup_fn)(void *);
};

struct testcase_t {
	const char *name;
	testcase_fn fn;
	unsigned long flags;
	const struct testcase_setup_t *setup;
	void *setup_data;
};

void _tinytest_set_test_failed(void);
int _tinytest_get_verbosity(void);

#if 0
struct testgroup_t;
struct testgroup_t *tinytest_group_new(const char *name);
void tinytest_group_add(struct testgroup_t *, struct testgroup_t *);
void tinytest_group_add_cases(struct testgroup_t *, struct testcase_t *);
void tinytest_group_free(struct testgroup_t *);
#endif

int testcase_run_testcase(const struct testcase_t *);
int tinytest_main(int argc, const char **argv, struct testcase_t *testcase);


#endif
