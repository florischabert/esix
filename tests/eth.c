#include "test.h"

test_ret test_eth(void)
{
	test_ret ret = test_failed;

	require(1==1, out);

	ret = test_passed;

out:
	return ret;
}