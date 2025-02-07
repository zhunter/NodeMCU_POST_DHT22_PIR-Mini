/*
    HTTP REST API POST/GET over TLS (HTTPS)
    Created By: Zachary Hunter / zachary.hunter@gmail.com
    Created On: April 2, 2024
    Verified Working on NodeMCU & NodeMCU MMINI on ESP8266
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>
#include "certs.h"
#include "DHT.h"

#define DHTPIN 4       // DHT 22 Sensor
#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

#define PIRPIN 5  // Mini PIR Sensor

int SensorState = 0;

// ESP8266 WIFI Setup
const char* ssid = "AutumnHillsGuest";
const char* password = "Hunter2023";
const char* serverName = "https://www.zachhunter.net/wp-json/wp-arduino-api/v1/arduino";

// Timer set to 1 seconds (1000)
unsigned long timerDelay = (1000 * 60 * 5);

// Time Last Run
unsigned long lastTime = millis() - timerDelay;

X509List cert(cert_ISRG_Root_X1);

void setup() {
  pinMode(DHTPIN, OUTPUT);
  pinMode(PIRPIN, INPUT);

  Serial.begin(115200);

  dht.begin();

  digitalWrite(2, HIGH);  // DIGITAL IO PIN2, BUILT IN BLUE LED

  WiFi.begin(ssid, password);
  Serial.print("Connecting ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));

  Serial.print("NodeMCU POST Sensor Value Every ");
  Serial.print(timerDelay / 1000);
  Serial.println(" seconds.");
}

void loop() {
  // int currentState = digitalRead(PIRPIN);

  JSONVar myObject;
  String jsonString;
  String sensorWriting;

  myObject["sendorValue"] = WiFi.localIP().toString();

  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      // Read temperature as Fahrenheit (isFahrenheit = true)
      float f = dht.readTemperature(true);

      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t) || isnan(f)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }

      // Compute heat index in Fahrenheit (the default)
      float hif = dht.computeHeatIndex(f, h);
      // Compute heat index in Celsius (isFahreheit = false)
      float hic = dht.computeHeatIndex(t, h, false);

      // ========================================================
      // POST PIR Motion Sensor
      // ========================================================
      jsonString, sensorWriting = "";
      myObject["sensorType"] = "DHT22 Humidity %";
      myObject["sensorValue"] = h;
      jsonString = JSON.stringify(myObject);
      sensorWriting = httpPOSTRequest(serverName, jsonString);

      // ========================================================
      // POST PIR Motion Sensor
      // ========================================================
      jsonString, sensorWriting = "";
      myObject["sensorType"] = "DHT22 Temperature °F";
      myObject["sensorValue"] = f;
      jsonString = JSON.stringify(myObject);
      sensorWriting = httpPOSTRequest(serverName, jsonString);

      // ========================================================
      // POST PIR Motion Sensor
      // ========================================================
      jsonString, sensorWriting = "";
      myObject["sensorType"] = "DHT22 Heat index °F";
      myObject["sensorValue"] = hif;
      jsonString = JSON.stringify(myObject);
      sensorWriting = httpPOSTRequest(serverName, jsonString);

      lastTime = millis();
    } else {
      Serial.println("WiFi Disconnected");
    }
  }

  // if (SensorState != currentState) {
  //   //Check WiFi connection status
  //   if (WiFi.status() == WL_CONNECTED) {
  //     String SensorStatus = "On";

  //     if (currentState == 0) {
  //       SensorStatus = "Off";
  //     } else {
  //       SensorStatus = "On";
  //     }

  //     SensorState = currentState;

  //     Serial.println(SensorStatus);

  //     // ========================================================
  //     // POST PIR Motion Sensor
  //     // ========================================================
  //     jsonString, sensorWriting = "";
  //     myObject["sensorType"] = "PIR Motion Sensor";
  //     myObject["sensorValue"] = SensorStatus;
  //     jsonString = JSON.stringify(myObject);
  //     sensorWriting = httpPOSTRequest(serverName, jsonString);
  //     Serial.println("==============================================================================");
  //   } else {
  //     Serial.println("WiFi Disconnected");
  //   }
  // }
}

// POST JSON Data to REST API
String httpPOSTRequest(const char* serverName, String httpRequestData) {
  WiFiClientSecure client;
  HTTPClient http;

  client.setTrustAnchors(&cert);

  // Enable if your having certificate issues
  //client.setInsecure();

  Serial.println("Secure POST Request to: " + String(serverName));
  Serial.println("Payload: " + httpRequestData);

  http.begin(client, serverName);
  http.addHeader("Authorization", "QT1kIG0Dt9u090ODH6bHXvGU");
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(httpRequestData);

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  Serial.println();

  http.end();

  return payload;
}

// GET Request to REST API
String httpGETRequest(const char* serverName) {
  WiFiClientSecure client;
  HTTPClient http;

  client.setTrustAnchors(&cert);

  // Enable if your having certificate issues
  //client.setInsecure();

  Serial.println("Secure GET Request to: " + String(serverName));

  http.begin(client, serverName);
  http.addHeader("Authorization", "QT1kIG0Dt9u090ODH6bHXvGU");

  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  Serial.println();

  http.end();

  return payload;
}