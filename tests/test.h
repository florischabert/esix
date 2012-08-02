#ifndef _TEST_H
#define _TEST_H

#include <stdio.h>

typedef enum {
	test_passed,
	test_failed,
} test_ret;

typedef test_ret (*test_f)(void);
test_ret run_tests(test_f tests[]); // NULL-terminated array

#define require(cond) \
do { \
	if (!(cond)) { \
		fprintf(stderr, "%s:%d in %s: " #cond " failed!\n", __FILE__, __LINE__, __FUNCTION__); \
		return test_failed; \
	} \
} while (0)

#define require_or_goto(cond, label) \
do { \
	if (!(cond)) { \
		fprintf(stderr, "%s:%d in %s: " #cond " failed!\n", __FILE__, __LINE__, __FUNCTION__); \
		goto label; \
	} \
} while (0)

#endif