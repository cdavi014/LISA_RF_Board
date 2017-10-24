/*
 * my_lisa.c
 *
 *  Created on: Sep 16, 2017
 *      Author: Carlos R. Davila
 *
 *      Status: Working version. Tests and robustness still WIP.
 */

#include "lisa_lib.h"

void join_lisa_payload(unsigned char * buffer, unsigned char * lisa_sync, int lisa_len,
		char * payload, int payload_len) {

	// Insert LISA and payload fields byte by byte
	for (int i = 0; i < lisa_len; i++) {
		buffer[i] = lisa_sync[i];
	}

	for (int i = 0; i < payload_len; i++) {
		buffer[i + lisa_len] = payload[i];
	}
}

/**
 * Function will combine lisa_sync + payload and place it in the
 * buffer provided at random location.
 *
 * @param buffer    Output buffer to place sync + payload.
 * @param lisa_sync Sync field to place in the buffer
 * @param payload   The payload to place in the output buffer
 * @param gen_file  Whether to generate an output file [1] or not [0]. The
 *                  name of the output file will be lisa_output.txt
 */
int gen_output_buffer_rand(unsigned char * buffer, unsigned char * lisa_sync,
		char * payload) {

	int lisa_idx, payload_idx;
	int payload_len = strlen(payload);

	// Generate random offset for sync + payload
	// between 0 and BUFFER_LEN - LISA_SYNC_LEN - payload_len
	lisa_idx = rand() % (BUFFER_LEN - LISA_SYNC_LEN - payload_len);
	payload_idx = lisa_idx + LISA_SYNC_LEN;

	// Insert LISA and payload fields byte by byte
	for (int i = lisa_idx; i < lisa_idx + LISA_SYNC_LEN; i++) {
		buffer[i] = lisa_sync[i - lisa_idx];
		buffer[i + LISA_SYNC_LEN] = payload[i - lisa_idx];
	}

	if (DEBUG == 1) {
		for (int i = lisa_idx; i < lisa_idx + LISA_SYNC_LEN; i++)
			printf("output_buffer[%d] = %x\n", i, buffer[i] & 0xff);

		for (int i = payload_idx; i < payload_idx + payload_len; i++)
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
void generate_lisa_sync_binary(int corrupt_pct,
		unsigned char * lisa_sync_buffer, unsigned char * lisa_bit_buffer) {
	// Generate standard LISA
	generate_lisa_sync(corrupt_pct, lisa_sync_buffer);

	printf("LISA Corrupted (%d): %s", corrupt_pct, lisa_sync_buffer);

	// Extract LISA bits
	for (int byte_n = 0; byte_n < LISA_SYNC_LEN; byte_n++) {
		for (int bit_n = 7; bit_n >= 0; bit_n--) {
			//convert to binary and store
			lisa_bit_buffer[byte_n * 8 + 7 - bit_n] = ((lisa_sync_buffer[byte_n]
					>> bit_n) & 1);
		}
	}
}

/**
 * Generate a LISA sync buffer with specified corruption
 *
 * @param corrupt_pct       Percentage of bits to corrupt in LISA buffer (0-100)
 * @param lisa_sync_buffer  Buffer to save sync field to
 */
void generate_lisa_sync(int corrupt_pct, unsigned char * lisa_sync_buffer) {
	char lisa_prefixes[2] = { 0xA0, 0x50 };
	int idx;

	// Generate standard LISA
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 16; j++) {
			idx = i * 16 + j;
			lisa_sync_buffer[idx] = lisa_prefixes[i] + j;

			if (DEBUG > 1)
				printf("gen_lisa_buffer[%d] = %x\n", idx, lisa_sync_buffer[idx] & 0xff);
		}
	}

	// Corrupt LISA (non-uniform distribution)
	int num_corrupt_bits = (LISA_SYNC_LEN * 8 * corrupt_pct) / 100;
	int rand_bit_buffer[num_corrupt_bits]; // history of corrupted bits
	int global_byte_offset = 0; // byte offset where bit will be corrupted
	uint bit_mask = 0;          // bit mask to invert bit
	int rand_bit = 0;           // corruption bit offset

	for (int i = 0; i < num_corrupt_bits; i++) {
		while (1) {
			rand_bit = rand() % (LISA_SYNC_LEN * 8); // generate offset location

			// check if location has already been corrupted
			for (int j = 0; j < num_corrupt_bits; j++)
				if (rand_bit_buffer[j] == rand_bit)
					continue;

			rand_bit_buffer[i] = rand_bit;
			break;
		}

		// go to offset byte and invert the selected bit offset
		global_byte_offset = rand_bit / 8;
		bit_mask = gen_mask(rand_bit - global_byte_offset * 8);
		lisa_sync_buffer[global_byte_offset] ^= bit_mask;

		if (DEBUG >= 1)
			printf("[%d] Byte after applying mask [%d] at %d: %x\n", i, bit_mask,
					global_byte_offset, lisa_sync_buffer[global_byte_offset]);
	}
}

/**
 * Standard LISA algorithm discussed in class. Will implement custom ones for
 * fun to test speed and other reliability improvements.
 *
 * @param confidence_lvl	confidence level for the search algorithm to test
 * @param input_buffer		buffer to search for the LISA sync and payload
 */
int lisa_find_payload_binary(int confidence_lvl, unsigned char * input_buffer,
		unsigned char * lisa_bit_buffer) {
	int num_matched, max_match_idx = 0;
	double max_matched_pct = 0.0;
	int lisa_bit_len = LISA_SYNC_LEN * 8;
	int buffer_bit_len = BUFFER_LEN;
	int window_match_pcts[BUFFER_LEN] = { 0 };
	int payload_location = -1;

	// Window method
	for (int buff_idx = 0; buff_idx < buffer_bit_len; buff_idx++) {
		for (int lisa_idx = 0; lisa_idx < lisa_bit_len; lisa_idx++) {
			if (input_buffer[(buff_idx + lisa_idx) % (buffer_bit_len)]
					== lisa_bit_buffer[lisa_idx])
				num_matched++;
		}
		window_match_pcts[buff_idx] = num_matched;
		if (window_match_pcts[max_match_idx] < num_matched) {
			max_match_idx = buff_idx;
		}
		num_matched = 0;
	}
	max_matched_pct = (double) window_match_pcts[max_match_idx] / lisa_bit_len;

	if (max_matched_pct * 100 >= confidence_lvl) {
		payload_location = (max_match_idx + lisa_bit_len) % (buffer_bit_len);
	}
	//printf("\n[INFO] Match at (%d) with confidence: %.2f%%\n", payload_location,
	//		max_matched_pct * 100);
	return payload_location;
}

// Relies on null character
void print_payload(int idx, unsigned char * input_buffer, int buffer_len) {
	unsigned int curr = 0, temp = 0, shift = 0;
	//char rcv_pld [256] = {'\0'};
	printf("\nMessage is: ");
	for (int i = 0; i < 256; i++) {
		temp = input_buffer[(i + idx) % buffer_len];
		shift = (7 - i % 8);
		if (temp == 0)
			curr |= 0 << shift;
		else
			curr |= 1 << shift;
		if (shift == 0 && i > 0) {
			printf("%c", (unsigned char) curr);
			curr = 0;
		}
	}
	printf("\n");
}

time_t get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec;
}

long get_clock_time_us() {
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return tv.tv_sec * 1000000 + tv.tv_nsec / 1000;
}

/**
 * Converts any character string into a char array of '1' and '0'
 */
void char_to_bin(char * input, unsigned char ** result) {
	int input_len = strlen(input);
	unsigned char curr;

	*result = (unsigned char *) calloc(input_len * 8 + 1, sizeof(unsigned char));

	for (int i = 0; i < input_len; i++) {
		curr = input[i];
		for (int j = 0; j < 8; j++) {
			(*result)[8 * i + j] = ((curr >> (7 - j)) & 1);
		}
	}
}

