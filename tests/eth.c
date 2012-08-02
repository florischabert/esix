#include "test.h"

#include "../src/eth.h"

static test_ret test_addr(void)
{
	esix_eth_addr addr = {{ 1, 2, 3 }};
	esix_eth_addr same_addr = {{ 1, 2, 3 }};
	esix_eth_addr bad_addr = {{ 1, 2, 4 }};

	require(esix_eth_addr_match(&addr, &same_addr));
	require(!esix_eth_addr_match(&addr, &bad_addr));

	return test_passed;
}

test_ret test_eth(void)
{
	test_f tests[] = {
		test_addr,
		NULL
	};

	return run_tests(tests);
}