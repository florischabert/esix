#include "test.h"
#include <esix.h>

extern test_ret test_tools();
extern test_ret test_nd6();
extern test_ret test_eth();
extern test_ret test_ip6();
extern test_ret test_icmp6();
extern test_ret test_udp6();
extern test_ret test_tcp6();
extern test_ret test_socket();
extern test_ret test_misc();

typedef struct {
	char *name;
	test_ret (*func)(void);
} test_t;

static test_t tests[] = {
	{ "misc", test_misc },
	{ "tools", test_tools },
	{ "ethernet", test_eth },
	{ "nd6", test_nd6 },
	{ "icmpv6", test_icmp6 },
	{ "ipv6", test_ip6 },
	{ "udpv6", test_udp6 },
	{ "tcpv6", test_tcp6 },
	{ "socket", test_socket },
};

test_ret run_tests(test_f tests[])
{
	test_ret ret = test_passed;
	test_f *test;

	for (test = tests; *test; test++) {
		if ((*test)() == test_failed) {
			ret = test_failed;
		}
	}

	return ret;
}

void esix_send(void *data, int len) {
	
}

int main(int argc, char const *argv[])
{
	const int num_tests = sizeof(tests)/sizeof(tests[0]);
	int failures = 0;
	test_t *test;

	printf("Running esix test suite:\n");

	for (test = tests; test < tests + num_tests; test++) {
		printf("> testing %s...\n", test->name);
		if (test->func() == test_failed) {
			failures++;
		}
	}

	if (failures) {
		printf("%d test%s failed.\n", failures, failures > 1 ? "s" : "");
	}
	else {
		printf("All test passed.\n");
	}

	return 0;
}
