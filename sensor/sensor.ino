#include <WiFi.h>
#include <ArduinoHA.h>

#include "wifi.h"
// Create file with the following
// *************************************************************************
// #define SECRET_SSID "";  /* Replace with your SSID */
// #define SECRET_PSK "";   /* Replace with your WPA2 passphrase */
// *************************************************************************

const char ssid[] = SECRET_SSID;         /* Replace with your SSID */
const char passphrase[] = SECRET_PSK;   /* Replace with your WPA2 passphrase */

#define BROKER_ADDR IPAddress(192,168,178,42)

byte mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A};
unsigned long lastSentAt = millis();
double lastValue = 0;

WiFiClient client;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device);
HASensor temp("temp"); // "temp" is unique ID of the sensor. You should define your own ID.

void onBeforeSwitchStateChanged(bool state, HASwitch* s)
{
  // this callback will be called before publishing new state to HA
  // in some cases there may be delay before onStateChanged is called due to network latency
}

boolean isConnected = false;
void setup() {
  Serial.begin(115200);
  delay(10);

  // Make sure you're in station mode
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print(F("Connecting to "));
  Serial.print(ssid);

  if (passphrase != NULL)
    WiFi.begin(ssid, passphrase);
  else
    WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print(F("Connected with IP: "));
  Serial.println(WiFi.localIP());

  // set device's details (optional)
  device.setName("Hottub");
  device.setSoftwareVersion("0.0.1");

  // configure sensor (optional)
  temp.setUnitOfMeasurement("°C");
  temp.setDeviceClass("temperature");
  temp.setIcon("mdi:home");
  temp.setName("Tub temperature");

  mqtt.begin(BROKER_ADDR);

}

String result = "";
double tempValue;
void loop() {
  mqtt.loop();

  if (!isConnected) {
    Serial.println("Try to connect to hottub...");
    if (!client.connect("hottub", 7777)) {
      Serial.println("Connection failed.");
      Serial.println("Waiting 5 seconds before retrying...");
      delay(5000);
      return;
    }
    else {
      Serial.println("Connected");
      isConnected = true;
    }
  }


  if (isConnected && client.available() > 0) {
    size_t len = client.available();
    //    Serial.printf("bytes avail = %u\n", len);
    uint8_t buf[len];
    client.read(buf, len);
    for (int i = 0; i < len; i++) {
      if (String(buf[i], HEX) == "fa") {
        Serial.printf("HexString2ASCIIString message = %s\n", HexString2ASCIIString(result));
        Serial.print("message = ");
        Serial.println(result);

        if(result.substring(0,4) == "fa14") {
          tempValue = (HexString2ASCIIString(result.substring(4,10)).toDouble() / 10);
          Serial.printf("temp = %f\n", tempValue);
          Serial.println(tempValue);
        }
        
        result = ""; // clear buffer
        Serial.println("\n");
      }
      if (buf[i] < 0x10) {
        result += '0';
      }
      result += String(buf[i], HEX);
    }
  }

  if ((millis() - lastSentAt) >= 10000) {
    Serial.printf("Send temp data %f\n", tempValue);
    lastSentAt = millis();
    temp.setValue(tempValue);

    // Supported data types:
    // uint32_t (uint16_t, uint8_t)
    // int32_t (int16_t, int8_t)
    // double
    // float
    // const char*
  }
}

String HexString2ASCIIString(String hexstring) {
  String temp = "", sub = "", result;
  char buf[3];
  for (int i = 0; i < hexstring.length(); i += 2) {
    sub = hexstring.substring(i, i + 2);
    sub.toCharArray(buf, 3);
    char b = (char)strtol(buf, 0, 16);
    if (b == '\0')
      break;
    temp += b;
  }
  return temp;
}
