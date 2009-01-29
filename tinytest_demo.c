/* tinytest.c -- Copyright 2009 Nick Mathewson

   Anyone may use, reproduce, or modify this work in any way so long as the
   above copyright notice and this license statement are retained.  There is
   no warranty.
*/

#include "tinytest.h"
#include "tinytest_macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void testcase_add(void *data)
{
	tt_want(1==10);
	tt_int_eq(1+1, 2);
	tt_int_eq(1+1, 3);
 end:
	return;
}

void testcase_add2(void *data)
{
	tt_want(10==10);
	tt_int_eq(1+1, 2);
	tt_int_neq(1+1, 3);
	if (!malloc(-10))
		tt_fail_perror("malloc");
 end:
	return;
}


void testcase_other(void *data)
{
	tt_assert(10==10);
 end:
	return;
}

struct testcase_t tc[] = {
	{ "add", testcase_add, TT_FORK, NULL },
	{ "add2", testcase_add2, 0, NULL },
	{ "other", testcase_other, 0, NULL },
	{ NULL, NULL, 0 },
};

struct testgroup_t groups[] = {
	{ "x/", tc },
	{ NULL, NULL },
};

int main(int c, const char **v)
{
	tinytest_skip(groups, "x/other");
	tinytest_main(c, v, groups);
	return 0;
}
