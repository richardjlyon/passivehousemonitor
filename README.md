# TempHumidityRHT03

An environmental logger for a Passive House.

This is the firmware for a Particle Photon.

The main elements are:

1. The photon and firmware [this](https://github.com/richardjlyon/passivehousemonitor)
2. An RHT03 combined temperature and humidity sensor
3. An [InfluxDB](https://www.influxdata.com/time-series-platform/influxdb/) timeseries database and  [Grafana](http://139.59.166.111:3000/dashboard/db/kingswood?orgId=1) ( u:guest, p:kingswood) dashboard running on a [DigitalOcean](https://www.digitalocean.com) virtual machine
4. An [InfluxDB library](https://github.com/richardjlyon/InfluxDB) I wrote (https://github.com/richardjlyon/InfluxDB)
5. [OpenUV](https://www.openuv.io/index.html) for UV Index info
6. [DarkSky.net](https://darksky.net/dev/docs) for external temperature, humidity, and cloud cover

It obtains external temperature, humidity, cloud cover, and UV Index from APIs triggered by webooks. It combines those with the internal sensor data and sends them to InfluxDB. It’s been running continuously for 60 days and seems to be stable.
