/**********************************************************************************
 * Smart Greenhouse IoT System for Real-Time Monitoring and Alert
 * 
 * Developer: Akhil Rawat (Graphic Era Hill University, Dehradun)
 * Course: B.Tech Computer Science & Engineering
 * Project: IoT Mini Project
 * Microcontroller: ESP8266 (NodeMCU v3 / ESP-12E)
 * Cloud Platform: Blynk IoT 2.0
 * 
 * Features:
 *  - Temperature & Humidity Monitoring (DHT11)
 *  - Soil Moisture Sensing with Low Moisture Alert (Analog Resistive Sensor)
 *  - Ambient Light Detection (LDR Module)
 *  - Proximity / Intrusion Detection with Buzzer (Ultrasonic HC-SR04)
 *  - Cloud Push Notifications & LED Indicators
 **********************************************************************************/

// Blynk Template Configuration (Get these from your Blynk Console)
#define BLYNK_TEMPLATE_ID "TMPL3zUiMuQij"
#define BLYNK_TEMPLATE_NAME "NODEMCU"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN" // Replace with your Blynk Auth Token

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// Wi-Fi Credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "YOUR_WIFI_SSID";     // Enter your Wi-Fi SSID
char pass[] = "YOUR_WIFI_PASSWORD"; // Enter your Wi-Fi Password

// Hardware Pin Definitions
#define DHTPIN D4        // DHT11 Data Pin (GPIO 2)
#define DHTTYPE DHT11    // DHT sensor type
#define SOIL_PIN A0      // Soil moisture analog pin (ADC0)
#define LDR_PIN D0       // LDR Digital Output Pin (GPIO 16)
#define TRIG_PIN D7      // Ultrasonic HC-SR04 TRIG Pin (GPIO 13)
#define ECHO_PIN D8      // Ultrasonic HC-SR04 ECHO Pin (GPIO 15)
#define BUZZER_PIN D5    // Piezo Buzzer Pin (GPIO 14)
#define LED_PIN D6       // Soil Moisture Alert LED Pin (GPIO 12)

DHT dht(DHTPIN, DHTTYPE);

// Blynk Virtual Pins
#define V_TEMP V1        // Temperature (°C)
#define V_HUM V2         // Humidity (%)
#define V_SOIL V3        // Soil Moisture (%)
#define V_LIGHT V4       // Light Status / Intensity
#define V_DISTANCE V5    // Distance (cm)
#define V_ALERT V6       // Alert Notification

// Send event notifications to Blynk Mobile App and Console
void sendNotification(String message) {
    Blynk.logEvent("plant", message); 
    Serial.println("ALERT: " + message);
}

// Measure distance using HC-SR04 ultrasonic sensor (returns cm)
long getDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    long distance = duration * 0.034 / 2; // Convert echo sound travel time to centimeters
    return distance;
}

// Read all sensors, print to serial monitor, send to Blynk and trigger alerts
void sendSensorData() {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int soil = analogRead(SOIL_PIN); 
    int light = digitalRead(LDR_PIN);
    
    // Normalize analog soil moisture (1023 dry, 0 wet) to 0% - 100%
    int soilPercent = map(soil, 1023, 0, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);
    
    long distance = getDistance(); // Proximity detection

    // Display on Serial Monitor
    Serial.println("----------------------------------------");
    Serial.print("Temperature:   "); Serial.print(temp); Serial.println(" °C");
    Serial.print("Humidity:      "); Serial.print(hum); Serial.println(" %");
    Serial.print("Soil Moisture: "); Serial.print(soilPercent); Serial.println(" %");
    Serial.print("Distance:      "); Serial.print(distance); Serial.println(" cm");
    Serial.print("Light Level:   "); Serial.println(light == LOW ? "Low" : "Adequate");
    Serial.println("----------------------------------------");

    // Sending data labels to Blynk console and mobile application
    Blynk.setProperty(V_TEMP, "label", String(temp) + " C");
    Blynk.setProperty(V_HUM, "label", String(hum) + " %");
    Blynk.setProperty(V_SOIL, "label", String(soilPercent) + " %");
    Blynk.setProperty(V_DISTANCE, "label", String(distance) + " cm");

    // Also update virtual pin stream values for gauges/charts
    Blynk.virtualWrite(V_TEMP, temp);
    Blynk.virtualWrite(V_HUM, hum);
    Blynk.virtualWrite(V_SOIL, soilPercent);
    Blynk.virtualWrite(V_DISTANCE, distance);

    // Rule 1: Soil Moisture Alert (< 25%)
    if (soilPercent < 25) {
        digitalWrite(LED_PIN, HIGH); // Turn on visual alert LED
        sendNotification("Low Soil Moisture: " + String(soilPercent) + "%");
    } else {
        digitalWrite(LED_PIN, LOW);  // Turn off visual alert LED
    }

    // Rule 2: Animal/Intruder Proximity Alert (Within 30 cm)
    if (distance > 0 && distance <= 30) {
        digitalWrite(BUZZER_PIN, HIGH); // Sound buzzer alarm
        delay(2000); 
        sendNotification("Object or animal detected near the plant! Distance: " + String(distance) + " cm");
        digitalWrite(BUZZER_PIN, LOW);  // Silence buzzer
    }

    // Rule 3: Low Light Alert
    if (light == LOW) {
        sendNotification("Low Light Detected!!!");
    }
}

void setup() {
    Serial.begin(9600);
    
    // Connect to Wi-Fi and Blynk Cloud
    WiFi.begin(ssid, pass);
    Blynk.begin(auth, ssid, pass);
    
    dht.begin();

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected successfully!");
    Serial.print("NodeMCU IP Address: ");
    Serial.println(WiFi.localIP());

    // Configure Pin Modes
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);
}

void loop() {
    Blynk.run();       // Keep Blynk connection alive
    sendSensorData();  // Collect readings and evaluate alerts
    delay(10000);      // 10-second sampling interval
}
