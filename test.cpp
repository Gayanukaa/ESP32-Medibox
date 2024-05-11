#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

const int DHT_PIN = 15;
#define BUZZER 12
#define LDR1 34
#define LDR2 35
#define MOTOR 18

float minAngle = 30.0;       // Minimum angle
float controllingFac = 0.75; // Controlling factor

Servo motor;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
JsonDocument packet;

DHTesp dhtSensor;

bool isScheduledON = false; // Flag to indicate if the schedule is enabled
unsigned long scheduledOnTime;

char tempAr[6];  // Store temperature
char motorAr[6]; // Store light intensity
const float analogMinValue = 0.0;
const float analogMaxValue = 1023.0;
const float intensityMin = 0.0;
const float intensityMax = 1.0;

void setup()
{
    Serial.begin(115200);

    setupWifi();
    setupMqtt();

    dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
    timeClient.begin();
    timeClient.setTimeOffset(5.5 * 3600);

    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    pinMode(LDR1, INPUT);
    pinMode(LDR2, INPUT);

    motor.attach(MOTOR, 600, 2400);
}

void loop()
{
    if (!mqttClient.connected())
    {
        connectToBroker();
    }

    mqttClient.loop();

    updateTemperature();

    checkSchedule();

    updateLightIntensity(); // Read light intensity from LDR

    delay(1000);
}

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

void setupMqtt()
{
    mqttClient.setServer("test.mosquitto.org", 1883);
    mqttClient.setCallback(receiveCallback);
}

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
        }
    }
    if (strcmp(topic, "MQTT-MIN-ANG") == 0)
    {
        minAngle = atof(payloadCharAr);
        Serial.println(minAngle);
    }
    if (strcmp(topic, "MQTT-CTRL-FAC") == 0)
    {
        controllingFac = atof(payloadCharAr);
        Serial.println(controllingFac);
    }
    if (strcmp(topic, "MQTT-CHOOSE-MED") == 0)
    {
        if (payloadCharAr[0] == 'A')
        {
            minAngle = 30;
            controllingFac = 0.5;
            Serial.println(minAngle);
            Serial.println(controllingFac);
        }
        else if (payloadCharAr[0] == 'B')
        {
            minAngle = 45;
            controllingFac = 0.3;
            Serial.println(minAngle);
            Serial.println(controllingFac);
        }
        else if (payloadCharAr[0] == 'C')
        {
            minAngle = 60;
            controllingFac = 0.8;
            Serial.println(minAngle);
            Serial.println(controllingFac);
        }
        else if (payloadCharAr[0] == 'X')
        {
            if (strcmp(topic, "MQTT-MIN-ANG") == 0)
            {
                minAngle = atof(payloadCharAr);
                Serial.println(minAngle);
            }
            if (strcmp(topic, "MQTT-CTRL-FAC") == 0)
            {
                controllingFac = atof(payloadCharAr);
                Serial.println(controllingFac);
            }
        }
    }
}

void connectToBroker()
{
    while (!mqttClient.connected())
    {
        Serial.println("Attempting MQTT connetion...");
        if (mqttClient.connect("ESP-55200255"))
        {
            Serial.println("Connected");
            mqttClient.subscribe("MQTT-ON-OFF");
            mqttClient.subscribe("MQTT-SCH-ON");
            mqttClient.subscribe("MQTT-CHOOSE-MED");
            mqttClient.subscribe("MQTT-MIN-ANG");
            mqttClient.subscribe("MQTT-CTRL-FAC");
        }
        else
        {
            Serial.println("failed");
            Serial.println(mqttClient.state());
            delay(5000);
        }
    }
}

void updateTemperature()
{
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    String(data.temperature, 2).toCharArray(tempAr, 6);

    // Serial.println("Temperature is " + String(tempAr) + "°C");
    mqttClient.publish("MQTT-TEMP", tempAr);
    delay((1000));
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
        if (rightLDR <= 1023)
        {
            intensity = (rightLDR - analogMinValue) / (analogMaxValue - analogMinValue);
            // Serial.print("Right LDR " + String(intensity) + " intensity");
            AdjustServoMotor(intensity, 0.5);
        }
        else
        {
            intensity = 1;
            // Serial.print("Right LDR " + String(intensity) + " intensity");
            AdjustServoMotor(intensity, 0.5);
        }
    }
    else
    {
        packet["LDR"] = "Left LED";
        if (leftLDR <= 1023)
        {
            intensity = (leftLDR - analogMinValue) / (analogMaxValue - analogMinValue);
            // Serial.print("Left LDR " + String(intensity) + " intensity");
            AdjustServoMotor(intensity, 1.5);
        }
        else
        {
            intensity = 1;
            // Serial.print("Left LDR " + String(intensity) + " intensity");
            AdjustServoMotor(intensity, 1.5);
        }
    }

    packet["Intensity"] = String(intensity);
    serializeJson(packet, dataJson);
    // Serial.println(dataJson);

    mqttClient.publish("MQTT-LIGHT-INTENSITY", dataJson);
}

void setupWifi()
{
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println("Wokwi-GUEST");
    WiFi.begin("Wokwi-GUEST", "");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("Wifi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

unsigned long getTime()
{
    timeClient.update();
    return timeClient.getEpochTime();
}

void checkSchedule()
{
    if (isScheduledON)
    {
        unsigned long currentTime = getTime();
        Serial.println("Current Time is " + String(currentTime));
        Serial.println("Scheduled Time is " + String(scheduledOnTime));
        if (currentTime > scheduledOnTime)
        {
            buzzerOn(true);
            isScheduledON = false;
            mqttClient.publish("MQTT-SCH-OFF-ESP", "0");
            Serial.println("Schedule ON");
        }
    }
}

void AdjustServoMotor(double lightintensity, double D)
{
    double angle = minAngle * D + (180.0 - minAngle) * lightintensity * controllingFac; // Calculate the angle based on light intensity
    angle = min(angle, 180.0);
    Serial.println(" and new angle: " + String(angle) + "°");
    motor.write(angle); // Set the angle of the servo motor to adjust the shaded sliding window
    String(angle - 90, 2).toCharArray(motorAr, 6);
    mqttClient.publish("MQTT-MOTOR-ANG", motorAr);
}