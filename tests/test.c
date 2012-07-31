#include "test.h"

extern test_ret test_tools();
extern test_ret test_eth();
extern test_ret test_icmp6();

typedef struct {
	char *name;
	test_ret (*func)(void);
} test_t;

test_t tests[] = {
	{ "tools", test_tools },
	{ "ethernet", test_eth },
	{ "icmpv6", test_icmp6 },
};

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