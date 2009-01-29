
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <evutil.h>

#include <sys/socket.h>

#include "tinytest.h"
#include "tinytest_macros.h"

#include "libevent_glue.h"

int pair[2];
int test_ok = 0;
int called = 0;


#define TT_NEED_SOCKETPAIR TT_FIRST_USER_FLAG

static void *
legacy_test_setup(const struct testcase_t *testcase)
{
	if (testcase->flags & TT_NEED_SOCKETPAIR) {
		if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == -1) {
			fprintf(stderr, "%s: socketpair\n", __func__);
			exit(1);
		}

		if (evutil_make_socket_nonblocking(pair[0]) == -1) {
			fprintf(stderr, "fcntl(O_NONBLOCK)");
			exit(1);
		}

		if (evutil_make_socket_nonblocking(pair[1]) == -1) {
			fprintf(stderr, "fcntl(O_NONBLOCK)");
			exit(1);
		}
	}

	return testcase->setup_data;
}

void
run_legacy_test_fn(void *ptr)
{
	void (*fn)(void);
	test_ok = called = 0;
	fn = ptr;
	fn();
	if (!test_ok)
		tt_fail();

 end:
	test_ok = 0;
}

static int
legacy_test_cleanup(void *ptr)
{
	(void)ptr;
	if (pair[0] != -1)
		EVUTIL_CLOSESOCKET(pair[0]);
	if (pair[1] != -1)
		EVUTIL_CLOSESOCKET(pair[1]);

	pair[0] = pair[1] = -1;

	return 1;
}

const struct testcase_setup_t legacy_setup = {
	legacy_test_setup, legacy_test_cleanup
};


