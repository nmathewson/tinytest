
#include "tinytest.h"

void run_legacy_test_fn(void *ptr);
extern int pair[2];
extern int test_ok;
extern int called;

extern const struct testcase_setup_t legacy_setup;

#define TT_NEED_SOCKETPAIR TT_FIRST_USER_FLAG

#define LEGACY(name,flags)						\
	{ #name, run_legacy_test_fn, flags, &legacy_setup,		\
	  test_##name }

#define tt_fail_sockerr(op,sock)					\
	TT_STMT_BEGIN							\
	int e = evutil_socket_errno(sock);				\
	_tinytest_set_test_failed();					\
	TT_GRIPE(("%s: %s",(op),evutil_socket_error_to_string(_e)));	\
	TT_EXIT_TEST_FUNCTION;						\
	TT_STMT_END



