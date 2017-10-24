#include "lisa_lib.h"
#include "alg_test.h"
#include "scrambling_lib.h"

void test_lisa() {
	int corruption_pct = 0, match_confidence = 60, payload_idx_output,
			payload_idx_found;
	unsigned char lisa_sync_buffer[LISA_SYNC_LEN] = { 0 };
	unsigned char * lisa_bit_buffer;
	unsigned char * lisa_org_bit_buffer;
	unsigned char * payload_bit_buffer;
	unsigned char input_buffer[BUFFER_LEN] = { 0 };
	unsigned char input_buffer_bin[BUFFER_LEN * 8] = { 0 };
	char * payload = "Carlos_Davila_010779067_CMPE245";
	char * payload_found;
	int payload_idx = 0;
	unsigned char curr;

	printf("\n****************************\n");
	printf("* Starting LISA Sync tests *\n");
	printf("****************************\n\n");

	// Generate LISA Syncs in binary
	generate_lisa_sync_binary(0, lisa_sync_buffer, LISA_SYNC_LEN,
			&lisa_org_bit_buffer);

	generate_lisa_sync_binary(corruption_pct, lisa_sync_buffer, LISA_SYNC_LEN,
			&lisa_bit_buffer);

	// Convert payload to binary
	char_to_bin((unsigned char *) payload, strlen(payload), &payload_bit_buffer);

	// Combine with payload and return starting index
	payload_idx = gen_output_buffer_idx(input_buffer_bin, lisa_org_bit_buffer,
	LISA_SYNC_LEN * 8, payload_bit_buffer, strlen(payload) * 8, 10);

	// Search for payload in the buffer
	payload_idx_found = lisa_find_payload_binary(match_confidence,
			input_buffer_bin, lisa_org_bit_buffer);

	if (payload_idx_found != payload_idx) {
		printf(
				ANSI_COLOR_RED "\n[FAIL] idx found [%d] does not match original [%d]" ANSI_COLOR_RESET,
				payload_idx_found, payload_idx);
		return;
	}

	printf("Payload found at: %d\n", payload_idx_found);
	printf("Payload (b): ");
	for (int i = 0; i < strlen(payload) * 8; i++) {
		printf("%d", input_buffer_bin[i + payload_idx_found]);
	}

	bin_to_char(input_buffer_bin + payload_idx_found, strlen(payload) * 8, &payload_found);
	printf("\nPayload: %s", payload_found);

	printf(ANSI_COLOR_GREEN "\n\nAll LISA Sync tests passed.\n" ANSI_COLOR_RESET);
}

void test_scrambling(int order) {
	int L = calc_large(order);
	unsigned char payload_bin[14] = { 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1 };
	unsigned char scrambled_bin[14] = { 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0 };
	int payload_len = 14;
	unsigned char * scrambled_result;
	unsigned char * descrambled_result;

	printf("\n*******************************************\n");
	printf("* Starting Scrambling/De-scrambling tests *\n");
	printf("*******************************************\n\n");

	printf("\nScrambling ...\n");
	scramble(payload_bin, payload_len, &scrambled_result, order);

	for (int i = 0; i < payload_len; i++) {
		if (scrambled_bin[i] != scrambled_result[i]) {
			printf(
					ANSI_COLOR_RED "\n[FAIL] Scrambled code not matching!\n" ANSI_COLOR_RESET);
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
			printf(
					ANSI_COLOR_RED "\n[FAIL] Descrambled code not matching!\n" ANSI_COLOR_RESET);
			printf("[%d]->(T)%d != (R)%d\n", i, payload_bin[i],
					descrambled_result[i]);
			return;
		}
		printf("%d", descrambled_result[i]);
	}
	printf(
			ANSI_COLOR_GREEN "\n\nAll Scrambling/Descrambling tests passed.\n" ANSI_COLOR_RESET);
}

int main() {
	test_lisa();
	test_scrambling(5);
	return 0;
}
