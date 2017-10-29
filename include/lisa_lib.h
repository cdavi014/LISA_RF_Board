/*
 * my_lisa.h
 *
 *  Created on: Sep 20, 2017
 *      Author: Carlos R. Davila
 */

#ifndef MY_LISA_H_
#define MY_LISA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include "jetsonGPIO.h"
#include <errno.h>
using namespace std;

#define LISA_SYNC_LEN 32
#define BUFFER_LEN 1024
#define DEBUG 0

/* LISA Algorithms */
int lisa_find_payload_binary(int confidence_lvl, unsigned char * input_buffer,
		unsigned char * lisa_bit_buffer);
int lisa_find_payload_vanilla(int confidence_lvl, unsigned char * input_buffer);
int lisa_find_payload_prob(int confidence_lvl, unsigned char * input_buffer);

/* LISA Helper Functions */
int gen_output_buffer_idx(unsigned char * buffer, int buffer_len, unsigned char * lisa_sync,
		int lisa_len, unsigned char * payload, int payload_len, int lisa_idx);
int gen_output_buffer_rand(unsigned char * buffer, int buffer_len, unsigned char * lisa_sync,
		int lisa_len, unsigned char * payload, int payload_len);
void join_lisa_payload(unsigned char ** buffer, unsigned char * lisa_sync,
		int lisa_len, unsigned char * payload, int payload_len);
uint gen_mask(uint position);
void generate_lisa_sync(int corrupt_pct, unsigned char * lisa_sync_buffer);
void generate_lisa_sync_binary(int corrupt_pct,
		unsigned char * lisa_sync_buffer, int lisa_sync_len,
		unsigned char ** lisa_bit_buffer);

void read_file_buffer(unsigned char * buffer, char * file_location);
void write_file_buffer(unsigned char * buffer, char * file_location);

void char_to_bin(unsigned char * input, int input_len, unsigned char ** result);
void bin_to_char(unsigned char * input, int input_len, char ** result);
void print_payload(int idx, unsigned char * input_buffer, int buffer_len);
void extract_payload(int idx, unsigned char * input_buffer, int buffer_len, int payload_max_len, char ** payload);

/* Misc Helper Functions */
void pct_prompt(int * input_pct, char * prompt);
time_t get_time();
time_t get_clock_time_us();

#endif /* MY_LISA_H_ */
