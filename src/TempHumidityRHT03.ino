/*
 * Project TempHumidityRHT03
 * Description: Temperature and Humidity monitor (RHT03 sensor)
 * Author: Richard Lyon
 * Date: 27 December 2017
 * Version: 0.1
*
* Device: kingswood_one
 */

 #include <Ubidots.h>
 #include <SparkFunRHT03.h>

 #define DEVICE_ID "kingswood_one"
 #define DEVICE_LABEL "37002b001147343438323536"
 #define TOKEN "A1E-Ke3IBMVqyEUnzfGP9aDQRAmY2vGweM"  // Ubidots TOKEN
 #define DELAY_BEFORE_REBOOT (10 * 1000)
 #define SAMPLE_RATE (10*1000) // Time between samples (miliseconds)

 Ubidots ubidots(TOKEN);
 RHT03 rht; // This creates a RTH03 object, which we'll use to interact with the sensor

 double temperature;
 double humidity;

 bool resetFlag = false;
 unsigned int rebootDelayMillis = DELAY_BEFORE_REBOOT;
 unsigned long rebootSync = millis();
 int cloudResetFunction(String command);

 void setup() {
     rht.begin(D2); // Initialize an RHT03 sensor, with the data pin connected to D2.

     // Publish IP address to Particle
     IPAddress myIP = WiFi.localIP();
     Particle.publish("IP Address", String(myIP));

     // Publish variables to PArticle
     Particle.variable("temperature", &temperature, DOUBLE);
     Particle.variable("humidity", &humidity, DOUBLE);

     // Publish the reset function to Particle
     Particle.function("reset", cloudResetFunction);

     // Set device label and ID on ubidots
     ubidots.setDeviceLabel(DEVICE_LABEL);
     ubidots.setDeviceName(DEVICE_ID);
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

         // send to ubidots
         ubidots.add("k1_temperature", temperature);
         ubidots.add("k1_humidity", humidity);
         ubidots.sendAll();
     }

     delay(SAMPLE_RATE);
 }

 int cloudResetFunction(String command)
 {
     resetFlag = true;
     rebootSync=millis();
     return 1;
 }
