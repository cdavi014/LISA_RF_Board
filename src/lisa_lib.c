/*
 * my_lisa.c
 *
 *  Created on: Sep 16, 2017
 *      Author: Carlos R. Davila
 *
 *      Status: Working version. Tests and robustness still WIP.
 */

#include "lisa_lib.h"

/**
 * Joins the lisa_sync and the payload buffers and places them into buffer.
 */
void join_lisa_payload(unsigned char ** buffer, unsigned char * lisa_sync,
	int lisa_len, unsigned char * payload, int payload_len) {

	*buffer = (unsigned char *) calloc(lisa_len + payload_len, sizeof(unsigned char));

	// Insert LISA Sync
	for (int i = 0; i < lisa_len; i++) {
		(*buffer)[i] = lisa_sync[i];
	}

	// Insert Payload
	for (int i = 0; i < payload_len; i++) {
		(*buffer)[i + lisa_len] = payload[i];
	}
}

/** Function will combine lisa_sync + payload and place it in the
 * buffer provided at location specified by lisa_idx. This is intended for
 * testing purposes only.
 *
 * @param buffer    Output buffer to place sync + payload.
 * @param lisa_sync Sync field to place in the buffer
 * @param payload   The payload to place in the output buffer
 * @param lisa_idx	Location of where to store the lisa + payload (circular buffer)
 */
int gen_output_buffer_idx(unsigned char * buffer, int buffer_len, unsigned char * lisa_sync,
		int lisa_len, unsigned char * payload, int payload_len, int lisa_idx) {

	unsigned char * lisa_payload;
	int payload_idx = (lisa_idx + lisa_len) % buffer_len;

	if (lisa_idx >= BUFFER_LEN) {
		printf("[ERROR] Lisa idx is greater than BUFFER_LEN\n");
		exit(0);
	}

	// Combine lisa sync with payload
	join_lisa_payload(&lisa_payload, lisa_sync, lisa_len, payload,
			payload_len);

	// Insert LISA and payload fields byte by byte with circular buffering
	for (int i = 0; i < lisa_len + payload_len; i++) {
		buffer[(i + lisa_idx) % BUFFER_LEN] = lisa_payload[i];
	}

	return payload_idx;
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
int gen_output_buffer_rand(unsigned char * buffer, int buffer_len, unsigned char * lisa_sync,
		int lisa_len, unsigned char * payload, int payload_len) {

	srand((unsigned)time(NULL)); // Seed random generator

	// Generate random offset for sync + payload
	// between 0 and BUFFER_LEN - LISA_SYNC_LEN - payload_len
	int lisa_rnd_idx = rand() % (buffer_len - 1);
	int payload_idx = lisa_rnd_idx + lisa_len;

	printf("Rand idx: %d", lisa_rnd_idx);
	gen_output_buffer_idx(buffer, buffer_len, lisa_sync, lisa_len,
			payload, payload_len, lisa_rnd_idx);

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
		unsigned char * lisa_sync_buffer, int lisa_sync_len,
		unsigned char ** lisa_bit_buffer) {
	// Generate standard LISA
	generate_lisa_sync(corrupt_pct, lisa_sync_buffer);
	// Extract LISA bits
	char_to_bin(lisa_sync_buffer, lisa_sync_len, lisa_bit_buffer);
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

void extract_payload(int idx, unsigned char * input_buffer, int buffer_len, int max_payload_len, char ** payload) {
	// Extract payload from circular buffer
	unsigned char * result = (unsigned char *) calloc(max_payload_len, sizeof(unsigned char)); //temp buffer

	for(int i = 0; i < max_payload_len; i++) {
		result[i] = input_buffer[(idx + i) % buffer_len];
	}

	bin_to_char(result, max_payload_len, payload);// Convert to chars
	free(result);
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
void char_to_bin(unsigned char * input, int input_len, unsigned char ** result) {
	unsigned char curr;

	*result = (unsigned char *) calloc(input_len * 8 + 1, sizeof(unsigned char));

	for (int i = 0; i < input_len; i++) {
		curr = input[i];
		for (int j = 0; j < 8; j++) {
			(*result)[8 * i + j] = ((curr >> (7 - j)) & 1);
		}
	}
}

void bin_to_char(unsigned char * input, int input_len, char ** result) {
	*result = (char *) calloc(input_len / 8 + 1, sizeof(char));
	unsigned char curr;

	for (int i = 0; i < input_len; i++) {
		curr = input[i];
		(*result)[i / 8] = ((curr << (7 - i % 8)) | (*result)[i / 8]);
	}
}
