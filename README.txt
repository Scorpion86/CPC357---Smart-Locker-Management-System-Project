Smart Locker Management System (CPC357)

Smart Locker is an ESP32-based IoT system that monitors locker occupancy, door status, and human presence using ultrasonic, reed switch, and PIR sensors. Sensor data is published via MQTT to a GCP-hosted stack (Mosquitto + InfluxDB + Grafana) for real-time visualization and analytics, supporting SDG 11 smart-city safety and resource management.

Project Overview
- Hardware: ESP32 + ultrasonic sensor + door switch (reed) + PIR sensor + buzzer + LEDs
- Cloud: GCP VM running Mosquitto, InfluxDB, and Grafana
- Visualization: Grafana live dashboard + analytics dashboard

Repository Contents
- final_code/final_code.ino: ESP32 firmware
- grafana_dashboard.json: live dashboard import file
- images/grafrana_dashboard.jpg: Grafana live dashboard screenshot
- images/vm dashbord.jpg: GCP VM instance screenshot
- images/Influx db.jpg: InfluxDB bucket setup screenshot

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
- Open firewall ports: 1883 (MQTT), 3000 (Grafana), 8086 (InfluxDB setup).
- Install Mosquitto:
  - sudo apt update
  - sudo apt install -y mosquitto mosquitto-clients
  - sudo mosquitto_passwd -c /etc/mosquitto/passwd lockeruser
  - sudo tee /etc/mosquitto/conf.d/locker.conf > /dev/null << 'EOF'
    listener 1883 0.0.0.0
    allow_anonymous false
    password_file /etc/mosquitto/passwd
    EOF
  - sudo systemctl restart mosquitto
  - sudo systemctl enable mosquitto
- Install InfluxDB, Telegraf, Grafana:
  - wget -q https://repos.influxdata.com/influxdata-archive_compat.key
  - sudo gpg --dearmor -o /usr/share/keyrings/influxdata-archive_compat.gpg influxdata-archive_compat.key
  - echo "deb [signed-by=/usr/share/keyrings/influxdata-archive_compat.gpg] https://repos.influxdata.com/ubuntu jammy stable" | sudo tee /etc/apt/sources.list.d/influxdata.list
  - sudo mkdir -p /etc/apt/keyrings
  - wget -qO- https://packages.grafana.com/gpg.key | sudo gpg --dearmor -o /etc/apt/keyrings/grafana.gpg
  - echo "deb [signed-by=/etc/apt/keyrings/grafana.gpg] https://packages.grafana.com/oss/deb stable main" | sudo tee /etc/apt/sources.list.d/grafana.list
  - sudo apt update
  - sudo apt install -y influxdb2 telegraf grafana
  - sudo systemctl enable --now influxdb telegraf grafana-server
- InfluxDB first-time setup:
  - Open http://<VM_EXTERNAL_IP>:8086
  - Create org: CPC357
  - Create bucket: CPC357_Project
  - Create token for Telegraf and Grafana
- Configure Telegraf (MQTT -> InfluxDB):
  - sudo tee /etc/telegraf/telegraf.d/locker_mqtt.conf > /dev/null << 'EOF'
    [[inputs.mqtt_consumer]]
      servers = ["tcp://127.0.0.1:1883"]
      topics = ["locker/telemetry"]
      username = "lockeruser"
      password = "YOUR_MQTT_PASSWORD"
      data_format = "json"
      json_string_fields = ["state"]

    [[outputs.influxdb_v2]]
      urls = ["http://127.0.0.1:8086"]
      token = "YOUR_INFLUX_TOKEN"
      organization = "CPC357"
      bucket = "CPC357_Project"
    EOF
  - sudo systemctl restart telegraf

3) Grafana Dashboard
- Open Grafana at http://<VM_EXTERNAL_IP>:3000.
- Add InfluxDB as a data source.
- Import grafana_dashboard.json for the live dashboard.

Dashboard Screenshot
- images/grafrana_dashboard.jpg
- images/vm dashbord.jpg
- images/Influx db.jpg

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
