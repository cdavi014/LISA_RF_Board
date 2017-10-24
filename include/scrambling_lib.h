#ifndef SCRAMBLING_LIB_H_
#define SCRAMBLING_LIB_H_

#include <stdio.h>
#include <stdlib.h>

#define SCRMB_BUFF_LEN 64

void scramble(unsigned char * payload, int payload_len, unsigned char ** result, int order);
void descramble(unsigned char * payload, int payload_len, unsigned char ** result, int order);
int calc_large(int order);

#endif /* SCRAMBLING_LIB_H_ */
