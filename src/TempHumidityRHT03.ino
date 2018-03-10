/*
 * Project TempHumidityRHT03
 * Temperature and Humidity monitor (RHT03 sensor)
 * Retrieves outside temperature from darksky.net
 * Retrieves UV Index from OpenUV.io
 * Author: Richard Lyon
 * Date: 27 December 2017
 * Version: 0.1
*
* Device: kingswood_one
 */

#include "authenticate.h" // InfluxDB credentials
#include "InfluxDB.h"
#include <SparkFunRHT03.h>

InfluxDB idb = InfluxDB(USERNAME, PASSWORD);
RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor

// RHT03 sensor data
double temperature;
double humidity;

// Dark Sky Data
double cloud_cover;
double humidity_outside;
double temperature_outside;
double uvindex = 0;
double transmission = 0;
double uvindex_corrected;
String data = String(10);

int delay_millis_from_sample_rate(String sample_rate);
String sample_rate_str;
int delay_millis; // millis corresponding to sample_rate to delay loop()

void setup() {

    // Initialize an RHT03 sensor, with the data pin connected to D2.
    rht.begin(D2);

    // Initialise to 500 samples per day (OpenUV limit) and publish
    sample_rate_str = String(500);

    // Set the delay time from the sample rate
    delay_millis_from_sample_rate(sample_rate_str);

    // Register sample rate cloud function
    Particle.function("sample_rate", delay_millis_from_sample_rate);

    // Subscribe to webhooks
    Particle.subscribe("hook-response/dark_sky", didGetDarkSkyData, MY_DEVICES);
    Particle.subscribe("hook-response/uv_index", didGetOpenUVData, MY_DEVICES);

    // Set device name on InfluxDB
    idb.setDeviceName("kingswood_one");
}

void loop() {

    int updateRet = rht.update();

    if (updateRet == 1)
    {
        // get sensor data
        temperature = rht.tempC();
        humidity = rht.humidity();

        // trigger webhooks
        Particle.publish("dark_sky", data, PRIVATE);
        Particle.publish("uv_index", data, PRIVATE);

        // calculate corrected UV Index
        transmission = (cloud_cover * -0.69) + 1.0;
        uvindex_corrected = uvindex * transmission;

        // update influxDB
        idb.add("temperature", temperature);
        idb.add("humidity", humidity);
        idb.add("humidity_outside", humidity_outside);
        idb.add("outside_temperature", temperature_outside);
        idb.add("cloud_cover", cloud_cover);
        idb.add("uv_index", uvindex);
        idb.add("uv_index_corrected", uvindex_corrected);
        idb.sendAll();

        // publish sample rate
        Particle.publish("sample_rate", sample_rate_str, PRIVATE);
    }

    delay(delay_millis);
}

void didGetDarkSkyData(const char *event, const char *data) {
     // format: cloudCover~humidity~temperature
     String str = String(data);
     char strBuffer[125] = "";
     str.toCharArray(strBuffer, 125);

     cloud_cover = atof(strtok(strBuffer, "~"));
     humidity_outside = atof(strtok(NULL, "~"));
     temperature_outside = atof(strtok(NULL, "~"));
}

void didGetOpenUVData(const char *event, const char *data) {
    uvindex = atof(data);
}

int delay_millis_from_sample_rate(String sample_rate) {
    // sets the milliseconds of delay for the number of daily samples
    sample_rate_str = sample_rate;
    int sample_rate_int = atoi(sample_rate);
    delay_millis = int(1000 * 60 * 60 * 24 / sample_rate_int);
    return 1;
}
