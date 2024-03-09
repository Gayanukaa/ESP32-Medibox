#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

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

// User interface configuration
int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set   Time Zone", "2 - Set   Alarm 1", "3 - Set   Alarm 2", "4 - Set   Alarm 3", "5- Disable Alarms"};

void setup() {
  // Initialize DHT Sensor
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  // Set up pin modes
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_TEMP, OUTPUT);
  pinMode(LED_HUM, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  // Initialize serial monitor and OLED display
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Turn on OLED display
  display.display();
  delay(2000);

  // Connect to Wi-Fi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI..", 0, 10, 2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0, 10, 2);

  // Configure NTP time synchronization
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();

  print_line("Welcome to MediBox!", 0, 10, 2);
  display.clearDisplay();
}

void loop() {
  // Update time and check for alarms
  update_time_with_check_alarm();

  // Check if the OK button is pressed to navigate to the menu
  if (digitalRead(PB_OK) == LOW) {
    delay(200); // Debouncing push button
    go_to_menu();
  }

  // Check temperature and humidity
  check_temp_and_hum();
}

// Function to print a line on the OLED display
void print_line(String text, int column, int row, int text_size) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
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
