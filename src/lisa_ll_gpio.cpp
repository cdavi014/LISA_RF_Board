// exampleApp.c
#include "my_lisa.h"
#include <stdio.h>
#include <stdlib.h>
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
using namespace std;

int open_gpio_pin(jetsonGPIO pin) {
    int fileDescriptor;
    char commandBuffer[MAX_BUF];

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR "/gpio%d/value", pin);
    fileDescriptor = open(commandBuffer, O_RDWR);

    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioGetValue unable to open gpio%d",pin) ;
        perror(errorBuffer);
        return -1;
    }
    return fileDescriptor;
} 

int gpio_read_value(int file_descriptor) {
    char rd;
    int ret = read(file_descriptor, &rd, 1);
    if (ret != 1) {
       cout << "Error: " << ret << endl;
       perror("gpioGetValue") ;
       return -1;
    }

    if (rd == '0') {
        return 1;
    } else {
        return 0;
    }
}

time_t get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec;
}

time_t proc_start_time, proc_end_time, total_proc_time;
int corruption_pct, match_confidence, payload_idx_output, payload_idx_input;
unsigned char lisa_sync_buffer [LISA_SYNC_LEN] = {0};	
unsigned char lisa_bit_buffer [LISA_SYNC_LEN * 8] = {0};	
char * payload;

int main(int argc, char *argv[]){
    setpriority(PRIO_PROCESS, 0, -20);
    cout << "Reading LISA Input" << endl;

    jetsonTX1GPIONumber RX_pin = gpio187;     // Input
    gpioExport(RX_pin);
    gpioSetDirection(RX_pin, inputPin);
    generate_lisa_sync_binary(corruption_pct, lisa_sync_buffer, lisa_bit_buffer);
	for(int i = 0; i < 32; i++) {
		printf("LSB[%d]: %u, ", i, lisa_sync_buffer[i]);
		printf("LBB[%d]: %u\n", i, lisa_bit_buffer[i]);
	}
	//exit(0);
    // Begin reading file
    unsigned char read_value;
    int fid = 0, err = 0;
    unsigned char read_buffer [BUFFER_LEN] = {0};
    int h = 0;
    match_confidence = 80;
    int bps = 10;

	fid = open("/sys/class/gpio/gpio187/value", O_RDONLY);
    while(1) {
	proc_start_time = get_time();
	if(h >= BUFFER_LEN)
		h = 0;
	err = read(fid, &read_value, 1);
	if(err == 0);
	lseek(fid, 0, SEEK_SET);

	//Store bit into specific byte on the buffer
	if(read_value == '0')
		read_buffer[h] = 0;
	else 
		read_buffer[h] = 1;

	//Check to see if it matches LISA
	if(h%64 == 0) {

		payload_idx_input = lisa_find_payload_binary(match_confidence, 
					read_buffer, lisa_bit_buffer);

		if(payload_idx_input != -1) {
			printf("[SUCCESS] Payload found at index %d",
				payload_idx_input); 
			print_payload(payload_idx_input, read_buffer, BUFFER_LEN);
		}
	}
	proc_end_time =  get_time();
	total_proc_time = proc_end_time - proc_start_time;
	//if(h%64 == 0)
		cout << "  Total Proc time: " << total_proc_time << endl;
		cout << "  Total Sleep time: " << 1000000 / bps - total_proc_time << endl;
	h++;
	// sleep microseconds
	cout << "  Start sleep: " << get_time() << endl;
	usleep(1000000 / bps - total_proc_time); 
	cout << "  End sleep: " << get_time() << endl;
    }
    return 0;
}


