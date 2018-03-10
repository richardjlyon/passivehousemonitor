/*
 * Project TempHumidityRHT03
 * Description: Temperature and Humidity monitor (RHT03 sensor)
 * Retrieves outside temperature from darksky.net
 * Author: Richard Lyon
 * Date: 27 December 2017
 * Version: 0.1
*
* Device: kingswood_one
 */

#include "authenticate.h" // InfluxDB credentials
#include "InfluxDB.h"

#define DELAY_BEFORE_REBOOT (20 * 1000)
#define SAMPLE_RATE (1000 * 60 * 60 * 24 / 1000) // 1000 requests per day (DarkSky limit)

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

bool resetFlag = false;
unsigned int rebootDelayMillis = DELAY_BEFORE_REBOOT;
unsigned long rebootSync = millis();
int cloudResetFunction(String command);

void setup() {
   rht.begin(D2); // Initialize an RHT03 sensor, with the data pin connected to D2.

   // Publish IP address to Particle
   IPAddress myIP = WiFi.localIP();
   Particle.publish("IP Address", String(myIP));

   // Declare Particle variables
   Particle.variable("cloud_cover", cloud_cover);
   Particle.variable("transmission", transmission);
   Particle.variable("UV_index", uvindex);
   Particle.variable("UV_index_cor", uvindex_corrected);

   // Subscribe to webhooks
   Particle.subscribe("hook-response/dark_sky", didGetDarkSkyData, MY_DEVICES);
   Particle.subscribe("hook-response/uv_index", didGetOpenUVData, MY_DEVICES);

   // Publish the reset function to Particle
   Particle.function("reset", cloudResetFunction);

   // Set device name on InfluxDB
   idb.setDeviceName("kingswood_one");
}

void loop() {
   // Handle cloud reset logic: requires a delay to prevent race conditions
   if ((resetFlag) && (millis() - rebootSync >=  rebootDelayMillis)) {
       Particle.publish("Remote Reset Initiated");
       System.reset();
   }

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

       // Update console
      //  Particle.publish("Updated InfluxDB");
      //  Particle.publish("outside_temperature", String(temperature_outside));
      //  Particle.publish("cloud_cover", String(cloud_cover));
      //  Particle.publish("uvindex", String(uvindex));

   }

   delay(SAMPLE_RATE);
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

 int cloudResetFunction(String command)
 {
     resetFlag = true;
     rebootSync=millis();
     return 1;
 }
