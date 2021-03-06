/*
 * lisa_ll_gpio.c
 *
 *  Created on: Oct 10, 2017
 *      Author: Carlos R. Davila
 *
 *  Status: Working Rx.
 *  TODO:		Need to implement Tx, Scrambling and SIGINT handling
 */
#include "lisa_lib.h"
#include "scrambling_lib.h"
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#define MODE 'T'	// T - Transmit, R - Receive

jetsonTX1GPIONumber RX_pin = gpio63;
jetsonTX1GPIONumber TX_pin = gpio186;

long proc_start_time, proc_end_time, proc_time_clock;
long sleep_start_time, sleep_end_time, sleep_time_clock = 0;
int corruption_pct, payload_idx_output, payload_idx_input;
unsigned char lisa_sync_buffer[LISA_SYNC_LEN] = { 0 };
unsigned char * lisa_bit_buffer;
const char * payload = "Carlos_Davila_SJSU_010779067";
unsigned char read_value;
int fid = 0, err = 0;
unsigned char read_buffer[BUFFER_LEN] = { 0 };
int h = 0;
int match_confidence = 60;
int bps = 1000;
int skew = 0; // Need to account for skew from sending device. Ideally this is not needed.
struct sigaction sa;
struct itimerval timer;
unsigned char * tx_buffer;
int tx_buffer_len;
int scramble_order = 5;

/**
 * poll_edge is used to trigger the collection process. It polls an interrupt coming from the GPIO
 * pin. Once the trigger comes in, then we set the timer. If we did not do this, then the timer.
 *
 */
int poll_edge(int fid) {
	printf("\nPolling edge...\n");
	int timeout = -1; /* INF seconds */
	char buff[8];
	int rc = 0;
	int NFDS = 2;	// Number of items in the fds array
	struct pollfd fdset[NFDS];

	memset((void*) fdset, 0, sizeof(fdset));

	fdset[0].fd = STDIN_FILENO;
	fdset[0].events = POLLERR;

	fdset[1].fd = fid;
	fdset[1].events = POLLPRI;

	/* Consume any prior interrupt */
	lseek(fid, 0, SEEK_SET);
	read(fid, buff, sizeof(buff));

	while (1) {
		/* Blocking call to poll */
		rc = poll(fdset, NFDS, timeout);
		//printf("\nRC returned: %d\n", rc);

		if (rc < 0) {
			printf("\n[ERROR] poll() failed()\n");
			return -1;
		} else if (rc == 0) {
			printf(".");
		}

		if (fdset[1].revents & POLLPRI) {
			//printf("\nGPIO POLLPRI Event\n");
			return 0;
		}

		if (fdset[0].revents & POLLERR) {
			//printf("\nGPIO POLLERR Event\n");
			return 0;
		}
		fflush(stdout);
	}
	return 0;
}

/**
 * Read the GPIO pin and check for LISA input
 */
void read_gpio() { //TODO: fid should be a parameter
	char * payload_found;
	unsigned char * descrambled_result;

	if (h >= BUFFER_LEN)
		h = 0;
	lseek(fid, 0, SEEK_SET);
	err = read(fid, &read_value, 1);

	if (err != 1)
		printf("READ ERROR [%d]!!!", err);

	//Store bit into specific byte on the buffer
	if (read_value == '0')
		read_buffer[h] = (unsigned int) 0;
	else if (read_value == '1')
		read_buffer[h] = (unsigned int) 1;
	//cout << get_clock_time_us() << "," << h << "," << (int) read_buffer[h] <<endl;

	//Check to see if it matches LISA
	if (h % 128 == 0) {
		proc_start_time = get_clock_time_us();
		payload_idx_input = lisa_find_payload_binary(match_confidence, read_buffer,
				lisa_bit_buffer);
		// extract payload
		extract_payload(payload_idx_input, read_buffer, BUFFER_LEN, 256, &payload_found);

		// descramble payload
		descramble((unsigned char *) payload_found, 256, &descrambled_result, scramble_order);

		if (payload_idx_input != -1) {
			printf("[SUCCESS] Payload found at index %d", payload_idx_input);
			print_payload(payload_idx_input, read_buffer, BUFFER_LEN);
		}
		proc_end_time = get_clock_time_us();
	}
	read_value = 'z';
	h++;
	fflush(stdout);
}

void pause_timer(struct itimerval * timer) {
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, timer, NULL);
}

void resume_timer(struct itimerval * timer) {
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 1000000 / bps;
	setitimer(ITIMER_REAL, timer, NULL);
}

/**
 * Send LISA + payload
 */
void send_gpio() {
	static int buff_idx = 0;
	gpioSetValue(TX_pin, tx_buffer[buff_idx++]);
	printf("%d", tx_buffer[buff_idx]);

	if(buff_idx >= tx_buffer_len) {
		printf("\n\nReset!!\n\n");
		buff_idx = 0;
		pause_timer(&timer);
		// Sleep for 1 second
		sleep(1);
		// Restart the timer
		resume_timer(&timer);
	}
}

/**
 * Call back to handle the timeput of the timer.
 */
void gpio_rx_handler(int signum) {
	static int count = 0;

	count++;
	read_gpio();
}

/**
 * Call back to handle the timeput of the timer.
 */
void gpio_tx_handler(int signum) {
	static int count = 0;
	// Setup a buffer globably with the LISA + Scrambled payload
	// Use a pointer to continuously send data out
	count++;
	send_gpio();
}

/**
 * Timer that will expire to sample the GPIO pin.
 * Added 'skew' to account for any skew that the Tx device has.
 */
void setup_timer(struct itimerval * timer, struct sigaction * sa) {
	int ret = 0;

	memset(sa, 0, sizeof(*sa));
	if (MODE == 'R')
		sa->sa_handler = &gpio_rx_handler;
	else
		sa->sa_handler = &gpio_tx_handler;

	ret = sigaction(SIGALRM, sa, NULL);
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 1000000 / bps;
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 1000000 / bps + skew;
}

void print_rf_config() {
	printf("Pin configuration for RF board v1.0. Please make sure"
			" to set up your the RF board for the TX1 with the correct"
			" pin configuration:\n\n"
			" ------------------------------------\n"
			"|  Config  | Pins En. | Pin Func     |\n"
			"| LL_Rx_Tx |   1, 2   | 1-Tx   2-Rx  |\n"
			"|  RF_RX   |   3, 4   | 3-Data 4-Vcc |\n"
			"|  RF_TX   |   5, 6   | 5-Data 6-Vcc |\n"
			"|  MISC    |   7, 8   | 7-xx   8-xx  |\n"
			" ------------------------------------\n");
}

void rx_data() {
	// Set up GPIO Handler
	print_rf_config();
	sleep(5);

	printf("\nSetting up timer...\n");
	setup_timer(&timer, &sa);

	// Set up GPIO
	printf("\nSetting up GPIO Pins for Rx...\n");
	gpioExport(RX_pin);
	gpioSetDirection(RX_pin, inputPin);
	gpioSetEdge(RX_pin, "rising");
	fid = gpioOpen(RX_pin);

	// Wait for first edge to align the clock
	int ret = poll_edge(fid);
	if (ret < 0) {
		printf("[ERROR] An error occurred when polling for edge.");
		return;
	}

	// Start timer after a small offset from trigger
	usleep(((1.0 / bps) * 1000000) * .2);
	setitimer(ITIMER_REAL, &timer, NULL);
	printf("\nDone setting up Rx!\n");

	while (1)
		; // Run indefinetly
}

void tx_data() {
	// Set up GPIO Handler
	print_rf_config();
	sleep(5);

	printf("\nSetting up timer...\n");
	setup_timer(&timer, &sa);

	// Set up GPIO
	printf("\nSetting up GPIO Pins for Tx...\n");
	gpioExport(TX_pin);
	gpioSetDirection(TX_pin, outputPin);

	setitimer(ITIMER_REAL, &timer, NULL);
	printf("\nDone setting up Tx!\n");

	while (1)
		;
}

int main(int argc, char *argv[]) {
	setpriority(PRIO_PROCESS, 0, -20);

	// Generate LISA sync
	generate_lisa_sync_binary(0, lisa_sync_buffer, LISA_SYNC_LEN,
			&lisa_bit_buffer);

	if (MODE == 'R') {
		rx_data();
	} else {
		unsigned char * payload_bit_buffer;
		unsigned char * scrambled_payload;

		// Convert payload to binary
		char_to_bin((unsigned char *) payload, strlen(payload),
				&payload_bit_buffer);

		// Scramble Payload
		scramble(payload_bit_buffer, strlen(payload) * 8, &scrambled_payload, scramble_order);

		// Join LISA + Payload
		join_lisa_payload(&tx_buffer, lisa_bit_buffer,
				LISA_SYNC_LEN * 8, scrambled_payload, strlen(payload) * 8);

		tx_buffer_len = LISA_SYNC_LEN * 8 + strlen(payload) * 8;
		// Send Buffer
		tx_data();
	}
	return 0;
}

