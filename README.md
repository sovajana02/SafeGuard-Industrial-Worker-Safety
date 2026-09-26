# SafeGuard: IoT-Based Industrial Worker Safety & Gas Detection System

## Team SparkSync

SafeGuard is an IoT-based industrial worker safety system designed for real-time monitoring of gas leakage, temperature, worker presence, and distance from hazardous equipment.

## Key Features

- Toxic / flammable gas detection using MQ-2
- Temperature monitoring using DHT11
- Worker presence and distance detection using Ultrasonic Sensor
- Real-time LCD monitoring
- ESP32 Web Server monitoring
- Distance-based buzzer warning

## System Architecture

Sensors → Arduino UNO → RX/TX Serial Communication → ESP32 → Web Server

The system provides monitoring through the LCD and ESP32 Web Server.

## Smart Safety Logic

When gas leakage is detected:

- Very close worker → High / Loud Alarm
- Moderate distance → Medium Alarm
- No nearby worker → Monitoring continues

Temperature abnormalities also generate a warning.

## Hardware Used

- Arduino UNO
- ESP32
- MQ-2 Gas Sensor
- DHT11 Temperature Sensor
- Ultrasonic Sensor
- LCD with I2C
- Buzzer
- Breadboard and jumper wires

## How It Works

1. Sensors collect environmental and distance data.
2. Arduino UNO reads and processes the sensor values.
3. Data is transferred to ESP32 using RX/TX serial communication.
4. ESP32 provides real-time monitoring through the Web Server.
5. LCD displays the monitoring information.
6. The buzzer provides a distance-based warning during hazardous conditions.

## Team Members

### SparkSync

- Sova Jana
- Papon Chowdhury
- Sohom Sar
- Sayantika Ghosh
- Subhajit Saha
- Sohom Sar
- Sayantika Ghosh
- Subhajit Saha
