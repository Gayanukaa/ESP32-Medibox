# ESP32 Medibox

This project, **ESP32 Medibox**, is a smart medicine box designed to assist individuals in managing their medication schedules effectively. The system utilizes an ESP32 microcontroller to control various components, including sensors, actuators, and communication modules, to provide a comprehensive solution for medication management.

<p>
 <img src="https://img.shields.io/github/license/Gayanukaa/ESP32-Medibox?style=flat&color=0080ff" alt="license">
 <img src="https://img.shields.io/github/last-commit/Gayanukaa/ESP32-Medibox?style=flat&logo=git&logoColor=white&color=0080ff" alt="last-commit">
 <img src="https://img.shields.io/github/languages/top/Gayanukaa/ESP32-Medibox?style=flat&color=0080ff" alt="repo-top-language">
 <img src="https://img.shields.io/github/languages/count/Gayanukaa/ESP32-Medibox?style=flat&color=0080ff" alt="repo-language-count">
</p>
<p>
  <em>Developed with the software and tools below.</em>
</p>
<p>
 <img src="https://img.shields.io/badge/VS_Code-007ACC.svg?style=flat&logo=visual-studio-code&logoColor=white" alt="VS Code">
 <img src="https://img.shields.io/badge/C++-00599C.svg?style=flat&logo=c%2B%2B&logoColor=white" alt="C++">
 <img src="https://img.shields.io/badge/Node_Red-8F0000.svg?style=flat&logo=node-red&logoColor=white" alt="Node-RED">
 <img src="https://img.shields.io/badge/Arduino-00979D.svg?style=flat&logo=arduino&logoColor=white" alt="Arduino">
 <img src="https://img.shields.io/badge/Wokwi-18A497.svg?style=flat&logo=google&logoColor=white" alt="Wokwi">
 <img src="https://img.shields.io/badge/PlatformIO-FF7F32.svg?style=flat&logo=platformio&logoColor=white" alt="PlatformIO">
</p>


## Circuit Diagram

The complete circuit diagram and simulation can be found on [Wokwi](https://wokwi.com/projects/397603939222652929).

<img src = "images/project image.png"></img>

## Table of Contents

- [ESP32 Medibox](#esp32-medibox)
  - [Circuit Diagram](#circuit-diagram)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Components](#components)
  - [How It Works](#how-it-works)
  - [Node-Red Flow](#node-red-flow)
    - [Running Node-RED](#running-node-red)
    - [MQTT Topics](#mqtt-topics)
  - [Getting Started](#getting-started)
  - [Building](#building)
  - [Simulating](#simulating)
  - [License](#license)

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

## How It Works

The ESP32 Medibox is designed to help users manage their medication by providing various features:

1. **Monitoring**: Continuously monitors temperature, humidity, and light intensity.
2. **Notifications**: Provides scheduled notifications through a buzzer to remind the user to take their medication.
3. **Remote Control**: Allows remote monitoring and adjustment of settings via MQTT.

The system is flexible, allowing for the customization of parameters such as the servo motor's minimum angle and controlling factor.

## Node-Red Flow

The Node-RED flow for this project is available in the `node-red-flow.json` file. You can import this flow into your Node-RED instance to interact with the ESP32 Medibox via MQTT.

<img src = "images/node-red flow.png"></img>

### Running Node-RED

To visualize and control your ESP32 project using Node-RED, follow these steps:

1. **Install Node-RED**: 
   - If Node.js is not installed on your machine, [install it first](https://nodejs.org/).
   - Open a terminal and run the following command to install Node-RED:
     ```bash
     npm install -g --unsafe-perm node-red
     ```

2. **Start Node-RED**: 
   - Run the following command in your terminal to start Node-RED:
     ```bash
     node-red
     ```
   - Node-RED will start and can be accessed via a web browser at [http://localhost:1880](http://localhost:1880).

3. **Create and Deploy Flows**:
   - Use the Node-RED interface to create flows that interact with your ESP32.
   - Deploy your flows and visualize data or control components from your web browser.

With Node-RED, you can create sophisticated dashboards and control mechanisms, enhancing the interactivity and functionality of your ESP32 project.

### MQTT Topics

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

## License

This project is licensed under the [MIT License](https://choosealicense.com/licenses/mit/).
