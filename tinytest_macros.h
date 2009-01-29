/* tinytest_macros.h -- Copyright 2009 Nick Mathewson

   Anyone may use, reproduce, or modify this work in any way so long as the
   above copyright notice and this license statement are retained.  There is
   no warranty.
*/

#ifndef _TINYTEST_MACROS_H
#define _TINYTEST_MACROS_H

#define TT_STMT_BEGIN do {
#define TT_STMT_END } while(0)

#ifndef TT_EXIT_TEST_FUNCTION
#define TT_EXIT_TEST_FUNCTION TT_STMT_BEGIN goto end; TT_STMT_END
#endif

#ifndef TT_GRIPE
#define TT_GRIPE(args)					\
	TT_STMT_BEGIN					\
	printf("\n  FAIL %s:%d: ",__FILE__,__LINE__);	\
	printf args ;					\
	TT_STMT_END
#endif

#ifndef TT_BLATHER
#define TT_BLATHER(args)					\
	TT_STMT_BEGIN						\
	if (_tinytest_get_verbosity() > 1) {			\
		printf("\n    OK %s:%d: ",__FILE__,__LINE__);	\
		printf args ;					\
	}							\
	TT_STMT_END
#endif

#define tt_fail_msg(msg)			\
	TT_STMT_BEGIN				\
	_tinytest_set_test_failed();		\
	TT_GRIPE((msg));			\
	TT_EXIT_TEST_FUNCTION;			\
	TT_STMT_END

#define tt_fail() tt_fail_msg("(Failed.)")

#define _tt_want(b, msg, fail)				\
	TT_STMT_BEGIN					\
	if (!(b)) {					\
		_tinytest_set_test_failed();		\
		TT_GRIPE((msg));			\
		fail;					\
	} else {					\
		TT_BLATHER((msg));			\
	}						\
	TT_STMT_END

#define tt_want_msg(b, msg)			\
	_tt_want(b, msg, );

#define tt_assert_msg(b, msg)			\
	_tt_want(b, msg, TT_EXIT_TEST_FUNCTION);

#define tt_want(b)   tt_want_msg( (b), "want("#b")")
#define tt_assert(b) tt_assert_msg((b), "assert("#b")")

#define tt_assert_eq_type(a,b,type,fmt)				   \
	TT_STMT_BEGIN						   \
	type _val1 = (type)(a);					   \
	type _val2 = (type)(b);					   \
	if (_val1 != _val2) {					   \
		_tinytest_set_test_failed();			   \
		TT_GRIPE(("assert(%s == %s): "fmt" != "fmt,	   \
			  #a, #b, _val1, _val2));		   \
		TT_EXIT_TEST_FUNCTION;				   \
	} else {						   \
		TT_BLATHER(("assert(%s == %s): "fmt" == "fmt,	   \
			    #a, #b, _val1, _val2));		   \
	}							   \
	TT_STMT_END

#define tt_assert_neq_type(a,b,type,fmt)			   \
	TT_STMT_BEGIN						   \
	type _val1 = (type)(a);					   \
	type _val2 = (type)(b);					   \
	if (_val1 == _val2) {					   \
		_tinytest_set_test_failed();			   \
		TT_GRIPE(("assert(%s != %s): "fmt" == "fmt,	   \
			  #a, #b, _val1, _val2));		   \
		TT_EXIT_TEST_FUNCTION;				   \
	} else {						   \
		TT_BLATHER(("assert(%s != %s): "fmt" == "fmt,	   \
			    #a, #b, _val1, _val2));		   \
	}							   \
	TT_STMT_END

#define tt_int_eq(a,b)				\
	tt_assert_eq_type(a,b,long,"%ld")

#define tt_uint_eq(a,b)				\
	tt_assert_eq_type(a,b,long,"%ld")

#define tt_int_neq(a,b)				\
	tt_assert_neq_type(a,b,unsigned long,"%lu")

#define tt_uint_neq(a,b)			\
	tt_assert_neq_type(a,b,unsigned long,"%lu")

#endif
