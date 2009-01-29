/* tinytest.c -- Copyright 2009 Nick Mathewson

   Anyone may use, reproduce, or modify this work in any way so long as the
   above copyright notice and this license statement are retained.  There is
   no warranty.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "tinytest.h"
#include "tinytest_macros.h"

struct testgroup_t {
	const char *name;
	const struct testcase *testcases;
};

static int in_tinytest_main = 0;
static int n_ok = 0;
static int n_bad = 0;

static int opt_nofork = 0;
static int opt_exitcode = 0;
static int opt_verbosity = 1;

static int cur_test_outcome = 0;
const char *cur_test_name = NULL;

#ifdef WIN32
static const char *commandname = NULL;
#else
static int outcome_pipe[2] = { -1, -1 };
#endif

static int
_testcase_run_bare(const struct testcase_t *testcase)
{
	void *env = NULL;
	int outcome;
	if (testcase->setup) {
		env = testcase->setup->setup_fn(testcase);
		assert(env);
	}

	cur_test_outcome = 1;
	testcase->fn(env);
	outcome = cur_test_outcome;

	if (testcase->setup) {
		if (testcase->setup->cleanup_fn(env) == 0)
			outcome = 0;
	}

	return outcome;
}

static int
_testcase_run_forked(const struct testcase_t *testcase)
{
#ifdef WIN32
	int ok;
	char buffer[4096];
	PROCESS_INFORMATION info;
	DWORD exitcode;

	if (!in_tinytest_main) {
		printf("\nERROR.  On Windows, _testcase_run_forked must be"
		       " called from within tinytest_main.\n");
		abort();
	}
	printf("[forking] ");
	snprintf(buffer, sizeof(buffer), "%s --no-fork --exitcode %s"
		 commandname, testcase->name);

	ok = CreateProcess(commandname, buffer, NULL, NULL, 0,
			   0, NULL, NULL, NULL, &info);
	if (!ok) {
		printf("CreateProcess failed!\n");
		return 0;
	}
	WaitForSingleObject(info.hProcess, INFINITE);
	GetExitCodeProcess(info.hProcess, &exitcode);
	CloseHandle(info.hProcess);
	CloseHandle(info.hThread);
	return exitcode == 0;
#else
	pid_t pid;
	if (outcome_pipe[0] == -1) {
		if (pipe(outcome_pipe))
			perror("opening pipe");
	}

	printf("[forking] ");
	pid = fork();
	if (!pid) {
		/* child. */
		int test_r = _testcase_run_bare(testcase);
		int write_r = write(outcome_pipe[1], test_r ? "Y" : "N", 1);
		if (write_r != 1) {
			perror("write outcome to pipe");
			exit(1);
		}
		exit(0);
	} else {
		int status, r;
		char b[1];
		r = read(outcome_pipe[0], b, 1);
		if (r != 1)
			perror("read outcome from pipe");
		waitpid(pid, &status, 0);
		return b[0] == 'Y' ? 1 : 0;
	}
#endif
}

int
testcase_run(const struct testcase_t *testcase)
{
	int outcome;

	if (opt_verbosity)
		printf("%s... ", testcase->name);
	else
		cur_test_name = testcase->name;

	if ((testcase->flags & TT_FORK) && !opt_nofork) {
		outcome = _testcase_run_forked(testcase);
	} else {
		outcome  = _testcase_run_bare(testcase);
	}

	if (outcome) {
		++n_ok;
		if (opt_verbosity)
			puts("OK");
	} else {
		++n_bad;
		printf("\n  [%s FAILED]\n", testcase->name);
	}

	if (opt_exitcode) {
		exit(outcome ? 0 : 1);
	} else {
		return outcome;
	}
}

int
tinytest_main(int c, const char **v, struct testcase_t *cases)
{
	unsigned int *tc_enabled = NULL;
	int returnval = -1;
	int n_testcases, i, j, n_enabled=0;

	for (n_testcases=0; cases[n_testcases].name; ++n_testcases)
		;

	if (!(tc_enabled = calloc(n_testcases, sizeof(int)))) {
		perror("calloc");
		return -1;
	}

#ifdef WIN32
	commandname = v[0];
#endif
	for (i=1; i<c; ++i) {
		if (v[i][0] == '-') {
			if (!strcmp(v[i], "--no-fork"))
				opt_nofork = 1;
			else if (!strcmp(v[i], "--exitcode"))
				opt_exitcode = 1;
			else if (!strcmp(v[i], "--quiet"))
				opt_verbosity = 0;
			else if (!strcmp(v[i], "--loud"))
				opt_verbosity = 2;
		} else if (strchr(v[i],'*')) {
			int found = 0;
			int pos = strchr(v[i], '*')-v[i];
			for (j=0; j<n_testcases; ++j) {
				if (!strncmp(cases[j].name, v[i], pos))
					tc_enabled[j] = found = 1;
			}
			if (!found) {
				printf("No tests matched %s!\n", v[i]);
				goto out;
			}
		} else {
			int found = 0;

			n_enabled++;
			for (j=0; j<n_testcases; ++j) {
				if (!strcmp(cases[j].name, v[i])) {
					tc_enabled[j] = found = 1;
					break;
				}
			}
			if (!found) {
				printf("No such test as %s!\n", v[i]);
				goto out;
			}
		}
	}
	if (!n_enabled) {
		for (j=0; j<n_testcases; ++j)
			tc_enabled[j] = 1;
	}

	setvbuf(stdout, NULL, _IONBF, 0);

	in_tinytest_main = 1;
	for (i=0; i<n_testcases; ++i) {
		if (tc_enabled[i])
			testcase_run(&cases[i]);
	}
	in_tinytest_main = 0;

	if (n_bad == 0)
		returnval = 1;
	else
		returnval = 0;
 out:
	if (tc_enabled)
		free(tc_enabled);
	return returnval;
}

int
_tinytest_get_verbosity(void)
{
	return opt_verbosity;
}

void
_tinytest_set_test_failed(void)
{
	if (opt_verbosity == 0 && cur_test_name) {
		printf("%s... ", cur_test_name);
		cur_test_name = 0;
	}
	cur_test_outcome = 0;
}

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
 end:
	return;
}

struct testcase_t tc[] = {
	{ "add", testcase_add, TT_FORK, NULL },
	{ "add2", testcase_add2, 0, NULL },
	{ NULL, NULL, 0 },
};

int main(int c, const char **v)
{
	tinytest_main(c, v, tc);
	return 0;
}
