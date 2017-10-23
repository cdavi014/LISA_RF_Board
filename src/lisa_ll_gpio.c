// exampleApp.c
/**
 *
 */
#include "lisa_lib.h"
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#define NFDS 2

long proc_start_time, proc_end_time, proc_time_clock;
long sleep_start_time, sleep_end_time, sleep_time_clock = 0;
int corruption_pct, payload_idx_output, payload_idx_input;
unsigned char lisa_sync_buffer[LISA_SYNC_LEN] = { 0 };
unsigned char lisa_bit_buffer[LISA_SYNC_LEN * 8] = { 0 };
char * payload;
jetsonTX1GPIONumber RX_pin = gpio63;
unsigned char read_value;
int fid = 0, err = 0;
unsigned char read_buffer[BUFFER_LEN] = { 0 };
int h = 0;
int match_confidence = 60;
int bps = 1000;
int skew = 15;
struct sigaction sa;
struct itimerval timer;

int poll_edge(int fid) {
	printf("\nPolling edge...\n");
	int timeout = -1; /* INF seconds */
	char buff[8];
	int rc = 0;
	struct pollfd fdset[NFDS];

	memset((void*)fdset, 0, sizeof(fdset));

	fdset[0].fd = STDIN_FILENO;
	fdset[0].events = POLLERR;

	fdset[1].fd = fid;
	fdset[1].events = POLLPRI;

	/* Consume any prior interrupt */
	lseek(fid, 0, SEEK_SET);
	read(fid, buff, sizeof(buff));

	while(1) {
		/* Blocking call to poll */
		rc = poll(fdset, NFDS, timeout);
		//printf("\nRC returned: %d\n", rc);
	
		if(rc < 0) {
			printf("\n[ERROR] poll() failed()\n");
			return -1;
		} else if (rc == 0) {
			printf(".");
		}

		if(fdset[1].revents & POLLPRI) {
			//printf("\nGPIO POLLPRI Event\n");
			return 0;
		}

		if(fdset[0].revents & POLLERR) {
			//printf("\nGPIO POLLERR Event\n");
			return 0;
		}

		fflush(stdout);
	}
	return 0;
}


void read_gpio() {
  if (h >= BUFFER_LEN) h = 0;
  lseek(fid, 0, SEEK_SET);
  err = read(fid, &read_value, 1);

if(err != 1) printf("READ ERROR [%d]!!!", err);

  //Store bit into specific byte on the buffer
  if (read_value == '0')
    read_buffer[h] = (unsigned int) 0;
  else if (read_value == '1')
    read_buffer[h] = (unsigned int) 1;
	else
		printf("\n\n\nWTF********\n\n\n");

  //cout << get_clock_time_us() << "," << h << "," << (int) read_buffer[h] <<endl;

  //Check to see if it matches LISA
  if (h % 128 == 0) {
		proc_start_time = get_clock_time_us();
    payload_idx_input = lisa_find_payload_binary(match_confidence,
			read_buffer, lisa_bit_buffer);

    if (payload_idx_input != -1) {
      printf("[SUCCESS] Payload found at index %d",
				payload_idx_input);
      print_payload(payload_idx_input, read_buffer, BUFFER_LEN);
    }
		proc_end_time = get_clock_time_us();
  }
	read_value = 'z';
  h++;
  fflush(stdout);
}

void gpio_handler(int signum) {
  static int count = 0;

  count++;
  //printf("%d,%d,%ld\n",count, h, get_clock_time_us());
  read_gpio();
  //fflush(stdout);
}

void setup_timer(struct itimerval * timer, struct sigaction * sa) {
  int ret = 0;

	memset(sa, 0, sizeof(*sa));
  sa->sa_handler = &gpio_handler;
  ret = sigaction(SIGALRM, sa, NULL);
  //printf("returned r= %d", ret);
  timer->it_value.tv_sec = 0;
  timer->it_value.tv_usec = 1000000 / bps;
  timer->it_interval.tv_sec = 0;
  timer->it_interval.tv_usec = 1000000 / bps + skew;
}

int main(int argc, char *argv[]) {
	int ret = 0;
  setpriority(PRIO_PROCESS, 0, -20);
  printf("\nStart setting up!\n");

  // Set up GPIO Handler
	setup_timer(&timer, &sa);

  // Set up GPIO
  gpioExport(RX_pin);
  gpioSetDirection(RX_pin, inputPin);
	gpioSetEdge(RX_pin, "rising");
  fid = gpioOpen(RX_pin);

  generate_lisa_sync_binary(0, lisa_sync_buffer,
		lisa_bit_buffer);

	// Wait for first edge to align the clock
	ret = poll_edge(fid);
	if(ret < 0) {
		return ret;
	}
  // Start timer
	usleep(((1.0 / bps) * 1000000) * .2);
  setitimer(ITIMER_REAL, &timer, NULL);
  printf("\nDone setting up!\n");

  while (1) {
		/**
		proc_start_time = get_clock_time_us();
		if (h >= BUFFER_LEN) h = 0;

		err = read(fid, &read_value, 1);
		lseek(fid, 0, SEEK_SET);

		//Store bit into specific byte on the buffer
		if (read_value == '0')
			read_buffer[h] = (unsigned int) 0;
		else
			read_buffer[h] = (unsigned int) 1;
		cout << h << "," << (int) read_buffer[h] <<endl;

		//cout << get_clock_time_us() <<endl;
		//Check to see if it matches LISA
		if (h % 1024 == 0) {
			payload_idx_input = lisa_find_payload_binary(match_confidence,
					read_buffer, lisa_bit_buffer);

			if (payload_idx_input != -1) {
				printf("[SUCCESS] Payload found at index %d",
						payload_idx_input);
				print_payload(payload_idx_input, read_buffer, BUFFER_LEN * 8);
			}
		}
		//proc_end_time = get_clock_time_us();
		proc_time_clock = get_clock_time_us() - proc_start_time;
		h++;
		// sleep microseconds
		sleep_start_time = get_clock_time_us();
		usleep(1000000 / bps - proc_time_clock + sleep_time_clock);
		sleep_time_clock = 1000000 / bps - (get_clock_time_us() - sleep_start_time);
		//cout << sleep_time_clock << endl;
		//fflush(stdout);
		*/
	}
	return 0;
}

