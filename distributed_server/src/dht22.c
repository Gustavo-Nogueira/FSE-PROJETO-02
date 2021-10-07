/*
 *  dht.c:
 *      Author: Juergen Wolf-Hofer
 *      based on / adapted from http://www.uugear.com/portfolio/read-dht1122-temperature-humidity-sensor-from-raspberry-pi/
 *	reads temperature and humidity from DHT11 or DHT22 sensor and outputs according to selected mode
 *
 *  Change History:
 *      Variable and function name update ​​to resolve possible name conflicts.
 */

#include "dht22.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

// CONSTANTS
#define MAX_TIMINGS 85
#define DHT_DEBUG 0
#define WAIT_TIME 2000
#define DHT_PIN 28  // default GPIO 20 (wiringPi 28)

// GLOBAL VARIABLES
static int dht_data[5] = {0, 0, 0, 0, 0};
static float dht_temp_cels = -1;
static float dht_temp_fahr = -1;
static float dht_humidity = -1;

// FUNCTION DECLARATIONS
static int read_dht_data();

// FUNCTION DEFINITIONS
int init_dht() {
    if (wiringPiSetup() == -1) {
        return 1;
    }
    return 0;
}

int get_dht_data(float *temp_cels, float *temp_fahr, float *humidity) {
    if (read_dht_data()) return 1;
    *temp_cels = dht_temp_cels;
    *temp_fahr = dht_temp_fahr;
    *humidity = dht_humidity;
    return 0;
}

static int read_dht_data() {
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0;
    uint8_t i;

    dht_data[0] = dht_data[1] = dht_data[2] = dht_data[3] = dht_data[4] = 0;

    /* pull pin down for 18 milliseconds */
    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);

    /* prepare to read the pin */
    pinMode(DHT_PIN, INPUT);

    /* detect change and read data */
    for (i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (digitalRead(DHT_PIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        laststate = digitalRead(DHT_PIN);

        if (counter == 255)
            break;

        /* ignore first 3 transitions */
        if ((i >= 4) && (i % 2 == 0)) {
            /* shove each bit into the storage bytes */
            dht_data[j / 8] <<= 1;
            if (counter > 16)
                dht_data[j / 8] |= 1;
            j++;
        }
    }

    /*
	 * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
	 * print it out if data is good
	 */
    if ((j >= 40) && (dht_data[4] == ((dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF))) {
        float h = (float)((dht_data[0] << 8) + dht_data[1]) / 10;
        if (h > 100) {
            h = dht_data[0];  // for DHT11
        }
        float c = (float)(((dht_data[2] & 0x7F) << 8) + dht_data[3]) / 10;
        if (c > 125) {
            c = dht_data[2];  // for DHT11
        }
        if (dht_data[2] & 0x80) {
            c = -c;
        }
        dht_temp_cels = c;
        dht_temp_fahr = c * 1.8f + 32;
        dht_humidity = h;
        if (DHT_DEBUG) printf("read_dht_data() Humidity = %.1f %% Temperature = %.1f *C (%.1f *F)\n", dht_humidity, dht_temp_cels, dht_temp_fahr);
        return 0;  // OK
    } else {
        if (DHT_DEBUG) printf("read_dht_data() Data not good, skip\n");
        dht_temp_cels = dht_temp_fahr = dht_humidity = -1;
        return 1;  // NOK
    }
}
