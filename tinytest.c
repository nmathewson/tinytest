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
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "tinytest.h"
#include "tinytest_macros.h"

#define LONGEST_TEST_NAME 16384

static int in_tinytest_main = 0;
static int n_ok = 0;
static int n_bad = 0;

static int opt_forked = 0;
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
_testcase_run_forked(const struct testgroup_t *group,
		     const struct testcase_t *testcase)
{
#ifdef WIN32
	int ok;
	char buffer[LONGEST_TEST_NAME+256];
	const char *verbosity;
	STARTUPINFO si;
	PROCESS_INFORMATION info;
	DWORD exitcode;

	if (!in_tinytest_main) {
		printf("\nERROR.  On Windows, _testcase_run_forked must be"
		       " called from within tinytest_main.\n");
		abort();
	}
	printf("[forking] ");

	verbosity = (opt_verbosity == 2) ? "--loud" :
		(opt_verbosity == 0) ? "--quiet" : "";
	snprintf(buffer, sizeof(buffer), "%s --RUNNING-FORKED %s %s%s",
		 commandname, verbosity, group->prefix, testcase->name);

	memset(&si, 0, sizeof(si));
	memset(&info, 0, sizeof(info));
	si.cb = sizeof(si);

	ok = CreateProcess(commandname, buffer, NULL, NULL, 0,
			   0, NULL, NULL, &si, &info);
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
        (void)group;
	if (outcome_pipe[0] == -1) {
		if (pipe(outcome_pipe))
			perror("opening pipe");
	}

	if (opt_verbosity)
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
testcase_run(const struct testgroup_t *group, const struct testcase_t *testcase)
{
	int outcome;

	if (testcase->flags & TT_SKIP) {
		if (opt_verbosity)
			printf("%s%s... SKIPPED\n",
			    group->prefix, testcase->name);
		return 1;
	}

	if (opt_verbosity && !opt_forked)
		printf("%s%s... ", group->prefix, testcase->name);
	else
		cur_test_name = testcase->name;

	if ((testcase->flags & TT_FORK) && !opt_forked) {
		outcome = _testcase_run_forked(group, testcase);
	} else {
		outcome  = _testcase_run_bare(testcase);
	}

	if (outcome) {
		++n_ok;
		if (opt_verbosity && !opt_forked)
			puts("OK");
	} else {
		++n_bad;
		if (!opt_forked)
			printf("\n  [%s FAILED]\n", testcase->name);
	}

	if (opt_forked) {
		exit(outcome ? 0 : 1);
	} else {
		return outcome;
	}
}

int
_tinytest_set_flag(struct testgroup_t *groups, const char *arg, unsigned long flag)
{
	int i, j;
	int length = strchr(arg,'*') ? (strchr(arg,'*')-arg) : LONGEST_TEST_NAME;
	char fullname[LONGEST_TEST_NAME];
	int found=0;
	for (i=0; groups[i].prefix; ++i) {
		for (j=0; groups[i].cases[j].name; ++j) {
			snprintf(fullname, sizeof(fullname), "%s%s",
				 groups[i].prefix, groups[i].cases[j].name);
			if (!strncmp(fullname, arg, length)) {
				groups[i].cases[j].flags |= flag;
				++found;
			}
		}
	}
	return found;
}

int
tinytest_main(int c, const char **v, struct testgroup_t *groups)
{
	int i, j, n=0;

#ifdef WIN32
	commandname = v[0];
#endif
	for (i=1; i<c; ++i) {
		if (v[i][0] == '-') {
			if (!strcmp(v[i], "--RUNNING-FORKED"))
				opt_forked = 1;
			else if (!strcmp(v[i], "--quiet"))
				opt_verbosity = 0;
			else if (!strcmp(v[i], "--loud"))
				opt_verbosity = 2;
		} else {
			++n;
			if (!_tinytest_set_flag(groups, v[i], _TT_ENABLED)) {
				printf("No such test as %s!\n", v[i]);
				return -1;
			}
		}
	}
	if (!n)
		_tinytest_set_flag(groups, "*", _TT_ENABLED);

	setvbuf(stdout, NULL, _IONBF, 0);

	++in_tinytest_main;
	for (i=0; groups[i].prefix; ++i)
		for (j=0; groups[i].cases[j].name; ++j)
			if (groups[i].cases[j].flags & _TT_ENABLED)
				testcase_run(&groups[i], &groups[i].cases[j]);

	--in_tinytest_main;

	return (n_bad == 0) ? 1 : 0;
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
		cur_test_name = NULL;
	}
	cur_test_outcome = 0;
}

