/*
 * Project TempHumidityRHT03
 * Description: Temperature and Humidity monitor (RHT03 sensor)
 * Retrieves outside temperature from 'openweathermap'
 * Author: Richard Lyon
 * Date: 27 December 2017
 * Version: 0.1
*
* Device: kingswood_one
 */

 #include "authenticate.h" // InfluxDB credentials
 #include "InfluxDB.h"

 #define DELAY_BEFORE_REBOOT (20 * 1000)
 #define SAMPLE_RATE (1000 * 60 * 60 * 24 / 500) // 500 requests per day (OpenUV limit)

 #include <SparkFunRHT03.h>

 InfluxDB idb = InfluxDB(USERNAME, PASSWORD);
 RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor

 double temperature;
 double temperature_outside;
 double temperature_outside_old;
 double humidity;
 double uv_index;
 String str_outside_temperature = String(-999);
 String str_uv_index = String(-1);

 bool resetFlag = false;
 unsigned int rebootDelayMillis = DELAY_BEFORE_REBOOT;
 unsigned long rebootSync = millis();
 int cloudResetFunction(String command);

 void setup() {
     rht.begin(D2); // Initialize an RHT03 sensor, with the data pin connected to D2.

     // Publish IP address to Particle
     IPAddress myIP = WiFi.localIP();
     Particle.publish("IP Address", String(myIP));

     // Publish variables to Particle
     Particle.variable("temperature", &temperature, DOUBLE);
     Particle.variable("humidity", &humidity, DOUBLE);
     Particle.variable("t outside", &temperature_outside, DOUBLE);


     // Subscribe to 'outside_temperature' webhook
     Particle.subscribe("hook-response/outside_temperature", didGetOutsideTemperature, MY_DEVICES);

     // Subscribe to 'uv_index' webhook
     Particle.subscribe("hook-response/uv_index", didGetUVIndex, MY_DEVICES);

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
         temperature = rht.tempC();
         humidity = rht.humidity();

         // trigger webhook 'outside_temperature'
         Particle.publish("outside_temperature", str_outside_temperature, PRIVATE);

         // trigger webhook 'uv_index'
         Particle.publish("uv_index", str_uv_index, PRIVATE);

         // send to webhook -> influxDB
         Particle.publish("temperature", String(temperature), PRIVATE);

         // send to ubidots
         idb.add("temperature", temperature);
         idb.add("humidity", humidity);
         idb.add("outside_temperature", temperature_outside);
         idb.add("uv_index", uv_index);
         idb.sendAll();
     }

     delay(SAMPLE_RATE);
 }

 void didGetOutsideTemperature(const char *event, const char *data) {
   // Handle the 'outside_temperature' response
   String temperatureReturn = String(data);
   double temp = atof(temperatureReturn) - 273.15;
   if (temp > -20.0)
   {
     temperature_outside = round(temp*10)/10;
   }
 }

 void didGetUVIndex(const char *event, const char *data) {
   // Handle UV Index response
   String uvindex_return = String(data);
   double temp = atof(uvindex_return);
   if (temp >= 0)
   {
     uv_index = temp;
   }
}

 int cloudResetFunction(String command)
 {
     resetFlag = true;
     rebootSync=millis();
     return 1;
 }
