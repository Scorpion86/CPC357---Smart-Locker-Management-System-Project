Smart Locker Management System (CPC357)

Smart Locker is an ESP32-based IoT system that monitors locker occupancy, door status, and human presence using ultrasonic, reed switch, and PIR sensors. Sensor data is published via MQTT to a GCP-hosted stack (Mosquitto + InfluxDB + Grafana) for real-time visualization and analytics, supporting SDG 11 smart-city safety and resource management.

Project Overview
- Hardware: ESP32 + ultrasonic sensor + door switch (reed) + PIR sensor + buzzer + LEDs
- Cloud: GCP VM running Mosquitto, InfluxDB, and Grafana
- Visualization: Grafana live dashboard + analytics dashboard

Repository Contents
- final_code/final_code.ino: ESP32 firmware
- grafana_dashboard.json: live dashboard import file

Hardware Requirements
- ESP32 board
- Ultrasonic sensor (HC-SR04 or equivalent)
- Door magnetic reed switch (MC-38)
- PIR motion sensor
- Buzzer
- LEDs (red/green)
- Jumper wires and power supply

Pin Mapping (ESP32)
- Ultrasonic TRIG: GPIO 14
- Ultrasonic ECHO: GPIO 21
- Door switch: GPIO 47
- PIR: GPIO 4
- Buzzer: GPIO 12
- LED Green: GPIO 5
- LED Red: GPIO 6

Software Requirements
- Arduino IDE with ESP32 board support
- Arduino libraries:
  - PubSubClient
- GCP VM (Ubuntu 22.04 recommended)
- Mosquitto, InfluxDB, Telegraf, Grafana

Setup Instructions
1) ESP32 Firmware
- Open final_code/final_code.ino in Arduino IDE.
- Update WiFi and MQTT settings near the top:
  - WIFI_SSID, WIFI_IDENTITY, WIFI_USERNAME, WIFI_PASSWORD
  - MQTT_HOST, MQTT_USER, MQTT_PASS
- Install PubSubClient from the Library Manager.
- Upload the sketch to ESP32.

2) GCP VM (Mosquitto + InfluxDB + Grafana)
- Create a GCP VM (Ubuntu 22.04).
- Install Mosquitto, InfluxDB, Telegraf, and Grafana.
- Configure Mosquitto user/password authentication.
- Configure Telegraf to subscribe to MQTT and write to InfluxDB.

3) Grafana Dashboard
- Open Grafana at http://<VM_EXTERNAL_IP>:3000.
- Add InfluxDB as a data source.
- Import grafana_dashboard.json for the live dashboard.

Data Fields
The ESP32 publishes JSON to MQTT topic locker/telemetry:
- distance_cm (float)
- door_open (0/1)
- human_detected (0/1)
- state (EMPTY/OCCUPIED/IN_USE/ALERT)
- alert_remaining_s (float)

Dependencies
- ESP32 Arduino core
- PubSubClient Arduino library
- Mosquitto
- InfluxDB 2.x
- Telegraf
- Grafana

SDG 11 Impact
This system improves urban safety and resource management by detecting locker misuse, door left open, and abnormal activity in shared spaces. Real-time monitoring and alerting help reduce theft and optimize public asset usage.

License
For academic use only.