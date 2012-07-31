#include "test.h"

#include "src/tools.h"


static test_ret test_foreach(void)
{
	test_ret ret = test_failed;
	const int size = 100;
	int tab[size];
	int *pi;
	int i;

	for (i = 0; i < size; i++) {
		tab[i] = i;
	}

	i = 0;
	esix_foreach(pi, tab) {
		require(i == *pi, out);
		i++;
	}

	ret = test_passed;

out:
	return ret;
}

typedef struct {
	int val;
	esix_list link;
} mydata;

static test_ret test_list(void)
{
	test_ret ret = test_failed;
	esix_list list;
	const int size = 100;
	mydata data[size];
	mydata *pdata;
	int i;

	for (i = 0; i < size; i++) {
		data[i].val = i;
	}

	esix_list_init(&list);
	require(esix_list_empty(&list), out);

	for (i = 0; i < size; i++) {
		esix_list_add(&data[i].link, &list);
	}
	require(!esix_list_empty(&list), out);

	i = size-1;
	esix_list_foreach(pdata, &list, link) {
		require(pdata == &data[i], out);
		require(pdata->val == i, out);
		i--;
	}

	for (i = 0; i < size; i++) {
		require(!esix_list_empty(&list), out);
		
		esix_list_tail(pdata, &list, link);
		require(pdata == &data[i], out);
		require(pdata->val == i, out);

		esix_list_del(&pdata->link);
	}
	require(esix_list_empty(&list), out);

	for (i = 0; i < size; i++) {
		esix_list_add(&data[i].link, &list);
	}
	require(!esix_list_empty(&list), out);

	for (i = 0; i < size; i++) {
		require(!esix_list_empty(&list), out);
		esix_list_del(&data[i].link);
	}
	require(esix_list_empty(&list), out);


	ret = test_passed;

out:
	return ret;
}

test_ret test_tools(void)
{
	test_ret ret = test_failed;

	should_pass(test_foreach(), out);
	should_pass(test_list(), out);

	ret = test_passed;

out:
	return ret;
}