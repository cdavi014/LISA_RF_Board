#include "lisa_lib.h"
#include "alg_test.h"
#include "scrambling_lib.h"

int corruption_pct = 0, match_confidence = 60, payload_idx_output,
		payload_idx_input;
unsigned char lisa_sync_buffer[LISA_SYNC_LEN] = { 0 };
unsigned char lisa_bit_buffer[LISA_SYNC_LEN * 8] = { 0 };
unsigned char lisa_org_bit_buffer[LISA_SYNC_LEN * 8] = { 0 };
unsigned char input_buffer[BUFFER_LEN] = { 0 };
unsigned char input_buffer_bin[BUFFER_LEN * 8] = { 0 };
char * payload = "Carlos_Davila_010779067_CMPE245";

void test_lisa() {
	int payload_idx = 0;
	unsigned char curr;
	// Generate LISA Sync
	generate_lisa_sync_binary(0, lisa_sync_buffer, lisa_org_bit_buffer);
	generate_lisa_sync_binary(corruption_pct, lisa_sync_buffer, lisa_bit_buffer);

	// Combine with payload
	payload_idx = gen_output_buffer_idx(input_buffer, lisa_org_bit_buffer,
	LISA_SYNC_LEN, payload, strlen(payload), 10);

	// Convert to binary
	for (int i = 0; i < BUFFER_LEN; i++) {
		curr = input_buffer[i];
		for (int j = 0; j < 8; j++) {
			input_buffer_bin[8 * i + j] = ((curr >> (7 - j)) & 1);
		}
	}
	printf("\nBuffer[%d] -> %s\n", payload_idx, input_buffer + payload_idx);

	// Search for payload in the buffer
	payload_idx_input = lisa_find_payload_binary(match_confidence,
			input_buffer_bin, lisa_org_bit_buffer);

	printf("Payload found at: %d\n", payload_idx_input);
}

void test_scrambling(int order) {
	int L = calc_large(order);
	unsigned char payload_bin[14] = { 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1 };
	unsigned char scrambled_bin[14] = { 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0 };
	int payload_len = 14;
	unsigned char * scrambled_result;
	unsigned char * descrambled_result;

	printf("\nScrambling ...\n");
	scramble(payload_bin, payload_len, &scrambled_result, order);

	for (int i = 0; i < payload_len; i++) {
		if (scrambled_bin[i] != scrambled_result[i]) {
			printf("\nScrambled code not matching!\n");
			printf("[%d]->(T)%d != (R)%d\n", i, scrambled_bin[i],
					scrambled_result[i]);
			return;
		}
		printf("%d", scrambled_result[i]);
	}

	printf("\n");

	printf("\nDescrambling ...\n");
	descramble(scrambled_result, payload_len, &descrambled_result, order);

	for (int i = 0; i < payload_len; i++) {
		if (descrambled_result[i] != payload_bin[i]) {
			printf("\nDescrambled code not matching!\n");
			printf("[%d]->(T)%d != (R)%d\n", i, payload_bin[i],
					descrambled_result[i]);
			return;
		}
		printf("%d", descrambled_result[i]);
	}
	printf("\n\nAll tests passed.\n");
}

int main() {
	//test_lisa();
	test_scrambling(5);
	return 0;
}
