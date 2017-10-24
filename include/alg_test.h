#include "lisa_lib.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/** Function will combine lisa_sync + payload and place it in the
 * buffer provided at location specified by lisa_idx. This is intended for
 * testing purposes only.
 *
 * @param buffer    Output buffer to place sync + payload.
 * @param lisa_sync Sync field to place in the buffer
 * @param payload   The payload to place in the output buffer
 * @param lisa_idx	Location of where to store the lisa + payload (circular buffer)
 */
int gen_output_buffer_idx(unsigned char * buffer, unsigned char * lisa_sync,
		int lisa_len, unsigned char * payload, int payload_len, int lisa_idx) {

	//printf("Lisa_idx[%d] , Lisa_len[%d], Payload_len[%d], sum = %d\n", lisa_idx, lisa_len, payload_len,  (lisa_idx + payload_len) % BUFFER_LEN + 1);
	int payload_idx = (lisa_idx + lisa_len) % BUFFER_LEN;
	unsigned char * lisa_payload = (unsigned char *) calloc(lisa_idx + lisa_len + payload_len, sizeof(unsigned char));

	if (lisa_idx >= BUFFER_LEN) {
		printf("[ERROR] Lisa idx is greater than BUFFER_LEN\n");
		exit(0);
	}

	// Combine lisa sync with payload
	join_lisa_payload(lisa_payload, lisa_sync, lisa_len, payload,
			payload_len);

	// Insert LISA and payload fields byte by byte with circular buffering
	for (int i = 0; i < lisa_len + payload_len; i++) {
		buffer[(i + lisa_idx) % BUFFER_LEN] = lisa_payload[i];
	}

	return payload_idx;
}
