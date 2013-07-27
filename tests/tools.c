#include "test.h"

#include "../src/tools.h"

static test_ret test_foreach(void)
{
	const int size = 100;
	int tab[size];
	int *pi;
	int i;

	for (i = 0; i < size; i++) {
		tab[i] = i;
	}

	i = 0;
	esix_foreach(pi, tab) {
		require(i == *pi);
		i++;
	}

	return test_passed;
}

typedef struct {
	int val;
	esix_list link;
} mydata;

static test_ret test_list(void)
{
	esix_list list;
	const int size = 100;
	mydata data[size];
	mydata *pdata;
	int i;

	for (i = 0; i < size; i++) {
		data[i].val = i;
	}

	esix_list_init(&list);
	require(esix_list_empty(&list));

	for (i = 0; i < size; i++) {
		esix_list_add(&data[i].link, &list);
	}
	require(!esix_list_empty(&list));

	i = size-1;
	esix_list_foreach(pdata, &list, link) {
		require(pdata == &data[i]);
		require(pdata->val == i);
		i--;
	}

	for (i = 0; i < size; i++) {
		require(!esix_list_empty(&list));
		
		esix_list_tail(pdata, &list, link);
		require(pdata == &data[i]);
		require(pdata->val == i);

		esix_list_del(&pdata->link);
	}
	require(esix_list_empty(&list));

	for (i = 0; i < size; i++) {
		esix_list_add(&data[i].link, &list);
	}
	require(!esix_list_empty(&list));

	for (i = 0; i < size; i++) {
		require(!esix_list_empty(&list));
		esix_list_del(&data[i].link);
	}
	require(esix_list_empty(&list));

	return test_passed;
}

static test_ret test_lock(void)
{
	return test_passed;
}

static test_ret test_hton(void)
{
	return test_passed;
}

test_ret test_tools(void)
{
	test_f tests[] = {
		test_foreach,
		test_list,
		test_hton,
		test_lock,
		NULL
	};

	return run_tests(tests);
}