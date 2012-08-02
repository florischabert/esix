#include "test.h"

#include <esix.h>
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

static int sent_ok = 0;
static esix_eth_addr dst_addr = {{ 1, 2, 3 }};
static char payload[] = "payload";

static test_ret validate_frame(void *data, int len)
{
	esix_eth_hdr *hdr = data;
	esix_eth_addr null_addr = {{ 0, 0, 0 }};
	esix_eth_addr dst_addr_n;
	int i;

	for (i = 0; i < 3; i++) {
		dst_addr_n.raw[i] = hton16(dst_addr.raw[i]);
	}

	require(len == sizeof(payload) + sizeof(esix_eth_hdr));
	require(esix_eth_addr_match(&hdr->dst_addr, &dst_addr_n));
	require(esix_eth_addr_match(&hdr->src_addr, &null_addr));
	require(hdr->type == hton16(esix_eth_type_ip6));
	require(memcmp(hdr+1, payload, sizeof(payload)) == 0);

	return test_passed;
}

static void send_callback(void *data, int len)
{
	int ret;

	ret = validate_frame(data, len);
	sent_ok = (ret == test_passed);
}

static test_ret test_send(void)
{
	esix_err err;

	err = esix_worker(send_callback);
	require(!err);

	esix_eth_send(&dst_addr, esix_eth_type_ip6, payload, sizeof(payload));
	
	return sent_ok ? test_passed : test_failed;
}

test_ret test_eth(void)
{
	test_f tests[] = {
		test_addr,
		test_send,
		NULL
	};

	return run_tests(tests);
}