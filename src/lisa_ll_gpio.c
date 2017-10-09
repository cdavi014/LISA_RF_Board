// exampleApp.c
/**
 *
 */
#include "lisa_lib.h"

time_t proc_start_time, proc_end_time, total_proc_time;
int corruption_pct, match_confidence, payload_idx_output, payload_idx_input;
unsigned char lisa_sync_buffer[LISA_SYNC_LEN] = { 0 };
unsigned char lisa_bit_buffer[LISA_SYNC_LEN * 8] = { 0 };
char * payload;
jetsonTX1GPIONumber RX_pin = gpio187;
unsigned char read_value;
int fid = 0, err = 0;
unsigned char read_buffer[BUFFER_LEN] = { 0 };
int h = 0;
match_confidence = 80;
int bps = 10;

int main(int argc, char *argv[]) {
	setpriority(PRIO_PROCESS, 0, -20);

	gpioExport(RX_pin);
	gpioSetDirection(RX_pin, inputPin);
	generate_lisa_sync_binary(corruption_pct, lisa_sync_buffer,
			lisa_bit_buffer);

	// Begin reading pin
	fid = gpioOpen(gpio187);

	while (1) {
		proc_start_time = get_time();
		if (h >= BUFFER_LEN) h = 0;

		err = read(fid, &read_value, 1);
		if (err == 0);
		lseek(fid, 0, SEEK_SET);

		//Store bit into specific byte on the buffer
		if (read_value == '0')
			read_buffer[h] = 0;
		else
			read_buffer[h] = 1;

		//Check to see if it matches LISA
		if (h % 64 == 0) {
			payload_idx_input = lisa_find_payload_binary(match_confidence,
					read_buffer, lisa_bit_buffer);

			if (payload_idx_input != -1) {
				printf("[SUCCESS] Payload found at index %d",
						payload_idx_input);
				print_payload(payload_idx_input, read_buffer, BUFFER_LEN);
			}
		}
		proc_end_time = get_time();
		total_proc_time = proc_end_time - proc_start_time;
		//if(h%64 == 0)
		cout << "  Total Proc time: " << total_proc_time << endl;
		cout << "  Total Sleep time: " << 1000000 / bps - total_proc_time
				<< endl;
		h++;
		// sleep microseconds
		cout << "  Start sleep: " << get_time() << endl;
		usleep(1000000 / bps - total_proc_time);
		cout << "  End sleep: " << get_time() << endl;
	}
	return 0;
}

