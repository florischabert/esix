#include "test.h"

#include <esix.h>
#include "../src/intf.h"
#include "../src/eth.h"
#include "../src/ip6.h"

static test_ret test_addr(void)
{
	esix_ip6_addr addr;
	esix_ip6_addr same_addr;
	esix_ip6_addr bad_addr;

	addr = esix_ip6_addr_create(1, 2, 3, 4);
	same_addr = esix_ip6_addr_create(1, 2, 3, 4);
	bad_addr = esix_ip6_addr_create(4, 3, 2, 1);

	require(esix_ip6_addr_match(&addr, &same_addr));
	require(!esix_ip6_addr_match(&addr, &bad_addr));

	return test_passed;
}

static int sent_ok = 0;
static esix_ip6_addr src_addr = {{ 1, 2, 3, 4 }};
static esix_ip6_addr dst_addr = {{ 5, 6, 7, 8 }};
static char payload[] = "payload";

static test_ret validate_frame(void *data, int len)
{
	esix_ip6_hdr *hdr = (void *)((uintptr_t)data + sizeof(esix_eth_hdr));
	esix_ip6_addr src_addr_n;
	esix_ip6_addr dst_addr_n;	
	int i;

	for (i = 0; i < 4; i++) {
		src_addr_n.raw[i] = hton32(src_addr.raw[i]);
		dst_addr_n.raw[i] = hton32(dst_addr.raw[i]);
	}

	require(len == sizeof(payload) + sizeof(esix_ip6_hdr) + sizeof(esix_eth_hdr));
	require(esix_ip6_addr_match(&hdr->dst_addr, &dst_addr_n));
	require(esix_ip6_addr_match(&hdr->src_addr, &src_addr_n));
	require(hdr->next_header == esix_ip6_next_icmp);
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
	esix_eth_addr eth_addr = {{ 0, 0, 0 }};
	esix_ip6_addr mask = {{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }};

	err = esix_worker(send_callback);
	require(!err);

	esix_intf_init();
	esix_intf_add_neighbor(&dst_addr, &eth_addr, 0);
	esix_intf_add_route(&dst_addr, &mask, &dst_addr, 0, 0, 0);

	esix_ip6_send(&src_addr, &dst_addr, 0, esix_ip6_next_icmp, payload, sizeof(payload));
	
	return sent_ok ? test_passed : test_failed;
}

static test_ret test_checksum(void)
{
	esix_ip6_addr src = {{ 1, 2, 3, 4 }};
	esix_ip6_addr dst = {{ 5, 6, 7, 8 }};
	char data[] = "abcdefabcdefabcdefabcdefabcdefabcdefabcdef";
	uint16_t expected_sum = 24274;
	uint16_t sum;

	sum = esix_ip6_upper_checksum(&src, &dst, esix_ip6_next_icmp, data, sizeof(data));
	require(expected_sum == sum);

	return test_passed;
}

test_ret test_ip6(void)
{
	test_f tests[] = {
		test_addr,
		test_send,
		test_checksum,
		NULL
	};

	return run_tests(tests);
}