#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

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
#define LDR1 5 //left
#define LDR2 25 //right
#define MOTOR 18

// Define variables for controlling the servo motor
float minAngle = 30.0;       // Minimum angle
float controllingFac = 0.75; // Controlling factor
float customAngle;
float customControlFac;

//Initialise clients and objects
Servo motor;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
JsonDocument packet;

// NTP Configuration
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0
int UTC_OFFSET = 19800;
int offsets[] = {0, 0} ; //Default set for IST time zone
int offset_hours = 5;
int offset_minutes = 30;

// Initialize DHT Sensor
DHTesp dhtSensor;

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global variables for time
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

// Alarm configuration
bool alarm_enabled = true;
int n_alarms = 3;
//centred number for easy choose
int alarm_hours[] = {12, 12, 12};
int alarm_minutes[] = {27, 27, 27};
bool alarm_triggered[] = {false, false, false};
//TODO: make isScheduledON like above and make all 3 alarms schedulable

// Variables for schedule
bool isScheduledON = false; // Indicate if the schedule is enabled
unsigned long scheduledOnTime;

// Musical notes for the buzzer
int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

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

// User interface configuration
int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set   Time Zone", "2 - Set   Alarm 1", "3 - Set   Alarm 2", "4 - Set   Alarm 3", "5- Disable Alarms"};

void setup() {
  // Set up pin modes
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_TEMP, OUTPUT);
  pinMode(LED_HUM, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);
  motor.attach(MOTOR, 600, 2400);

  // Initialize serial monitor and OLED display
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  setupWifi();
  setupMqtt();

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  timeClient.begin();
  timeClient.setTimeOffset(5.5 * 3600);

  // Turn on OLED display
  display.display();
  delay(2000);

  // Configure NTP time synchronization
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();

  print_line("Welcome to MediBox!", 0, 10, 2);
  display.clearDisplay();

  // Publish initial configuration to MQTT
  // connectToBroker();
  customAngle = 30;
  customControlFac = 0;
  mqttClient.publish("MQTT-SET-ANG", "30");
  mqttClient.publish("MQTT-SET-FAC", "0.75");
}

void loop() {
  //TODO: optimise all these processes
  if (!mqttClient.connected()){
    connectToBroker();
  }
  // Update time and check for alarms
  update_time_with_check_alarm();

  checkSchedule(); //TODO: Integrate check schedule with update_time_with_check_alarm

  updateLightIntensity(); // Read light intensity from LDR

  // Check if the OK button is pressed to navigate to the menu
  if (digitalRead(PB_OK) == LOW) {
    delay(200); // Debouncing push button
    go_to_menu();
  }

  // Check temperature and humidity
  check_temp_and_hum(); //Try to remove this
  updateTemperature();
  //TODO: Implement updateHumidity();
}

// Setup WiFi connection
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

// Setup MQTT client
void setupMqtt()
{
    mqttClient.setServer("test.mosquitto.org", 1883);
    mqttClient.setCallback(receiveCallback);
}

// Function to print a line on the OLED display
void print_line(String text, int column, int row, int text_size) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
}

// Connect to MQTT broker
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
        // Handle different drop-down options
        if (payloadCharAr[0] == 'D') //default
        {
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
        else if (payloadCharAr[0] == 'X') //Custom
        {
            String(customAngle, 2).toCharArray(angleAr, 6);
            String(customControlFac, 2).toCharArray(ctrlAr, 6);
            mqttClient.publish("MQTT-SET-ANG", angleAr);
            mqttClient.publish("MQTT-SET-FAC", ctrlAr);

            if (strcmp(topic, "MQTT-MIN-ANG") == 0)
            {
                minAngle = atof(payloadCharAr);
                customAngle = minAngle;
                //Serial.println("New custom angle is: " + String(minAngle));
                String(minAngle, 2).toCharArray(angleAr, 6);
                mqttClient.publish("MQTT-SET-ANG", angleAr);
            }
            if (strcmp(topic, "MQTT-CTRL-FAC") == 0)
            {
                controllingFac = atof(payloadCharAr);
                customControlFac = controllingFac;
                String(customControlFac, 2).toCharArray(ctrlAr, 6);
                //Serial.println("New custom factor is: " + String(controllingFac));
                mqttClient.publish("MQTT-SET-FAC", ctrlAr);
            }
        }
    }
}

// Function to print the current time on the OLED display
void print_time_now(void) {
  display.clearDisplay();

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeDay[4];
  strftime(timeDay, 4, "%a", &timeinfo);  //day of the week
  print_line(String(timeDay), 10, 0, 2);

  print_line(" ", 30, 0, 2);

  char timeDate[3];
  strftime(timeDate, 3, "%d", &timeinfo);  //day of the month
  print_line(String(timeDate), 50, 0, 2);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);  //hour in 24-hour format
  print_line(String(timeHour), 10, 25, 2);

  print_line(":", 30, 25, 2);

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);  //minute
  print_line(String(timeMinute), 40, 25, 2);

  print_line(":", 60, 25, 2);

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);  //second
  print_line(String(timeSecond), 70, 25, 2);

  display.display();
}

// Function to update the time
void update_time() {
  struct tm timeinfo;
  getLocalTime(&timeinfo, UTC_OFFSET);

  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
}

// Function to ring the alarm
void ring_alarm() {
  display.clearDisplay();
  print_line("TAKE YOUR MEDICINE!", 0, 0, 2);

  digitalWrite(LED_1, HIGH);

  bool break_happened = false;

  // Ring the buzzer
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

// Function to update time and check for alarms
void update_time_with_check_alarm(void) {
  update_time();
  print_time_now();

  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }
}

void checkSchedule()
{
    if (isScheduledON)
    {
        unsigned long currentTime = getTime();
        if (currentTime > scheduledOnTime)
        {
            buzzerOn(true);
            mqttClient.publish("MQTT-SCH-OFF-ESP", "0");
            Serial.println("Current Time is " + String(currentTime));
            Serial.println("Scheduled Time is " + String(scheduledOnTime));
            isScheduledON = false;
        }
    }
}

// Get current time from NTP server
unsigned long getTime()
{
    timeClient.update();
    return timeClient.getEpochTime();
}


// Function to wait for a button press
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    } else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    } else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    } else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }

    update_time();
  }
}

// Function to navigate to the menu
void go_to_menu() {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    } else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      run_mode(current_mode);
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

// Function to set an alarm
void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];
  alarm_enabled = true;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);
}

// Function to run the selected mode
void run_mode(int mode) {
  if (mode == 0) {
    set_timezone();
    UTC_OFFSET = 0;
    if (offsets[0] < 0 ) {
      UTC_OFFSET = (UTC_OFFSET + (offsets[0] * 60 * 60  - 1 * offsets[1] * 60));
    }
    else {
      UTC_OFFSET = UTC_OFFSET + (offsets[0] * 60 * 60 + offsets[1] * 60);
    }
    configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  } else if (mode == 1 || mode == 2 || mode == 3) {
    set_alarm(mode - 1);
  } else if (mode == 4) {
    alarm_enabled = false;
    display.clearDisplay();
    delay(200);
    print_line("Alarms Disabled", 0, 0, 2);
    delay(1000);
    display.clearDisplay();
  }
}

// Function to check temperature and humidity
void check_temp_and_hum() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature > 32) {
    digitalWrite(LED_TEMP, HIGH);
    print_line("TEMPETATURE HIGH", 10, 45, 1);
    digitalWrite(LED_TEMP,LOW);
  } else if (data.temperature < 26) {
    digitalWrite(LED_TEMP, HIGH);
    print_line("TEMPETATURE LOW", 10, 45, 1);
    digitalWrite(LED_TEMP,LOW);
  }

  if (data.humidity > 80) {
    digitalWrite(LED_HUM, HIGH);
    print_line("HUMIDITY HIGH", 10, 55, 1);
    digitalWrite(LED_HUM, LOW);
  } else if (data.humidity < 60) {
    digitalWrite(LED_HUM, HIGH);
    print_line("HUMIDITY LOW", 10, 55, 1);
    digitalWrite(LED_HUM, LOW);
  }
}

// Update temperature reading and publish to MQTT
void updateTemperature()
{
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    String(data.temperature, 2).toCharArray(tempAr, 6);

    //Serial.println("Temperature is " + String(tempAr) + "°C");
    mqttClient.publish("MQTT-TEMP", tempAr);
    delay((1000));
}

// Function to change timezone
void set_timezone() {
  int temp_offset_hours = offset_hours;
  while (true){
    display.clearDisplay();
    print_line("Enter offset hours: " + String(temp_offset_hours), 0, 0, 2);

    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      delay(200);
      temp_offset_hours += 1;
      temp_offset_hours = temp_offset_hours % 15; //max time zone was 14
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_offset_hours -= 1;
      if (temp_offset_hours < -12) {
        temp_offset_hours = 0;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      offset_hours = temp_offset_hours;
      offsets[0] = offset_hours;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_offset_minutes = offset_minutes;
  while (true) {
    display.clearDisplay();
    print_line("Enter offset minutes: " + String(temp_offset_minutes), 0, 0, 2);

    int pressed = wait_for_button_press(); // when a button is pressed it is assigned to this

    if (pressed == PB_UP) {
      delay(200);
      temp_offset_minutes += 15;
      temp_offset_minutes = temp_offset_minutes % 60;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_offset_minutes -= 15;
      if (temp_offset_minutes < 0) {
        temp_offset_minutes = 60;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      offset_minutes = temp_offset_minutes;
      offsets[1] = offset_minutes;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Timezone  is Set", 0, 0, 2);
  delay(1000);
}

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
           //Serial.print("Right LDR " + String(intensity) + " intensity");
            AdjustServoMotor(intensity, 0.5);
        }
        else
        {
            intensity = 1;
            //Serial.print("Right LDR " + String(intensity) + " intensity");
            AdjustServoMotor(intensity, 0.5);
        }
    }
    else
    {
        packet["LDR"] = "Left LED";
        if (leftLDR <= 1023)
        {
            intensity = (leftLDR - analogMinValue) / (analogMaxValue - analogMinValue);
            //Serial.print("Left LDR " + String(intensity) + " intensity");
            AdjustServoMotor(intensity, 1.5);
        }
        else
        {
            intensity = 1;
            //Serial.print("Left LDR " + String(intensity) + " intensity");
            AdjustServoMotor(intensity, 1.5);
        }
    }

    packet["Intensity"] = String(intensity);
    serializeJson(packet, dataJson);
    //Serial.println(dataJson);

    mqttClient.publish("MQTT-LIGHT-INTENSITY", dataJson);
}

// Adjust servo motor position based on light intensity and distance from minAngle
void AdjustServoMotor(double lightintensity, double D)
{
    double angle = minAngle * D + (180.0 - minAngle) * lightintensity * controllingFac; // Calculate the angle based on light intensity
    angle = min(angle, 180.0);
    // Serial.println(" and new angle: " + String(angle) + "°");
    motor.write(angle);
    String(angle-90, 2).toCharArray(motorAr, 6);
    mqttClient.publish("MQTT-MOTOR-ANG", motorAr);
}