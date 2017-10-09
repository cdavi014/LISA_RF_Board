#include "lisa_lib.h"
#include "alg_test.h"

int corruption_pct = 50, match_confidence = 60, payload_idx_output, payload_idx_input;
unsigned char lisa_sync_buffer[LISA_SYNC_LEN] = { 0 };
unsigned char lisa_bit_buffer[LISA_SYNC_LEN * 8] = { 0 };
unsigned char lisa_org_bit_buffer[LISA_SYNC_LEN * 8] = { 0 };
unsigned char input_buffer[BUFFER_LEN] = { 0 };
unsigned char input_buffer_bin[BUFFER_LEN * 8] = { 0 };
char * payload = "Carlos_Davila_010779067_CMPE240";

int main() {
	int payload_idx = 0;
	unsigned char curr;
	// Generate LISA Sync
	generate_lisa_sync_binary(0, lisa_sync_buffer, lisa_org_bit_buffer);
	generate_lisa_sync_binary(corruption_pct, lisa_sync_buffer, lisa_bit_buffer);
	// Combine with payload
	payload_idx = gen_output_buffer_idx(input_buffer, lisa_sync_buffer, payload,
			10);

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
	return 0;
}
