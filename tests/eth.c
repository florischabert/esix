#include "test.h"

#include <esix.h>
#include "../src/esix.h"
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

static test_ret test_send(void)
{
	esix_buffer *buffer;
	esix_eth_hdr *hdr;
	esix_eth_addr null_addr = {{ 0, 0, 0 }};
	esix_eth_addr dst_addr = {{ 1, 2, 3 }};
	char payload[] = "payload";
	esix_eth_addr dst_addr_n;
	int i;

	esix_internal_init();
	
	esix_eth_send(&dst_addr, esix_eth_type_ip6, payload, sizeof(payload));

	buffer = esix_outqueue_pop();
	require(buffer);
	require(buffer->data);

	hdr = buffer->data;
	require(buffer->len == sizeof(payload) + sizeof(esix_eth_hdr));

	for (i = 0; i < 3; i++) {
		dst_addr_n.raw[i] = hton16(dst_addr.raw[i]);
	}
	require(esix_eth_addr_match(&hdr->dst_addr, &dst_addr_n));
	require(esix_eth_addr_match(&hdr->src_addr, &null_addr));
	require(hdr->type == hton16(esix_eth_type_ip6));
	require(memcmp(hdr+1, payload, sizeof(payload)) == 0);
	
	return test_passed;
}

static test_ret test_receive(void)
{
	esix_buffer *buffer;
	esix_eth_hdr *hdr;
	esix_eth_addr null_addr = {{ 0, 0, 0 }};
	esix_eth_addr dst_addr = {{ 1, 2, 3 }};
	char payload[] = "payload";
	esix_eth_addr dst_addr_n;
	int i;
	uint8_t data[] =
		"\x00\x01\x00\x02\x00\x03"     // dst MAC
		"\x00\x00\x00\x00\x00\x00"     // src MAC
		"\x86\xdd"       // ethernet type
		"payload"; //payload

	esix_internal_init();
	
	esix_enqueue(data, sizeof(data));

	buffer = esix_inqueue_pop();
	require(buffer);
	require(buffer->data);

	hdr = buffer->data;
	require(buffer->len == sizeof(payload) + sizeof(esix_eth_hdr));

	for (i = 0; i < 3; i++) {
		dst_addr_n.raw[i] = hton16(dst_addr.raw[i]);
	}
	require(esix_eth_addr_match(&hdr->dst_addr, &dst_addr_n));
	require(esix_eth_addr_match(&hdr->src_addr, &null_addr));
	require(hdr->type == hton16(esix_eth_type_ip6));
	require(memcmp(hdr+1, payload, sizeof(payload)) == 0);

	return test_passed;
}


test_ret test_eth(void)
{
	test_f tests[] = {
		test_addr,
		test_send,
		test_receive,
		NULL
	};

	return run_tests(tests);
}