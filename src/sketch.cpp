#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include "sketch.ino"

// Function Prototypes
void setupMqtt();
void connectToBroker();
void receiveCallback(char *topic, byte *payload, unsigned int length);
void buzzerOn(bool on);
void updateTemperature();
void updateLightIntensity();
void AdjustServoMotor(float intensity, float controllingFactor);
void checkSchedule();

// Pin Definitions
#define BUZZER 4
#define LED_1 15
#define LED_TEMP 13
#define LED_HUM 2
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 14
#define LDR1 5  // Left LDR
#define LDR2 25 // Right LDR
#define MOTOR 18

// Variables for controlling the servo motor
float minAngle = 30.0;       // Minimum angle
float controllingFac = 0.75; // Controlling factor
float customAngle;
float customControlFac;

// Initializing clients and objects
Servo motor;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
DynamicJsonDocument packet(1024);
DHTesp dhtSensor;

// Variables for schedule
bool isScheduledON = false; // Indicate if the schedule is enabled
unsigned long scheduledOnTime;

// Arrays to store data for MQTT messages
char tempAr[6];
char motorAr[6];
char angleAr[6];
char ctrlAr[6];

// Constants for analog reading
const float analogMinValue = 0.0;
const float analogMaxValue = 1023.0;
const float intensityMin = 0.0;
const float intensityMax = 1.0;

void setup()
{
    Serial.begin(115200);

    setupWifi();
    setupMqtt();

    dhtSensor.setup(DHTPIN, DHTesp::DHT22);
    timeClient.begin();
    timeClient.setTimeOffset(5.5 * 3600);

    // Setup pins for components
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);
    pinMode(LDR1, INPUT);
    pinMode(LDR2, INPUT);
    motor.attach(MOTOR, 600, 2400);

    // Publish initial configuration to MQTT
    customAngle = 30;
    customControlFac = 0;
    mqttClient.publish("MQTT-SET-ANG", "30");
    mqttClient.publish("MQTT-SET-FAC", "0.75");
}

void loop()
{
    if (!mqttClient.connected())
    {
        connectToBroker();
    }

    // Maintain MQTT connection
    mqttClient.loop();

    updateTemperature();

    checkSchedule();

    updateLightIntensity(); // Read light intensity from LDR

    delay(1000);
}

// Setup MQTT client
void setupMqtt()
{
    mqttClient.setServer("test.mosquitto.org", 1883);
    mqttClient.setCallback(receiveCallback);
}

// Connect to MQTT broker
void connectToBroker()
{
    while (!mqttClient.connected())
    {
        Serial.println("Attempting MQTT connection...");
        if (mqttClient.connect("ESP-55200255"))
        {
            Serial.println("Connected");
            mqttClient.subscribe("MQTT-ON-OFF");
            mqttClient.subscribe("MQTT-SCH-ON");
            mqttClient.subscribe("MQTT-DROP-DOWN");
            mqttClient.subscribe("MQTT-MIN-ANG");
            mqttClient.subscribe("MQTT-CTRL-FAC");
        }
        else
        {
            Serial.print("Connection failed with state: ");
            Serial.println(mqttClient.state());
            delay(5000);
        }
    }
}

// MQTT receive callback function
void receiveCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    char payloadCharAr[length];

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
        payloadCharAr[i] = (char)payload[i];
    }

    Serial.println();

    // Process received MQTT messages
    if (strcmp(topic, "MQTT-ON-OFF") == 0)
    {
        buzzerOn(payloadCharAr[0] == '1');
    }
    else if (strcmp(topic, "MQTT-SCH-ON") == 0)
    {
        if (payloadCharAr[0] == 'N')
        {
            isScheduledON = false;
        }
        else
        {
            isScheduledON = true;
            scheduledOnTime = atol(payloadCharAr);
            Serial.println("Schedule ON");
            Serial.println("Scheduled Time is " + String(scheduledOnTime));
        }
    }
    // Update minimum angle
    if (strcmp(topic, "MQTT-MIN-ANG") == 0)
    {
        minAngle = atof(payloadCharAr);
        Serial.println(minAngle);
    }
    // Update controlling factor
    if (strcmp(topic, "MQTT-CTRL-FAC") == 0)
    {
        controllingFac = atof(payloadCharAr);
        Serial.println(controllingFac);
    }
    // Update minimum angle and controlling factor based on drop-down selection
    if (strcmp(topic, "MQTT-DROP-DOWN") == 0)
    {
        if (payloadCharAr[0] == 'D')
        { // Default
            minAngle = 30;
            controllingFac = 0.75;
            Serial.println("Angle:" + String(minAngle) + " and Control Factor: " + String(controllingFac));
            mqttClient.publish("MQTT-SET-ANG", "30");
            mqttClient.publish("MQTT-SET-FAC", "0.75");
        }
        else if (payloadCharAr[0] == 'A')
        {
            minAngle = 30;
            controllingFac = 0.5;
            Serial.println("Angle:" + String(minAngle) + " and Control Factor: " + String(controllingFac));
            mqttClient.publish("MQTT-SET-ANG", "30");
            mqttClient.publish("MQTT-SET-FAC", "0.5");
        }
        else if (payloadCharAr[0] == 'B')
        {
            minAngle = 45;
            controllingFac = 0.3;
            Serial.println("Angle:" + String(minAngle) + " and Control Factor: " + String(controllingFac));
            mqttClient.publish("MQTT-SET-ANG", "45");
            mqttClient.publish("MQTT-SET-FAC", "0.3");
        }
        else if (payloadCharAr[0] == 'C')
        {
            minAngle = 60;
            controllingFac = 0.8;
            Serial.println("Angle:" + String(minAngle) + " and Control Factor: " + String(controllingFac));
            mqttClient.publish("MQTT-SET-ANG", "60");
            mqttClient.publish("MQTT-SET-FAC", "0.8");
        }
        else if (payloadCharAr[0] == 'X')
        { // Custom
            String(customAngle, 2).toCharArray(angleAr, 6);
            String(customControlFac, 2).toCharArray(ctrlAr, 6);
            mqttClient.publish("MQTT-SET-ANG", angleAr);
            mqttClient.publish("MQTT-SET-FAC", ctrlAr);

            if (strcmp(topic, "MQTT-MIN-ANG") == 0)
            {
                minAngle = atof(payloadCharAr);
                customAngle = minAngle;
                String(minAngle, 2).toCharArray(angleAr, 6);
                mqttClient.publish("MQTT-SET-ANG", angleAr);
            }
            if (strcmp(topic, "MQTT-CTRL-FAC") == 0)
            {
                controllingFac = atof(payloadCharAr);
                customControlFac = controllingFac;
                String(customControlFac, 2).toCharArray(ctrlAr, 6);
                mqttClient.publish("MQTT-SET-FAC", ctrlAr);
            }
        }
    }
}

// Function to turn the buzzer on or off
void buzzerOn(bool on)
{
    if (on)
    {
        tone(BUZZER, 256);
    }
    else
    {
        noTone(BUZZER);
    }
}

// Update temperature reading and publish to MQTT
void updateTemperature()
{
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    String(data.temperature, 2).toCharArray(tempAr, 6);
    mqttClient.publish("MQTT-TEMP", tempAr);
    delay(1000);
}

// Adjust the position of the shaded sliding window based on light intensity
void updateLightIntensity()
{
    float rightLDR = analogRead(LDR1);
    float leftLDR = analogRead(LDR2);
    float intensity = 0;
    char dataJson[50];

    if (rightLDR > leftLDR)
    {
        packet["LDR"] = "Right LED";
        if (rightLDR - leftLDR > 30)
        {
            intensity = (rightLDR - leftLDR) / (analogMaxValue - analogMinValue) * (intensityMax - intensityMin) + intensityMin;
            AdjustServoMotor(intensity, controllingFac);
        }
    }
    else
    {
        packet["LDR"] = "Left LED";
        if (leftLDR - rightLDR > 30)
        {
            intensity = (leftLDR - rightLDR) / (analogMaxValue - analogMinValue) * (intensityMax - intensityMin) + intensityMin;
            AdjustServoMotor(intensity, controllingFac);
        }
    }

    String(intensity, 2).toCharArray(motorAr, 6);
    packet["LDR-Value"] = String(intensity, 2);
    serializeJson(packet, dataJson);
    mqttClient.publish("MQTT-LDR", dataJson);
    delay(1000);
}

// Adjust the servo motor based on light intensity
void AdjustServoMotor(float intensity, float controllingFactor)
{
    motor.write(minAngle + controllingFactor * intensity * (90 - minAngle));
}

// Check if the scheduled time is reached and turn on the buzzer
void checkSchedule()
{
    timeClient.update();

    if (isScheduledON && scheduledOnTime == timeClient.getEpochTime())
    {
        isScheduledON = false;
        mqttClient.publish("MQTT-SCH-ON", "N");
        buzzerOn(true);
        delay(1000);
    }
}
