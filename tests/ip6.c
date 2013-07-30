#include "test.h"

#include <esix.h>
#include "../src/nd6.h"
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

static test_ret test_send(void)
{
	esix_buffer *buffer;
	esix_ip6_hdr *hdr;
	esix_ip6_addr src_addr = {{ 1, 2, 3, 4 }};
	esix_ip6_addr dst_addr = {{ 5, 6, 7, 8 }};
	esix_ip6_addr src_addr_n;
	esix_ip6_addr dst_addr_n;	
	char payload[] = "payload";
	esix_lla lla = { 0, 0, 0, 0, 0, 0 };
	esix_eth_addr eth_addr;
	esix_ip6_addr mask = {{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }};
	 esix_ip6_addr addr_null = {{ 0, 0, 0, 0 }};
	int i;
	
	esix_internal_init();
	esix_nd6_init(lla);
	eth_addr = esix_nd6_lla();

	esix_nd6_add_neighbor(&dst_addr, &eth_addr, 0);
	esix_nd6_add_route(&dst_addr, &mask, &addr_null, 0, 0, 0);

	esix_ip6_send(&src_addr, &dst_addr, 0, esix_ip6_next_icmp, payload, sizeof(payload));

	esix_outqueue_pop(); // autoconf neighbor sol
	buffer = esix_outqueue_pop();
	require(buffer);
	require(buffer->data);

	hdr = (void *)((uintptr_t)buffer->data + sizeof(esix_eth_hdr));
	require(buffer->len == sizeof(payload) + sizeof(esix_ip6_hdr) + sizeof(esix_eth_hdr));

	for (i = 0; i < 4; i++) {
		src_addr_n.raw[i] = hton32(src_addr.raw[i]);
		dst_addr_n.raw[i] = hton32(dst_addr.raw[i]);
	}
	require(esix_ip6_addr_match(&hdr->dst_addr, &dst_addr_n));
	require(esix_ip6_addr_match(&hdr->src_addr, &src_addr_n));
	require(hdr->next_header == esix_ip6_next_icmp);
	require(memcmp(hdr+1, payload, sizeof(payload)) == 0);

	return test_passed;
}

static test_ret test_checksum(void)
{
	esix_ip6_addr src = {{ 1, 2, 3, 4 }};
	esix_ip6_addr dst = {{ 5, 6, 7, 8 }};
	char data[] = "abcdefabcdefabcdefabcdefabcdefabcdefabcdef";
	uint16_t expected_sum = 24280;
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