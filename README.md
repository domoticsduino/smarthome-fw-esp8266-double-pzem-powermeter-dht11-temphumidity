# Smarthome firmware for ESP8266 - PZEM Power Meter and DHT11 Temperature/Humidity sensor
Firmware for my board named **ESP8266MCU13**, based on **ESP8266 microcontroller**.

This device use two **PZEM-004T boards** as power meter device to monitor *photovoltaic production* and *house consumption*, a **DHT11 sensor** to monitor *temperature and humidity*. Use **MQTT protocol** to send data to **OpenHAB Smart Home Automation System**.

Built with **platformio** (https://platformio.org) and **visual studio code** (https://code.visualstudio.com)

To clone the project you need to use the *recursive* flag, to update libraries.

Depends on the following *dd libraries*:

 - ddcommon
 - dddhtxx
 - ddwifi
 - ddmqtt
 - ddpzem004t

To build your firmware bin file:
 - clone this repository with the *--recursive* flag to checkout **dd-libraries**
 - rename file *src/user-config-template.h* in *src/user-config.h*
 - set your **WIFI** and **MQTT** settings in file *src/user-config.h*
 
Firmware file will be in ***.pio/build/esp12e/firmware.bin***