#include "Header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include "i2c-dev.h"

int loop=1;


void signal_handler(int sig)
{
	printf( "Ctrl-C pressed, cleaning up and exiting..\n" );
	loop = 0;
}

int main(int argc, char** argv){
	
	//variable declarations
	struct pollfd fdset[1];
	int nfds = 1;
	int timeout = 3000;
	int rc;
	char* buf[MAX_BUF];	
	int gpio1, gpio2;
	int gpio1_fd, gpio2_fd;
	int gpio2_value = 0;
	int pattern =0;
	int value =0;
	char *end;
	int res, i2cbus, address, size, file;
	int daddress;
	char filename[20];
	

	//Setting the signal handler for Ctrl + C
	if (signal(SIGINT, signal_handler) == SIG_ERR)
		printf("\ncan't catch SIGINT\n");


	//Argument checking for at least five
	if(argc < 6){
		printf("Usage: %s <input-gpio> <output-gpio> <i2c-bus> <i2c-address> <register>\n", argv[0]);
		printf("polls input-gpio, and writes value to output-gpio\n");
		fflush(stdout);
		return 1;
	}

	//Assigning gpio values
	gpio1 = atoi(argv[1]);
	gpio2 = atoi(argv[2]);

	//Input for Argument 1
	export_gpio(gpio1);
	set_gpio_direction(gpio1, "in");
	set_gpio_edge(gpio1, "falling");
	gpio1_fd = gpio_fd_open(gpio1);
	
	//Output for Argument 2
	export_gpio(gpio2);
	set_gpio_direction(gpio2, "out");
	set_gpio_value(gpio2, gpio2_value);
	gpio2_fd = gpio_fd_open(gpio2);

	//Assigning I2C values
	i2cbus   = atoi(argv[3]);
	address  = atoi(argv[4]);
	daddress = atoi(argv[5]);
	size = I2C_SMBUS_BYTE;

	sprintf(filename, "/dev/i2c-%d", i2cbus);
	file = open(filename, O_RDWR);
	if (file<0) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file "
				"/dev/i2c-%d: %s\n", i2cbus, strerror(ENOENT));
		} else {
			fprintf(stderr, "Error: Could not open file "
				"`%s': %s\n", filename, strerror(errno));
			if (errno == EACCES)
				fprintf(stderr, "Run as root?\n");
		}
		exit(1);
	}

	if (ioctl(file, I2C_SLAVE, address) < 0) {
			fprintf(stderr,
				"Error: Could not set address to 0x%02x: %s\n",
				address, strerror(errno));
			return -errno;
	}
	

	while(loop){
		memset((void*)fdset, 0, sizeof(fdset));
	
		fdset[0].fd = gpio1_fd;
		fdset[0].events = POLLPRI;

		rc = poll(fdset, nfds, timeout);

		if (rc < 0){
			printf("\npoll() failed!\n");
		}
	
		if (rc == 0){
			printf(".");
		}

		if((fdset[0].revents & POLLPRI) == POLLPRI) {
			read(fdset[0].fd, buf, MAX_BUF);
			printf("interrupt value=%c\n", buf[0]);
			pattern++;
		if(pattern == 4){
		pattern = 0;
		}
		}			
		
	


	printf("Case %d\n",pattern);
	
	switch(pattern){
	
	// Blink led 
	case 0:
		if(gpio2_value){
		gpio2_value = 0;
		}
		else{
		gpio2_value = 1;
		}
		set_gpio_value(gpio2, gpio2_value);
	break;
	// PWM output for blinking LED by frequency and duty cycle  	
	case 1:	 
		set_mux_value("gpmc_a2",6);
		set_pwm("ehrpwm.1:0",10,25);
	break;
	// Analog In for reading Voltage from POT
	case 2: unset_pwm("ehrpwm.1:0");
		value = read_ain("ain6");
		printf("Voltage: %d\n",value);
	break;	
	
	// I2C Device reading temperature from the sensor
	case 3:
		printf("Case 3\n");
		res = i2c_smbus_write_byte(file, daddress);
		if (res < 0) {
			fprintf(stderr, "Warning - write failed, filename=%s, daddress=%d\n",
				filename, daddress);
		}
		res = i2c_smbus_read_byte_data(file, daddress);
		if (res < 0) {
			fprintf(stderr, "Error: Read failed, res=%d\n", res);
			exit(2);
		}

		printf("Temperature: %d°C\n", res);
		break;
	default:
		break;

	}
	}
	
	close(file);
	gpio_fd_close(gpio1_fd);
	gpio_fd_close(gpio2_fd);
	unexport_gpio(gpio1);
	unexport_gpio(gpio2);
	fflush(stdout);
	return 0;
}
