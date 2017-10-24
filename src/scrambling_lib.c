#include "scrambling_lib.h"

/**
 * scramble() will take a payload buffer and scramble it to the 'order' provided.
 * The 'order' provided must be an odd number greater than or equal to 3. The result will be
 * placed in the buffer pointer provided by 'result'. The basics of the algorithm is described here:
 *
 * There are 3 pointers that circle around a circular buffer i, p and pp.
 * i  - Always points to the incoming bit. That is, i points to the location in the buffer representing the first
 * 		  incoming bit into the nth order scrambling/descrambling unit.
 * p  - Always points to the bit that is to be XORd at the middle of the scrambling/descrambling unit. So in a
 *      5th order system, p always points to the 3rd bit (3rd delay slot) that is to be XORd with the 5th delay slot.
 * pp - Always points to the bit that is to be XORd with p. In a 5th ordered system, pp is the 5th delay slot.
 * 			pp happens to be the same as i because of the way the circular buffer works out. One important distinction is
 * 			that pp is the value of the buffer BEFORE i is updated. Technically this pointer is not needed, just kept for
 * 			clarity.
 *
 * Alg. steps:
 *		1) Retrieve bit from payload buffer (MSB first)
 *		2) XOR new bit with p and pp
 *		3) Store result in output and in i
 *		4) Recalculate the pointers in the buffer
 *
 * @param payload 		-	The pointer to the payload bits (data assumed to be in unsigned chars)
 * @param payload_len -	The number of bits of the payload provided
 * @param result 			- Where the result will be stored
 * @param order 			- The order to which to scramble the payload (Min -> 3, Max -> No theoretical limit)
 */
void scramble(unsigned char * payload, int payload_len, unsigned char ** result,
		int order) {
	int PP = calc_large(order);
	int P = PP - 1;

	if (order % 2 == 0 || order < 3) {
		printf("[ERROR] Scrambling order must be an odd number >= 3\n");
		return;
	}

	unsigned char * scramble_buff = (unsigned char *) calloc(order,
			sizeof(unsigned char));
	*result = (unsigned char *) calloc(payload_len, sizeof(unsigned char));

	int i, p, pp; // p - idx for first part of scrambling, pp - second part idx
	unsigned char curr;
	for (int n = 0; n < payload_len; n++) {
		i = pp = n % order;
		p = (i + PP - 1) % order;

		// Get new bit
		curr = payload[n];

		// Scramlble and save
		scramble_buff[i] = (*result)[n] = curr
				^ (scramble_buff[p] ^ scramble_buff[pp]);
	}
	free(scramble_buff);
}

/**
 * descramble() will take a scrambled payload buffer and descramble it to the 'order' provided.
 * The 'order' provided must be an odd number greater than or equal to 3. The result will be
 * placed in the buffer pointer provided by 'result'. The basics of the algorithm is described here:
 *
 * There are 3 pointers that circle around a circular buffer i, p and pp.
 * i  - Always points to the incoming bit. That is, i points to the location in the buffer representing the first
 * 		  incoming bit into the nth order scrambling/descrambling unit.
 * p  - Always points to the bit that is to be XORd at the middle of the scrambling/descrambling unit. So in a
 *      5th order system, p always points to the 3rd bit (3rd delay slot) that is to be XORd with the 5th delay slot.
 * pp - Always points to the bit that is to be XORd with p. In a 5th ordered system, pp is the 5th delay slot.
 * 			pp happens to be the same as i because of the way the circular buffer works out. One important distinction is
 * 			that pp is the value of the buffer BEFORE i is updated. Technically this pointer is not needed, just kept for
 * 			clarity.
 *
 * Alg. steps:
 *		1) Retrieve bit from scrambled payload buffer (MSB first)
 *		2) XOR new bit with p and pp
 *		3) Store result in output result buffer
 *		4) Store new bit at place pointed by i
 *		5) Recalculate the pointers in the buffer
 *
 * @param payload 		-	The pointer to the payload bits (data assumed to be in unsigned chars)
 * @param payload_len -	The number of bits of the payload provided
 * @param result 			- Where the result will be stored
 * @param order 			- The order to which to descramble the payload (Min -> 3, Max -> No theoretical limit)
 */
void descramble(unsigned char * payload, int payload_len,
		unsigned char ** result, int order) {
	int PP = calc_large(order);
	int P = PP - 1;

	if (order % 2 == 0 || order < 3) {
		printf("[ERROR] Scrambling order must be an odd number >= 3\n");
		return;
	}

	unsigned char * descramble_buff = (unsigned char *) calloc(order,
			sizeof(unsigned char));
	*result = (unsigned char *) calloc(payload_len, sizeof(unsigned char));

	int i, p, pp; // p - idx for first part of scrambling, pp - second part idx
	unsigned char curr;
	for (int n = 0; n < payload_len; n++) {
		i = pp = n % order;
		p = (i + PP - 1) % order;

		// Get new byte
		curr = payload[n];

		// De-scramlble and save
		 (*result)[n] = curr ^ (descramble_buff[p] ^ descramble_buff[pp]);
		 descramble_buff[i] = curr;
	}
	free(descramble_buff);
}

int calc_large(int order) {
	return (int) (.5 * (order + 1));
}
