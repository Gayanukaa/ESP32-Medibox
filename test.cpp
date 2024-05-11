#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

#define DHT_PIN 15
#define BUZZER 12
#define LDR1 34
#define LDR2 35
#define MOTOR 18

float minAngle = 30.0;   // Minimum angle of the shaded sliding window
float controlFac = 0.75; // Controlling factor used to calculate motor angle

Servo motor;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

DHTesp dhtSensor;

bool isScheduledON = false; // Flag to indicate if the schedule is enabled
unsigned long scheduledOnTime;

char tempAr[6];  // Array to store temperature as a string
char lightAr[6]; // Array to store light intensity as a string

void setup()
{
    Serial.begin(115200);
    setupWifi(); // Connect to WiFi network

    setupMqtt(); // Setup MQTT communication

    dhtSensor.setup(DHT_PIN, DHTesp::DHT22); // Setup DHT sensor

    timeClient.begin();
    timeClient.setTimeOffset(5.5 * 3600); // Set time offset (5.5 hours for Sri Lanka)

    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    pinMode(LDR1, INPUT);
    pinMode(LDR2, INPUT);

    motor.attach(MOTOR, 500, 2400); // Attach servo motor to pin
}

void loop()
{
    if (!mqttClient.connected())
    {
        connectToBroker(); // Connect to MQTT broker if not already connected
    }

    mqttClient.loop();

    updateTemperature(); // Read temperature from DHT sensor
    Serial.println(tempAr);
    mqttClient.publish("TEMP", tempAr); // Publish temperature to MQTT topic

    checkSchedule(); // Check if the scheduled time has arrived

    updateLightIntensity();               // Read light intensity from LDR
    mqttClient.publish("LIGHT", lightAr); // Publish light intensity to MQTT topic

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

    Serial.println(":");

    if (strcmp(topic, "MAIN-ON-OFF") == 0)
    {
        buzzerOn(payloadCharAr[0] == '1');
    }
    else if (strcmp(topic, "SCH-ESP-ON") == 0)
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
    if (strcmp(topic, "MINIMUM-ANG") == 0)
    {
        minAngle = atof(payloadCharAr);
        Serial.println(minAngle);
    }
    if (strcmp(topic, "CONTROL-FAC") == 0)
    {
        controlFac = atof(payloadCharAr);
        Serial.println(controlFac);
    }
    if (strcmp(topic, "DROP-DOWN") == 0)
    {
        if (payloadCharAr[0] == 'A')
        {
            minAngle = 20;
            controlFac = 0.5;
            Serial.println(minAngle);
            Serial.println(controlFac);
        }
        else if (payloadCharAr[0] == 'B')
        {
            minAngle = 30;
            controlFac = 0.7;
            Serial.println(minAngle);
            Serial.println(controlFac);
        }
        else if (payloadCharAr[0] == 'C')
        {
            minAngle = 25;
            controlFac = 0.8;
            Serial.println(minAngle);
            Serial.println(controlFac);
        }
        else if (payloadCharAr[0] == 'M')
        {
            if (strcmp(topic, "MINIMUM-ANG") == 0)
            {
                minAngle = atof(payloadCharAr);
                Serial.println(minAngle);
            }
            if (strcmp(topic, "CONTROL-FAC") == 0)
            {
                controlFac = atof(payloadCharAr);
                Serial.println(controlFac);
            }
        }
    }
}

void connectToBroker()
{
    while (!mqttClient.connected())
    {
        Serial.println("Attempting MQTT connetion...");
        if (mqttClient.connect("ESP-9813247900"))
        {
            Serial.println("Connected");
            mqttClient.subscribe("MAIN-ON-OFF");
            mqttClient.subscribe("SCH-ESP-ON");
            mqttClient.subscribe("DROP-DOWN");
            mqttClient.subscribe("MINIMUM-ANG");
            mqttClient.subscribe("CONTROL-FAC");
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
}

void updateLightIntensity()
{
    const float analogMinValue = 0.0;    // Minimum analogRead value
    const float analogMaxValue = 1023.0; // Maximum analogRead value
    const float intensityMin = 0.0;      // Minimum intensity value
    const float intensityMax = 1.0;      // Maximum intensity value

    float rightLDR = analogRead(LDR1);
    float leftLDR = analogRead(LDR2);
    float intensity = 0;

    if (rightLDR > leftLDR)
    {
        if (rightLDR <= 1023)
        {
            intensity = (rightLDR - analogMinValue) / (analogMaxValue - analogMinValue);
            Serial.println("rightLDR " + String(rightLDR) + "  " + String(intensity));
            mqttClient.publish("MAX-LIGHT-INTENSITY", "RIGHT LDR");
            String(intensity, 2).toCharArray(lightAr, 6);
            AdjustServoMotor(intensity, 0.5); // Adjust the position of the shaded sliding window based on light intensity
        }
        else
        {
            intensity = 1;
            String(intensity, 2).toCharArray(lightAr, 6);
            Serial.println("rightLDR " + String(rightLDR) + "  " + String(intensity));
            mqttClient.publish("MAX-LIGHT-INTENSITY", "RIGHT LDR");
            AdjustServoMotor(intensity, 0.5); // Adjust the position of the shaded sliding window based on light intensity
        }
    }
    else
    {
        if (leftLDR <= 1023)
        {
            intensity = (leftLDR - analogMinValue) / (analogMaxValue - analogMinValue);
            Serial.println("leftLDR " + String(leftLDR) + "  " + String(intensity));
            mqttClient.publish("MAX-LIGHT-INTENSITY", "LEFT LDR");
            String(intensity, 2).toCharArray(lightAr, 6);
            AdjustServoMotor(intensity, 1.5);
        }
        else
        {
            intensity = 1;
            String(intensity, 2).toCharArray(lightAr, 6);
            Serial.println("leftLDR " + String(leftLDR) + "  " + String(intensity));
            mqttClient.publish("MAX-LIGHT-INTENSITY", "LEFT LDR");
            AdjustServoMotor(intensity, 1.5); // Adjust the position of the shaded sliding window based on light intensity
        }
        // Adjust the position of the shaded sliding window based on light intensity
    }
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
    return timeClient.getEpochTime(); // Get current time from NTP server
}

void checkSchedule()
{
    if (isScheduledON)
    {
        unsigned long currentTime = getTime();
        Serial.println("currentTime " + String(currentTime));
        Serial.println("scheduledOnTime " + String(scheduledOnTime));
        if (currentTime > scheduledOnTime)
        {
            buzzerOn(true);
            isScheduledON = false;
            mqttClient.publish("MAIN-ON-OFF", "1");
            mqttClient.publish("SCH-ESP-ON", "0");
            Serial.println("Scheduled ON");
        }
    }
}

void AdjustServoMotor(double lightintensity, double D)
{
    double angle = minAngle * D + (180.0 - minAngle) * lightintensity * controlFac; // Calculate the angle based on light intensity
    Serial.println(angle);
    motor.write(angle); // Set the angle of the servo motor to adjust the shaded sliding window
}