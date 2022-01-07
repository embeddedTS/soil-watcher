/*******************************************************************************
* soil-watcher:
*
*    For green-thumb enthusiasts, this program will run forever, checking soil
*    moisture content every so often.  If the moisture content is low, an red LED
*    will be turned on.  If the moisture content is okay, the red LED will be 
*    turned off.  The gardener will see a red LED and know the plant needs water.
*
*    This was developed on a TS-7250-V2.  Connections are as follows:
*        - Red LED:              DIO_03, GPIO #77
*        - Power Output:         DIO_01, GPIO #76
*        - Soil Moisture Sensor: A/D_01, ADC #0
*
*    The soil moisture sensor acts as a variable resistor and at 3.3 VDC, it
*    will read between 0 mV (dry) and 2500 mV (soaked).  When sticking it into 
*    plant soil, it seemed a reasonable threshold level was around 900 mV.
*
*    Reading the ADC values on the A/D header is done using `tshwctl --adc`.
*    Controlling the DIO outputs is done using the EVGPIO core (evgpio.c).
*
*    Compile using the `make` command (recommended), or manually using:
*        gcc -o soil-watcher soil-watcher.c evgpio.c  -mcpu=arm9
*
*    This program can be ran normally, with output to console, simply running:
*        ./soil-watcher
*
*    Or, it can be daemonized using the -d option, like so:
*        ./soil-watcher -d
*
*    For running this application on startup, check out instructions given here:
*        https://wiki.embeddedTS.com/wiki/TS-7250-V2#Starting_Automatically
* 
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "evgpio.h"

#define PWR 76  // GPIO pin for power enable to moisture sensor (vcc)
#define LED 77  // GPIO pin for indicator LED
#define LVL 900 // Threshold level for moisture before water is needed

/*******************************************************************************
* Simply prints out the usage of the program.  Only one option, deamonize (-d).
*******************************************************************************/
void usage() {
    printf("Usage: soil-watcher [d]\n");
    printf("    -d Daemonize the program\n\n");
}

/*******************************************************************************
* Runs system command `tshwctl --adc`, loops through the output, and extracts
* the ADC value from adc0 in mV.  Returns ADC value.
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
* Setup the system by initializing EVGPIO core and turn off power and red LED.
* This is run only once.
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
* Run the program in a forever loop in 10 second intervals, turning on the power
* to the moisture sensor, reading in the ADC value, and either turning the red
* LED on or off, depending on moisture content in mV.
*******************************************************************************/
void run() {
    printf("Running application...\n");

    while(1) {

        int adcValue;
 
        // Turn on power to the sensor
        evsetdata(PWR, 1);

        // Read in the sensor data 
        adcValue = getadc();

        if (adcValue <= 10) {

            printf("Ack! There was either a problem reading sensor data or things are bone dry (ADC Value: %d mV)!  Flashing the LED.\n",
                adcValue);

            // Flash the LED to indicate there was a problem or bone dry.
            // There are better ways to do this, but for now, this works.
            while(getadc() <= 10) {
                evsetdata(PWR, 0);

                evsetdata(LED, 1);
                usleep(100000);
                evsetdata(LED, 0);
                usleep(100000);

                evsetdata(PWR, 1);
            }
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

        // Wait another 10 seconds before checking moisture again.
        sleep(10);
    }
}

/*******************************************************************************
* Main.  Where it all begins.  Parses command line arguments, runs setup() and
* then run().
*******************************************************************************/
int main(int argc, char *argv[])
{
    int dflag = 0;
    int c;

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
