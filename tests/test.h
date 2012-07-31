#ifndef _TEST_H
#define _TEST_H

#include <stdio.h>

typedef enum {
	test_passed,
	test_failed,
} test_ret;

#define require(cond, label) \
do { \
	if (!(cond)) { \
		fprintf(stderr, "%s:%d in %s: " #cond " failed!\n", __FILE__, __LINE__, __FUNCTION__); \
		goto label; \
	} \
} while (0)

#define should_pass(cond, label) \
do { \
	if ((cond) != test_passed) { \
		goto label; \
	} \
} while (0)

#endif