#include "test.h"

extern test_ret test_tools();
extern test_ret test_intf();
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
	{ "interface", test_intf },
	{ "ethernet", test_eth },
	{ "ipv6", test_ip6 },
	{ "icmpv6", test_icmp6 },
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

	printf("-- %d/%d passed", num_tests - failures, num_tests);
	if (failures) {
		printf(" (%d failed)", failures);
	}
	printf("\n");

	return 0;
}
