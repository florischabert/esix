#include "test.h"

test_ret test_tools(void)
{
	test_ret ret = test_failed;

	require(0==1, out);

	ret = test_passed;

out:
	return ret;
}