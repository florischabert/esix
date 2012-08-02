#include "test.h"

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

typedef struct {
	char *name;
	test_ret (*func)(void);
} test_t;

extern test_ret test_tools();
extern test_ret test_eth();

static test_t tests[] = {
	{ "tools", test_tools },
	{ "ethernet", test_eth },
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
