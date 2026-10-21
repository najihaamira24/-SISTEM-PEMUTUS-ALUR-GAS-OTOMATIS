#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "eu.thingsboard.cloud";
const char* mqtt_token = "YOUR_THINGSBOARD_ACCESS_TOKEN";

WiFiClient espClient;
PubSubClient client(espClient);

// Definisi Pin & Sensor
const int gasPin = 34;   // Pin Sensor Gas (Potensiometer)
#define ONE_WIRE_BUS 4   // Pin Data DS18B20 terhubung ke GPIO 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const int ledMerah = 27;  // LED Indikator Bahaya
const int buzzerPin = 26; // Alarm Buzzer
const int relayValve = 14; // Kontrol Modul Relay & Solenoid Valve (LED Biru)

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_token, "")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledMerah, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayValve, OUTPUT);
  digitalWrite(relayValve, LOW); // Kondisi awal: katup terbuka

  sensors.begin(); // Memulai sensor DS18B20
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

unsigned long lastMsg = 0;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    int gasValue = analogRead(gasPin);
    sensors.requestTemperatures();
    float suhuValue = sensors.getTempCByIndex(0); // Membaca suhu Celsius

    String statusSistem = "Aman";

    // Logika Pengaman Ganda
    if (gasValue >= 2000 || suhuValue >= 45.0) {
      digitalWrite(ledMerah, HIGH);
      digitalWrite(buzzerPin, HIGH);
      digitalWrite(relayValve, HIGH); // Katup menutup
      statusSistem = "BAHAYA (Gas/Suhu Tinggi)";
    }
    else {
      digitalWrite(ledMerah, LOW);
      digitalWrite(buzzerPin, LOW);
      digitalWrite(relayValve, LOW); // Katup terbuka
      statusSistem = "Aman";
    }

    Serial.println("Gas: " + String(gasValue) + " | Suhu: " + String(suhuValue) + "°C | Status: " + statusSistem);

    // Kirim data telemetri multi-sensor ke ThingsBoard
    String payload = "{\"kadar_gas\": " + String(gasValue) + ", \"suhu\": " + String(suhuValue) + ", \"status_sistem\": \"" + statusSistem + "\"}";
    client.publish("v1/devices/me/telemetry", payload.c_str());
  }
}
