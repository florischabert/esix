#include "test.h"

#include <esix.h>


static test_ret test_neighbor_sol(void)
{
	return test_passed;
}

test_ret test_nd6(void)
{
	test_f tests[] = {
		test_neighbor_sol,
		NULL
	};

	return run_tests(tests);
}