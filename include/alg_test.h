#include "lisa_lib.h"

#define OUTPUT_MSG_LEN LISA_SYNC_LEN + 64

/** Function will combine lisa_sync + payload and place it in the
 * buffer provided at location idx.
 *
 * @param buffer    Output buffer to place sync + payload.
 * @param lisa_sync Sync field to place in the buffer
 * @param payload   The payload to place in the output buffer
 * @param gen_file  Whether to generate an output file [1] or not [0]. The
 *                  name of the output file will be lisa_output.txt
 * @param idx		Location of where to store the lisa + payload (circular buffer)
 */
int gen_output_buffer_idx(unsigned char * buffer, unsigned char * lisa_sync,
		char * payload, int lisa_idx) {
	int payload_len = strlen(payload);
	int payload_idx = (lisa_idx + payload_len) % BUFFER_LEN + 1;
	unsigned char lisa_payload[OUTPUT_MSG_LEN] = { 0 };

	if (lisa_idx >= BUFFER_LEN) {
		printf("[ERROR] Lisa idx is greater than BUFFER_LEN\n");
		exit(0);
	}

	// Combine lisa sync with payload
	join_lisa_payload(lisa_payload, lisa_sync, payload);

	// Insert LISA and payload fields byte by byte with circular buffering
	for (int i = 0; i < LISA_SYNC_LEN + payload_len; i++) {
		buffer[(i + lisa_idx) % BUFFER_LEN] = lisa_payload[i];
		printf("[%d] %c\n", (i + lisa_idx) % BUFFER_LEN, lisa_payload[i]);
	}

	return payload_idx;
}
