/* tinytest.h -- Copyright 2009 Nick Mathewson

   Anyone may use, reproduce, or modify this work in any way so long as the
   above copyright notice and this license statement are retained.  There is
   no warranty.
*/

#ifndef _TINYTEST_H
#define _TINYTEST_H

#define TT_FORK  (1<<0)
#define TT_SKIP  (1<<1)
#define _TT_ENABLED  (1<<2)
#define TT_FIRST_USER_FLAG (1<<3)

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

struct testgroup_t {
	const char *prefix;
	struct testcase_t *cases;
};

void _tinytest_set_test_failed(void);
int _tinytest_get_verbosity(void);
int _tinytest_set_flag(struct testgroup_t *groups, const char *arg, unsigned long flag);

#define tinytest_skip(groups, named) \
	_tinytest_set_flag(groups, named, TT_SKIP)

int testcase_run_testcase(const struct testcase_t *);
int tinytest_main(int argc, const char **argv, struct testgroup_t *groups);

#endif
