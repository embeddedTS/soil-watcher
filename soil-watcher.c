#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "evgpio.h"

#define PWR 76
#define LED 77
#define LVL 900


void usage() {
    printf("Usage: soil-watcher [d]\n");
    printf("    -d Daemonize the program\n\n");
}

/*******************************************************************************
* 
*******************************************************************************/
int getadc() {

    FILE *fp;
    char output[1035];
    int adcValue = -1;

    fp = popen("tshwctl --adc", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    const char s[2] = "=";
    char line[256];
    char *subString;
    char *value;

    while (fgets(output, sizeof(output)-1, fp) != NULL) {

        strcpy(line, output);
        subString = strtok(line, "=");
        
        if (strcmp(subString, "adc0") == 0) {
            value = strtok(NULL, "=");
            adcValue = atoi(value); 
        }
    }

    pclose(fp);

    return adcValue;
}

/*******************************************************************************
* 
*******************************************************************************/
void setup() {
    printf("Setting things up...\n");

    // Initialize the EVGPIO core
    printf("  >> Initialize the evgpio core\n");
    evgpioinit();

    // Set DIO pins to Output
    printf("  >> Setting PWR and LED to output (%d and %d)\n", PWR, LED);
    evsetddr(PWR, 1);
    evsetddr(LED, 1);


    // Set DIO pins to low
    printf("  >> Setting pins to low initially\n");
    evsetdata(PWR, 1);
    evsetdata(LED, 0);
}

/*******************************************************************************
* 
*******************************************************************************/
void run() {
    printf("Running application...\n");

    while(1) {

        // Turn on power to the sensor
        evsetdata(PWR, 1);

        // Read in the sensor data 
        int adcValue = getadc();

        if (adcValue <= 0) {
            printf("Ack! There was either a problem reading sensor data or \
                things are bone dry (ADC Value: %d mV)!  Flashing the LED.\n",
                adcValue);
           
            // Flash the LED to indicate there was a problem.
            evsetdata(LED, 1);
            sleep(1);
            evsetdata(LED, 0);
            sleep(1);
        }
        else if (adcValue > 0 && adcValue <= LVL) {
            printf("Moisture content is LOW (ADC Value: %d mV)!  Turning the LED on.\n", adcValue);
            evsetdata(LED, 1);
        }
        else if (adcValue > LVL) {
            printf("Moisture content is OKAY (ADC Value: %d mV)!  Turning the LED off.\n", adcValue);
            evsetdata(LED, 0);
        }

        // Turn off power to sensor to prevent oxidation of the probes
        evsetdata(PWR, 0);

        // Wait another 5 seconds before checking moisture again.
        sleep(5);
    }
}

/*******************************************************************************
* 
*******************************************************************************/
int main(int argc, char *argv[])
{
    int dflag = 0;
    int c;
    //opterr = 0;

    while ((c = getopt (argc, argv, "d")) != -1) {
        switch (c) {
            case 'd':
                dflag = 1;
                break;
            case 'h':
                usage();
                exit(1); 
            default:
                usage();
                exit(1);
        }
    }

    setup();

    // Daemonizing will run in the background with no output to stdout/stderr.
    if (dflag == 1) {
        printf("Running as daemon.\n");
        daemon(0, 0);
    }

    run();
}
