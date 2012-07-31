#ifndef _TEST_H
#define _TEST_H

#include <stdio.h>

typedef enum {
	test_passed,
	test_failed
} test_ret;

#define require(cond, label) \
do { \
	if (!(cond)) { \
		fprintf(stderr, "%s:%d: " #cond " failed!\n", __FILE__, __LINE__); \
		goto label; \
	} \
} while (0)

#endif