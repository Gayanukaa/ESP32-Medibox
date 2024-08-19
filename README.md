# ESP32 Medibox

This project, **ESP32 Medibox**, is a smart medicine box designed to assist individuals in managing their medication schedules effectively. The system utilizes an ESP32 microcontroller to control various components, including sensors, actuators, and communication modules, to provide a comprehensive solution for medication management.

<img src = "project image.png"></img>

## Table of Contents

- [ESP32 Medibox](#esp32-medibox)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Components](#components)
  - [Circuit Diagram](#circuit-diagram)
  - [How It Works](#how-it-works)
  - [MQTT Topics](#mqtt-topics)
  - [Getting Started](#getting-started)
  - [Building](#building)
  - [Simulating](#simulating)
  - [Wokwi Library List](#wokwi-library-list)

## Features

- **WiFi Connectivity**: Connects to WiFi for remote monitoring and control via MQTT.
- **Temperature and Humidity Monitoring**: Measures environmental conditions using the DHT22 sensor.
- **Light Intensity Measurement**: Uses LDRs to detect ambient light intensity and adjust the servo motor accordingly.
- **Servo Motor Control**: Adjusts the position of a shaded sliding window based on light intensity.
- **Buzzer Notifications**: Provides audio alerts for scheduled medication times.
- **Button Interface**: Includes buttons for user interaction, such as canceling alarms or confirming actions.
- **Customizable Settings**: Adjustable minimum angle and control factor for servo motor via MQTT.

## Components

- **ESP32**: The main microcontroller used in this project.
- **DHT22**: Temperature and humidity sensor.
- **LDRs**: Light-dependent resistors for measuring light intensity.
- **Servo Motor**: For adjusting the position of the shaded window.
- **Buzzer**: For audio notifications.
- **Buttons**: For user interaction.

## Circuit Diagram

The complete circuit diagram and simulation can be found on [Wokwi](https://wokwi.com/projects/397603939222652929).

## How It Works

The ESP32 Medibox is designed to help users manage their medication by providing various features:

1. **Monitoring**: Continuously monitors temperature, humidity, and light intensity.
2. **Notifications**: Provides scheduled notifications through a buzzer to remind the user to take their medication.
3. **Remote Control**: Allows remote monitoring and adjustment of settings via MQTT.

The system is flexible, allowing for the customization of parameters such as the servo motor's minimum angle and controlling factor.

## MQTT Topics

The following MQTT topics are used in this project:

- **MQTT-ON-OFF**: To control the buzzer (ON/OFF).
- **MQTT-SCH-ON**: To manage the schedule for buzzer notifications.
- **MQTT-TEMP**: To receive temperature readings.
- **MQTT-LDR**: To receive light intensity data.
- **MQTT-MIN-ANG**: To set the minimum angle of the servo motor.
- **MQTT-CTRL-FAC**: To set the controlling factor for the servo motor.
- **MQTT-DROP-DOWN**: To select predefined settings for the servo motor.

## Getting Started

To get started with the ESP32 Medibox:

1. **Clone the Repository**: Clone this repository to your local machine using the following command:
   ```bash
   git clone https://github.com/Gayanukaa/ESP32-Medibox.git
   ```
2. **Open the Project**: Navigate to the project directory:
   ```bash
   cd ESP32-Medibox
   ```
3. **Install Dependencies**: Ensure you have the necessary libraries installed. If you are using the Arduino IDE, install the following libraries via the Library Manager:
   - WiFi
   - PubSubClient
   - DHTesp
   - ESP32Servo
   - ArduinoJson

   If you are using PlatformIO, the required libraries are specified in the `platformio.ini` file and will be installed automatically during the build process.

4. **Configure WiFi and MQTT**: Modify the `setupWifi()` and `setupMqtt()` functions in the code to include your WiFi credentials and MQTT broker details.

5. **Upload the Code**: Connect your ESP32 to your computer and upload the code using the Arduino IDE or PlatformIO.

## Building

This is a [PlatformIO](https://platformio.org) project. To build it, [install PlatformIO](https://docs.platformio.org/en/latest/core/installation/index.html), and then run the following command:

```bash
pio run
```

This command compiles the code and prepares it for uploading to the ESP32.

## Simulating

To simulate this project, install [Wokwi for VS Code](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode). Open the project directory in Visual Studio Code, then:

1. Press **F1** and select "Wokwi: Start Simulator".
2. The simulation environment will launch within VS Code, and you can interact with the simulated components.
3. To interact with the simulated HTTP server, open your web browser and navigate to [http://localhost:8180](http://localhost:8180).

Wokwi allows you to test the functionality of your project without needing the physical hardware, making it easier to debug and validate your code.

## Wokwi Library List

For a list of libraries compatible with the Wokwi simulator, see the [Wokwi Library Documentation](https://docs.wokwi.com/guides/libraries). These libraries are essential for simulating sensors, actuators, and other components used in your ESP32 Medibox project.