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

   // Subscribe to 'dark_sky' webhook
   Particle.subscribe("hook-response/dark_sky", didGetDarkSkyData, MY_DEVICES);

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

       // trigger webhook 'didGetDarkSkyData'
       Particle.publish("dark_sky", data, PRIVATE);

       // update influxDB
       idb.add("temperature", temperature);
       idb.add("humidity", humidity);
       idb.add("humidity_outside", humidity_outside);
       idb.add("outside_temperature", temperature_outside);
       idb.add("uv_index", uvindex_corrected);
       idb.sendAll();
   }

   delay(SAMPLE_RATE);
}


 void didGetDarkSkyData(const char *event, const char *data) {
     // format: cloudCover~humidity~temperature-uvIndex
     String str = String(data);
     char strBuffer[125] = "";
     str.toCharArray(strBuffer, 125);

     cloud_cover = atof(strtok(strBuffer, "~"));
     humidity_outside = atof(strtok(NULL, "~"));
     temperature_outside = (atof(strtok(NULL, "~")) - 32.0) * 5.0/9.0;
     // correct UV Index for cloud cover
     double transmission = cloud_cover * -0.69 + 1;
     uvindex_corrected = atof(strtok(NULL, "~")) * transmission;
 }


 int cloudResetFunction(String command)
 {
     resetFlag = true;
     rebootSync=millis();
     return 1;
 }
