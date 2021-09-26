/* 1.0.0 VERSION */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "../include/config.h"

#include <ddcommon.h>
#include <ddwifi.h>
#include <ddmqtt.h>
#include <ddpzem004t.h>
#include <dddhtxx.h>

int currentSample;

//Wifi
DDWifi wifi(ssid, password, wifihostname, LEDSTATUSPIN);

//MQTT
DDMqtt clientMqtt(DEVICE, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PWD, TOPIC_S, MQTT_QOS, LEDSTATUSPIN);

//PZEM
DDPzem004t pzemWrapper(LEDSTATUSPIN);

PZEM004T pzemSolar = PZEM004T(PZEMSOLARPIN1, PZEMSOLARPIN2);
IPAddress ipSolar = IPAddress(192, 168, 1, 1);
float lastPowerTriggerValueSolar;

PZEM004T pzemBa = PZEM004T(PZEMBAPIN1, PZEMBAPIN2);
IPAddress ipBa = IPAddress(192, 168, 1, 2);
float lastPowerTriggerValueBa;

//DHTXX Temp/Humidity Sensor
DDDhtxx tempSensor(LEDSTATUSPIN, DHTPIN);
DDDHTXXVal sampleTempHumValues;
int countSampleTempHum;

//JSON
DynamicJsonBuffer jsonBuffer;
JsonObject &jsonRoot = jsonBuffer.createObject();
JsonObject &configRoot = jsonRoot.createNestedObject("config");
JsonObject &tempHum = jsonRoot.createNestedObject("env");
JsonObject &powerC = jsonRoot.createNestedObject("powerC");
JsonObject &powerFV = jsonRoot.createNestedObject("powerFV");
JsonObject &jsonInfo = jsonRoot.createNestedObject("info");

// WEB SERVER - OTA
AsyncWebServer server(80);

void createJsonConfig()
{

	configRoot["readInterval"] = READ_INTERVAL;
	configRoot["maxSamples"] = MAX_SAMPLES;
}

void printDebugPowerData(DDPZEM004TVal values)
{

	writeToSerial("POWER READ Success = ", false);
	writeToSerial(values.success ? "True" : "False", true);
	if (values.success)
	{

		writeToSerial("Voltage: ", false);
		writeToSerial(values.voltage, false);
		writeToSerial(" V ", true);
		writeToSerial("Current: ", false);
		writeToSerial(values.current, false);
		writeToSerial(" A ", true);
		writeToSerial("Power: ", false);
		writeToSerial(values.power, false);
		writeToSerial(" W ", true);
		writeToSerial("Energy: ", false);
		writeToSerial(values.energy, false);
		writeToSerial(" Wh ", true);
	}
}

void printDebugTempHum(DDDHTXXVal tempHumValue)
{

	writeToSerial("TEMP Success = ", false);
	writeToSerial(tempHumValue.success ? "True" : "False", true);
	if (!tempHumValue.success)
		writeToSerial(tempHumValue.errorMsg, true);
	else
	{

		writeToSerial("Humidity: ", false);
		writeToSerial(tempHumValue.humidity, false);
		writeToSerial(" %\t", false);
		writeToSerial("Temperature: ", false);
		writeToSerial(tempHumValue.tempC, false);
		writeToSerial(" *C ", false);
		writeToSerial(tempHumValue.tempF, false);
		writeToSerial(" *F\t", false);
		writeToSerial("Heat index: ", false);
		writeToSerial(tempHumValue.heatIndexC, false);
		writeToSerial(" *C ", false);
		writeToSerial(tempHumValue.heatIndexF, false);
		writeToSerial(" *F", true);
	}
}

String generateJsonMessageTemp(DDDHTXXVal tempHumValue, int countSampleTempHum)
{

	DDDHTXXVal tempHumValueTot;

	if (countSampleTempHum > 0)
	{

		tempHumValueTot.humidity = tempHumValue.humidity / countSampleTempHum;
		tempHumValueTot.tempC = tempHumValue.tempC / countSampleTempHum;
		tempHumValueTot.tempF = tempHumValue.tempF / countSampleTempHum;
		tempHumValueTot.heatIndexC = tempHumValue.heatIndexC / countSampleTempHum;
		tempHumValueTot.heatIndexF = tempHumValue.heatIndexF / countSampleTempHum;

		tempHum["error"] = "";
	}
	else
		tempHum["error"] = "No samples";

	tempHum["humidity"] = tempHumValueTot.humidity;
	tempHum["tempC"] = tempHumValueTot.tempC;
	tempHum["tempF"] = tempHumValueTot.tempF;
	tempHum["heatIndexC"] = tempHumValueTot.heatIndexC;
	tempHum["heatIndexF"] = tempHumValueTot.heatIndexF;

	String json;
	tempHum.printTo(json);

	return json;
}

String generateJsonMessagePower(DDPZEM004TVal values, JsonObject &power)
{
	power["voltage"] = values.voltage;
	power["current"] = values.current;
	power["power"] = values.power;
	power["energy"] = values.energy;
	String json;
	power.printTo(json);
	return json;
}

String generateJsonMessageRoot()
{
	String json;
	jsonRoot.printTo(json);
	return json;
}

void setup()
{

	createJsonConfig();

	pinMode(LEDSTATUSPIN, OUTPUT);
	digitalWrite(LEDSTATUSPIN, LOW);

	if (SERIAL_ENABLED)
		Serial.begin(SERIAL_BAUDRATE);

	writeToSerial(USER_SETTINGS_WIFI_HOSTNAME, false);
	writeToSerial(" Booting...", true);
	writeToSerial("FW Version: ", false);
	writeToSerial(AUTO_VERSION, true);

	// WIFI
	wifi.connect();

	//MQTT
	clientMqtt.reconnectMQTT();

	//WEB SERVER
	jsonInfo["name"] = USER_SETTINGS_WIFI_HOSTNAME;
	jsonInfo["version"] = AUTO_VERSION;
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "application/json", generateJsonMessageRoot());
	});
	AsyncElegantOTA.begin(&server);
	server.begin();
	writeToSerial("Http server started", true);

	//PZEM
	pzemWrapper.init(&pzemSolar, ipSolar);
	pzemWrapper.init(&pzemBa, ipBa);

	// DHTXX
	tempSensor.beginSensor();
	countSampleTempHum = 0;

	currentSample = 0;
	lastPowerTriggerValueSolar = -999.0;
	lastPowerTriggerValueBa = -999.0;
}

void loop()
{

	AsyncElegantOTA.loop();

	// Wait a few seconds between measurements.
	delay(configRoot["readInterval"]);

	clientMqtt.loop();

	DDPZEM004TVal valuesSolar = pzemWrapper.getValues(&pzemSolar, ipSolar);
	printDebugPowerData(valuesSolar);
	DDPZEM004TVal valuesBa = pzemWrapper.getValues(&pzemBa, ipBa);
	printDebugPowerData(valuesBa);

	// Get TEMP/HUMIDITY Value
	DDDHTXXVal tempHumValue = tempSensor.getValue();
	printDebugTempHum(tempHumValue);

	currentSample++;

	if (tempHumValue.success)
	{

		sampleTempHumValues.humidity += tempHumValue.humidity;
		sampleTempHumValues.tempC += tempHumValue.tempC;
		sampleTempHumValues.tempF += tempHumValue.tempF;
		sampleTempHumValues.heatIndexC += tempHumValue.heatIndexC;
		sampleTempHumValues.heatIndexF += tempHumValue.heatIndexF;

		countSampleTempHum++;
	}

	writeToSerial("Current Sample ", false);
	writeToSerial(currentSample, true);
	writeToSerial("lastPowerTriggerValueSolar ", false);
	writeToSerial(lastPowerTriggerValueSolar, true);
	writeToSerial("lastPowerTriggerValueBa ", false);
	writeToSerial(lastPowerTriggerValueBa, true);

	if (currentSample >= MAX_SAMPLES)
	{

		clientMqtt.sendMessage(TOPIC_DHT11_P, generateJsonMessageTemp(sampleTempHumValues, countSampleTempHum));

		currentSample = 0;
	}

	if (abs(lastPowerTriggerValueSolar - valuesSolar.power) >= DIFF_POWER_TRIGGER || (lastPowerTriggerValueSolar != valuesSolar.power && valuesSolar.power == 0))
	{

		clientMqtt.sendMessage(TOPIC_PZEM1_P, generateJsonMessagePower(valuesSolar, powerFV));

		lastPowerTriggerValueSolar = valuesSolar.power;
	}

	if (abs(lastPowerTriggerValueBa - valuesBa.power) >= DIFF_POWER_TRIGGER || (lastPowerTriggerValueBa != valuesBa.power && valuesBa.power == 0))
	{

		clientMqtt.sendMessage(TOPIC_PZEM2_P, generateJsonMessagePower(valuesBa, powerC));

		lastPowerTriggerValueBa = valuesBa.power;
	}
}