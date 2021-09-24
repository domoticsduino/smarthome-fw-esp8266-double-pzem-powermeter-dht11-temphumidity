# Smarthome firmware for ESP8266 - PZEM Power Meter and DHT11 Temperature/Humidity sensor
Firmware for my board named **ESP8266MCU13**, based on **ESP8266 microcontroller**.

This device use two **PZEM-004T boards** as power meter device to monitor *photovoltaic production* and *house consumption*, a **DHT11 sensor** to monitor *temperature and humidity*. Use **MQTT protocol** to send data to **OpenHAB Smart Home Automation System**.

Built with **platformio** (https://platformio.org) and **visual studio code** (https://code.visualstudio.com)

Depends on the following *dd libraries*:

 - ddcommon
 - dddhtxx
 - ddwifi
 - ddmqtt
 - ddpzem004t
 
## Compile & Build

To build your firmware bin file:
 - clone this repository with the *--recursive* flag to checkout **dd-libraries**
 - rename file *include/user-config-template.h* in *include/user-config.h*
 - set your **WIFI** and **MQTT** settings in file *include/user-config.h*
 
Firmware file will be in ***.pio/build/esp12e/firmware.bin***

## Branches
 - ***main*** contains work in progress commits - **NOT STABLE branch**
 - ***master*** contains **STABLE** commits and releases

## Credits
 - *For autoversion management script* => **PlatformIO forum** ["How to build got revision into binary for version output?"](https://community.platformio.org/t/how-to-build-got-revision-into-binary-for-version-output/15380/5)