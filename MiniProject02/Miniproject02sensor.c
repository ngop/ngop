/****************************************************************
****************************************************************/
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>


/****************************************************************
* Constants
****************************************************************/
#define MAX_BUF 64

/****************************************************************
* Global Variables
****************************************************************/
int keepgoing = 1;

void signal_handler(int sig)
{
	printf( "Ctrl-C pressed, cleaning up and exiting..\n" );
	keepgoing = 0;
}
	
/****************************************************************
 * read_ain
 ****************************************************************/
int read_ain(char* ain){
	FILE *fp;
	char ainPath[MAX_BUF];
	char ainRead[MAX_BUF];

	snprintf(ainPath, sizeof ainPath, "/sys/devices/platform/omap/tsc/%s", ain);

	if((fp = fopen(ainPath, "r")) == NULL){
		printf("Cannot open specified ain pin, %s\n", ain);
		return 1;
	}

	if(fgets(ainRead, MAX_BUF, fp) == NULL){
		printf("Cannot read specified ain pin, %s\n", ain);
	}
	
	fclose(fp);
	return atoi(ainRead);	
}

/****************************************************************
 * Main
 ****************************************************************/
int main(int argc, char **argv){

	int ainPin;
	char ain[MAX_BUF];
	float duty_cycle = 0;
	float avgDutyCycle = 0;
	int avgCount = 1;
	
	if (argc < 2){
		printf("Usage: ./MiniProject02 <ainpin>");
		exit(-1);
	}

	signal(SIGINT, signal_handler);
	ainPin = atoi(argv[1]);
	snprintf(ain, sizeof ain, "ain%d", ainPin);

	while (keepgoing) {
		usleep(100000);
		duty_cycle = read_ain(ain);
		/*duty_cycle = duty_cycle/4095 * 100;
		duty_cycle = 30 - duty_cycle;
		duty_cycle = duty_cycle*4;
		duty_cycle = duty_cycle - 28.6;
		if(duty_cycle <= 6) duty_cycle = 0;*/
		//avgDutyCycle += duty_cycle;
		/*if (avgCount == 250) {
			printf("Calculating.  \r");
			fflush(stdout);
		}
		if (avgCount == 500) {
			printf("Calculating . \r");
			fflush(stdout);
		}
		if (avgCount == 750) {
			printf("Calculating  .\r");
			fflush(stdout);
		}*/
		//if(avgCount >= 1000){
			//avgDutyCycle = avgDutyCycle/avgCount;
			//avgDutyCycle = avgDutyCycle * 100;
			printf("                                 \r");
			printf("Value:  %d\r\n",(int)duty_cycle);
			//if(avgDutyCycle >= 9000) printf("Calculating    It's over 9000!!!!\r");
			fflush(stdout);
			//avgDutyCycle = 0;
			//avgCount = 0;
		//}
		//avgCount++;
	}
	
	fflush(stdout);
	return 0;
}


