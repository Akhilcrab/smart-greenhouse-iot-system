/**********************************************************************************
 * Smart Greenhouse IoT System - Non-Blocking Production Version
 * 
 * Uses BlynkTimer to ensure smooth, non-blocking execution without delay()
 * Recommended for stable 24/7 continuous greenhouse monitoring.
 * 
 * Developer: Akhil Rawat (Graphic Era Hill University, Dehradun)
 **********************************************************************************/

#define BLYNK_TEMPLATE_ID "TMPL3zUiMuQij"
#define BLYNK_TEMPLATE_NAME "NODEMCU"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

#define DHTPIN D4        // DHT11 Data Pin
#define DHTTYPE DHT11
#define SOIL_PIN A0      // Soil Moisture Analog Pin
#define LDR_PIN D0       // LDR Module Digital Out
#define TRIG_PIN D7      // HC-SR04 TRIG Pin
#define ECHO_PIN D8      // HC-SR04 ECHO Pin
#define BUZZER_PIN D5    // Buzzer Pin
#define LED_PIN D6       // Soil Alert LED Pin

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

#define V_TEMP V1
#define V_HUM V2
#define V_SOIL V3
#define V_LIGHT V4
#define V_DISTANCE V5
#define V_ALERT V6

void sendNotification(String message) {
    Blynk.logEvent("plant", message); 
    Serial.println("ALERT: " + message);
}

long getDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
    if (duration == 0) return 999; // Out of range or no echo
    return duration * 0.034 / 2;
}

void sendSensorData() {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int soil = analogRead(SOIL_PIN); 
    int light = digitalRead(LDR_PIN);
    int soilPercent = constrain(map(soil, 1023, 0, 0, 100), 0, 100);
    long distance = getDistance();

    if (!isnan(temp) && !isnan(hum)) {
        Blynk.virtualWrite(V_TEMP, temp);
        Blynk.virtualWrite(V_HUM, hum);
        Blynk.setProperty(V_TEMP, "label", String(temp, 1) + " °C");
        Blynk.setProperty(V_HUM, "label", String(hum, 1) + " %");
    }

    Blynk.virtualWrite(V_SOIL, soilPercent);
    Blynk.setProperty(V_SOIL, "label", String(soilPercent) + " %");

    Blynk.virtualWrite(V_DISTANCE, distance);
    Blynk.setProperty(V_DISTANCE, "label", String(distance) + " cm");

    Blynk.virtualWrite(V_LIGHT, light == LOW ? "Low Light" : "Adequate Light");

    // Soil moisture condition
    if (soilPercent < 25) {
        digitalWrite(LED_PIN, HIGH);
        sendNotification("Low Soil Moisture: " + String(soilPercent) + "%");
    } else {
        digitalWrite(LED_PIN, LOW);
    }

    // Proximity / intrusion condition
    if (distance > 0 && distance <= 30) {
        digitalWrite(BUZZER_PIN, HIGH);
        sendNotification("Object or animal detected! Distance: " + String(distance) + " cm");
        timer.setTimeout(1500L, []() {
            digitalWrite(BUZZER_PIN, LOW);
        });
    }

    // Low light condition
    if (light == LOW) {
        sendNotification("Low Light Detected in Greenhouse!");
    }
}

void setup() {
    Serial.begin(9600);
    WiFi.begin(ssid, pass);
    Blynk.begin(auth, ssid, pass);
    dht.begin();

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);

    // Call sendSensorData every 5 seconds without blocking loop()
    timer.setInterval(5000L, sendSensorData);
}

void loop() {
    Blynk.run();
    timer.run();
}
