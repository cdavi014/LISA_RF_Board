/*
 * my_lisa.c
 *
 *  Created on: Sep 16, 2017
 *      Author: Carlos R. Davila
 *
 *      Status: Working version. Tests and robustness still WIP.
 */

#include "my_lisa.h"

/**
 * Function will combine lisa_sync + payload and place it in the
 * buffer provided.
 *
 * @param buffer    Output buffer to place sync + payload.
 * @param lisa_sync Sync field to place in the buffer
 * @param payload   The payload to place in the output buffer
 * @param gen_file  Whether to generate an output file [1] or not [0]. The
 *                  name of the output file will be lisa_output.txt
 */
int gen_output_buffer(unsigned char * buffer, unsigned char * lisa_sync,
		char * payload) {

    int lisa_idx, payload_idx;
    int payload_len = strlen(payload);

    // Generate random offset for sync + payload
    // between 0 and BUFFER_LEN - LISA_SYNC_LEN - payload_len
    lisa_idx = rand()%(BUFFER_LEN - LISA_SYNC_LEN - payload_len);
    payload_idx = lisa_idx + LISA_SYNC_LEN;

    // Insert LISA and payload fields byte by byte
    for(int i = lisa_idx; i < lisa_idx + LISA_SYNC_LEN; i++) {
            buffer[i] = lisa_sync[i - lisa_idx];
            buffer[i + LISA_SYNC_LEN] = payload[i - lisa_idx];
    }

    if(DEBUG == 1) {
        for(int i = lisa_idx; i < lisa_idx + LISA_SYNC_LEN; i++)
            printf("output_buffer[%d] = %x\n", i, buffer[i] & 0xff);

        for(int i = payload_idx; i < payload_idx + payload_len; i++)
                printf("output_buffer[%d] = %c\n", i, buffer[i]);
    }

    return payload_idx;
}

/**
 * Generate bit mask with a single 1 at 'position' and all other bits zeros
 *
 * @param position  Location of enabled bit in the sequence
 */
uint gen_mask(uint position) {
    return 1 << position;
}

/**
 * Generate a LISA sync buffer with specified corruption
 *
 * @param corrupt_pct       Percentage of bits to corrupt in LISA buffer (0-100)
 * @param lisa_sync_buffer  Buffer to save sync field to
 */
void generate_lisa_sync_binary(int corrupt_pct, unsigned char * lisa_sync_buffer, unsigned char * lisa_bit_buffer) {
    // Generate standard LISA
    generate_lisa_sync(0, lisa_sync_buffer);
	
    // Extract LISA bits
    for(int byte_n = 0; byte_n < LISA_SYNC_LEN; byte_n++) {
        for(int bit_n = 7; bit_n >= 0; bit_n--) { 
		//convert to binary and store
		lisa_bit_buffer[byte_n * 8 + 7 - bit_n] = ((lisa_sync_buffer[byte_n] >> bit_n) & 1);
	}
    }
}

/**
 * Standard LISA algorithm discussed in class. Will implement custom ones for
 * fun to test speed and other reliability improvements.
 *
 * @param confidence_lvl	confidence level for the search algorithm to test
 * @param input_buffer		buffer to search for the LISA sync and payload
 */
int lisa_find_payload_binary(int confidence_lvl, unsigned char * input_buffer, unsigned char * lisa_bit_buffer) {
    int num_matched, max_match_idx = 0;
    double max_matched_pct = 0.0;
    int lisa_bit_len = LISA_SYNC_LEN * 8;
    int window_match_pcts[8 * (BUFFER_LEN - LISA_SYNC_LEN)] = {0};
    int lisa_location = -1;

    // Window method
    for(int buff_idx = 0; buff_idx <= BUFFER_LEN; buff_idx++) {
        for(int lisa_idx = 0; lisa_idx < lisa_bit_len; lisa_idx++) {
            if(input_buffer[(buff_idx + lisa_idx) % BUFFER_LEN] == lisa_bit_buffer[lisa_idx])
                num_matched++;
        }
        window_match_pcts[buff_idx] = num_matched;
        if(window_match_pcts[max_match_idx] < num_matched)
            max_match_idx = buff_idx;
        num_matched = 0;
    }

    max_matched_pct = (double)window_match_pcts[max_match_idx] / lisa_bit_len;

    if(max_matched_pct * 100 >= confidence_lvl) {
        lisa_location = (max_match_idx + lisa_bit_len) % BUFFER_LEN;
    } 
	printf("[INFO] Match at (%d) with confidence: %.2f%%\n",
    		lisa_location, max_matched_pct * 100);
        return lisa_location;
}

// Relies on null character
void print_payload(int idx, unsigned char * input_buffer, int buffer_len) {
	unsigned int curr = 0, temp = 0, shift = 0;
	//char rcv_pld [256] = {'\0'};
	printf("\nMessage is: ");
	for(int i = 0; i < 256; i++) {
		temp = input_buffer[(i + idx) % buffer_len];
		shift = (7 - i % 8);
		if(temp == 0)
			curr |= 0 << shift;
		else
			curr |= 1 << shift;
		if(shift == 0 && i > 0) {
			printf("%c", (unsigned char) curr);
			curr = 0;
		}
	}
	printf("\n");
}
