/* tinytest.c -- Copyright 2009 Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

static int in_tinytest_main = 0; /**< true if we're in tinytest_main().*/
static int n_ok = 0; /**< Number of tests that have passed */
static int n_bad = 0; /**< Number of tests that have failed. */

static int opt_forked = 0; /**< True iff we're called from inside a win32 fork*/
static int opt_verbosity = 1; /**< 0==quiet,1==normal,2==verbose */

static int cur_test_outcome = 0; /**< True iff the current test has failed. */
const char *cur_test_prefix = NULL; /**< prefix of the current test group */
/** Name of the  current test, if we haven't logged is yet. Used for --quiet */
const char *cur_test_name = NULL;

#ifdef WIN32
/** Pointer to argv[0] for win32. */
static const char *commandname = NULL;
#else
/** Pipe-pair for Unix IPC */
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
	if (opt_verbosity)
		printf("[forking] ");

	verbosity = (opt_verbosity == 2) ? "--verbose" :
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
	else {
		cur_test_prefix = group->prefix;
		cur_test_name = testcase->name;
	}

	if ((testcase->flags & TT_FORK) && !opt_forked) {
		outcome = _testcase_run_forked(group, testcase);
	} else {
		outcome  = _testcase_run_bare(testcase);
	}

	if (outcome) {
		++n_ok;
		if (opt_verbosity && !opt_forked)
			puts(opt_verbosity==1?"OK":"");
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
	int length = LONGEST_TEST_NAME;
	if (strstr(arg, ".."))
		length = strstr(arg,"..")-arg;
	char fullname[LONGEST_TEST_NAME];
	int found=0;
	for (i=0; groups[i].prefix; ++i) {
		for (j=0; groups[i].cases[j].name; ++j) {
			snprintf(fullname, sizeof(fullname), "%s%s",
				 groups[i].prefix, groups[i].cases[j].name);
			if (!flag) /* Hack! */
				printf("    %s\n", fullname);
			if (!strncmp(fullname, arg, length)) {
				groups[i].cases[j].flags |= flag;
				++found;
			}
		}
	}
	return found;
}

static void
usage(struct testgroup_t *groups)
{
	puts("Options are: --verbose --quiet");
	puts("Known tests are:");
	_tinytest_set_flag(groups, "..", 0);
	exit(0);
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
			else if (!strcmp(v[i], "--verbose"))
				opt_verbosity = 2;
			else if (!strcmp(v[i], "--help"))
				usage(groups);
			else {
				printf("Unknown option %s.  Try --help\n",v[i]);
				return -1;
			}
		} else {
			++n;
			if (!_tinytest_set_flag(groups, v[i], _TT_ENABLED)) {
				printf("No such test as %s!\n", v[i]);
				return -1;
			}
		}
	}
	if (!n)
		_tinytest_set_flag(groups, "...", _TT_ENABLED);

	setvbuf(stdout, NULL, _IONBF, 0);

	++in_tinytest_main;
	for (i=0; groups[i].prefix; ++i)
		for (j=0; groups[i].cases[j].name; ++j)
			if (groups[i].cases[j].flags & _TT_ENABLED)
				testcase_run(&groups[i], &groups[i].cases[j]);

	--in_tinytest_main;

	if (n_bad)
		printf("%d TESTS FAILED.\n", n_bad);
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
		printf("%s%s... ", cur_test_prefix, cur_test_name);
		cur_test_name = NULL;
	}
	cur_test_outcome = 0;
}

