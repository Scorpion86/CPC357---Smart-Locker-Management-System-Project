#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_wifi.h"
#include "esp_wpa2.h"

// ===== PINS =====
#define TRIG_PIN 14
#define ECHO_PIN 21
#define DOOR_PIN 47
#define PIR_PIN 4
#define BUZZER_PIN 12
#define LED_GREEN 5
#define LED_RED 6

// ===== THRESHOLDS =====
#define OCCUPIED_DISTANCE 12.0   // cm (locker depth 13cm, safe margin)
#define ALERT_TIME 60000         // 60 seconds
#define PIR_HOLD_TIME 5000       // 5 seconds movement memory

// ===== STATES =====
enum LockerState {
  EMPTY,
  OCCUPIED,
  IN_USE,
  ALERT
};

LockerState currentState = EMPTY;

// ===== TIMERS =====
unsigned long doorOpenTime = 0;
unsigned long lastMotionTime = 0;

// ===== WIFI/MQTT =====
const char* WIFI_SSID = "USMSecure";
const char* WIFI_IDENTITY = "muvenddran@student.usm.my";
const char* WIFI_USERNAME = "muvenddran@student.usm.my";
const char* WIFI_PASSWORD = "Junior4002#";

const char* MQTT_HOST = "34.126.155.177";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "lockeruser";
const char* MQTT_PASS = "Muven4002#";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastPublish = 0;
const unsigned long publishInterval = 2000; // ms



// ===== WIFI/MQTT HELPERS =====
void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("Connecting WiFi...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)WIFI_IDENTITY, strlen(WIFI_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t*)WIFI_USERNAME, strlen(WIFI_USERNAME));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t*)WIFI_PASSWORD, strlen(WIFI_PASSWORD));
  esp_wifi_sta_wpa2_ent_enable();
  WiFi.begin(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi OK, IP: ");
  Serial.println(WiFi.localIP());
}

void ensureMqtt() {
  if (mqttClient.connected()) return;
  Serial.println("Connecting MQTT...");
  while (!mqttClient.connected()) {
    mqttClient.connect("locker-esp32", MQTT_USER, MQTT_PASS);
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("MQTT connected");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  ensureWiFi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(DOOR_PIN, INPUT_PULLUP); // MC-38
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  Serial.println("=== SMART LOCKER SYSTEM STARTED ===");
}

// ===== ULTRASONIC FUNCTION =====
float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999.0;
  return duration * 0.034 / 2;
}

// ===== STATE NAME FUNCTION =====
const char* stateName(LockerState s) {
  switch (s) {
    case EMPTY: return "EMPTY";
    case OCCUPIED: return "OCCUPIED";
    case IN_USE: return "IN_USE";
    case ALERT: return "ALERT";
    default: return "UNKNOWN";
  }
}

// ===== LOOP =====
void loop() {
  ensureWiFi();
  ensureMqtt();
  mqttClient.loop();


  // --- SENSOR READINGS ---
  float distance = readDistance();
  bool doorOpen = digitalRead(DOOR_PIN) == HIGH; // HIGH = OPEN

  // PIR movement memory
  if (digitalRead(PIR_PIN) == HIGH) {
    lastMotionTime = millis();
  }
  bool humanDetected = (millis() - lastMotionTime) < PIR_HOLD_TIME;

  // Reset alert timer when human detected AND door is open
  if (doorOpen && humanDetected) {
    doorOpenTime = millis();
  } 

  // Detect door opening moment
  static bool lastDoorState = false;
  if (doorOpen && !lastDoorState) {
    doorOpenTime = millis();
  }
  lastDoorState = doorOpen;

  LockerState lastState = currentState;

  // ===== STATE MACHINE =====
  switch (currentState) {

    case EMPTY:
      if (!doorOpen) {
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_RED, LOW);
      } else {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH); // EMPTY + OPEN → red ON
      }

      noTone(BUZZER_PIN);

      if (distance < OCCUPIED_DISTANCE) {
        currentState = OCCUPIED;
      }

      if (doorOpen && !humanDetected &&
          millis() - doorOpenTime > ALERT_TIME) {
        currentState = ALERT;
      }
      break;

    case OCCUPIED:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      noTone(BUZZER_PIN);

      if (doorOpen) {
        currentState = IN_USE;
      } else if (distance >= OCCUPIED_DISTANCE) {
        currentState = EMPTY;
      }
      break;

    case IN_USE:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
      noTone(BUZZER_PIN);

      if (doorOpen && !humanDetected &&
          millis() - doorOpenTime > ALERT_TIME) {
        currentState = ALERT;
      }

      if (!doorOpen) {
        currentState = (distance < OCCUPIED_DISTANCE) ? OCCUPIED : EMPTY;
      }
      break;

    case ALERT:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, millis() % 500 < 250); // blinking red
      tone(BUZZER_PIN, 2000);

      // ❗ HUMAN RETURNS → cancel alert
      if (humanDetected) {
        noTone(BUZZER_PIN);
        currentState = IN_USE;
        doorOpenTime = millis(); // reset timer
        break;
      }

      // 🚪 Door closed → resolve state
      if (!doorOpen) {
        noTone(BUZZER_PIN);
        currentState = (distance < OCCUPIED_DISTANCE) ? OCCUPIED : EMPTY;
      }
      break;
  }

  // ===== COUNTDOWN CALCULATION =====
  long remainingTime = -1;
  if (doorOpen && !humanDetected && currentState != ALERT) {
    remainingTime = ALERT_TIME - (millis() - doorOpenTime);
    if (remainingTime < 0) remainingTime = 0;
  }

  if (millis() - lastPublish > publishInterval) {
    lastPublish = millis();
    char payload[256];
    snprintf(payload, sizeof(payload),
      "{\"device_id\":\"locker01\",\"distance_cm\":%.2f,\"door_open\":%d,"
      "\"human_detected\":%d,\"state\":\"%s\",\"alert_remaining_s\":%.1f}",
      distance,
      doorOpen ? 1 : 0,
      humanDetected ? 1 : 0,
      stateName(currentState),
      remainingTime >= 0 ? remainingTime / 1000.0 : -1.0
    );
    mqttClient.publish("locker/telemetry", payload);
  }

  // ===== SERIAL DISPLAY =====
  Serial.print("[STATE: ");
  Serial.print(stateName(currentState));
  Serial.print("] Door: ");
  Serial.print(doorOpen ? "OPEN" : "CLOSED");
  Serial.print(" | Human: ");
  Serial.print(humanDetected ? "YES" : "NO");
  Serial.print(" | Distance: ");
  Serial.print(distance);
  Serial.print(" cm");

  if (remainingTime >= 0) {
    Serial.print(" | Alert in: ");
    Serial.print(remainingTime / 1000.0);
    Serial.print(" s");
  }

  Serial.println();

  if (currentState != lastState) {
    Serial.print(">>> STATE CHANGED TO: ");
    Serial.println(stateName(currentState));
  }

  delay(300);
}
