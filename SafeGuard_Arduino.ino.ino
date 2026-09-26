#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// =====================================================
// LCD
// =====================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// DHT11
// =====================================================
#define DHT_PIN 2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// =====================================================
// HC-SR04
// =====================================================
#define TRIG_PIN 9
#define ECHO_PIN 10

// =====================================================
// MQ-2
// =====================================================
#define MQ2_PIN A0

// =====================================================
// BUZZER
// =====================================================
#define BUZZER_PIN 3

// =====================================================
// PCF8574T
// =====================================================
#define PCF8574_ADDR 0x27

// =====================================================
// VARIABLES
// =====================================================
float distance = -1;
float temperature = NAN;
float humidity = NAN;

int mq2Value = 0;
bool pcfStatus = false;

unsigned long lastSensorRead = 0;
unsigned long lastDHTRead = 0;
unsigned long lastPageChange = 0;
unsigned long lastServerSend = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastLCDRefresh = 0;

const unsigned long SENSOR_INTERVAL = 500;
const unsigned long DHT_INTERVAL = 2000;
const unsigned long PAGE_INTERVAL = 3000;
const unsigned long SERVER_INTERVAL = 2000;
const unsigned long LCD_REFRESH_INTERVAL = 500;

int currentPage = 0;

bool buzzerOn = false;
unsigned long lastBeep = 0;

// =====================================================
// READ HC-SR04
// =====================================================
float readDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration =
    pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
  {
    return -1;
  }

  float cm = duration * 0.0343 / 2.0;

  if (cm < 2 || cm > 400)
  {
    return -1;
  }

  return cm;
}

// =====================================================
// CHECK PCF8574T
// =====================================================
bool checkPCF()
{
  Wire.beginTransmission(PCF8574_ADDR);

  byte error = Wire.endTransmission();

  return (error == 0);
}

// =====================================================
// STOP BUZZER
// =====================================================
void stopBuzzer()
{
  noTone(BUZZER_PIN);
  buzzerOn = false;
}

// =====================================================
// BUZZER CONTROL
// =====================================================
void buzzerControl()
{
  unsigned long now = millis();

  // ABOVE 40 CM OR INVALID DISTANCE
  if (distance < 0 || distance > 40)
  {
    stopBuzzer();
    lastBeep = now;
    return;
  }

  // =================================================
  // 0-20 CM: MAXIMUM CONTINUOUS SOUND
  // =================================================
  if (distance <= 20)
  {
    if (!buzzerOn)
    {
      tone(BUZZER_PIN, 2500);
      buzzerOn = true;
    }

    return;
  }

  // =================================================
  // 20-30 CM: AVERAGE BEEP
  // =================================================
  if (distance <= 30)
  {
    if (buzzerOn && now - lastBeep >= 200)
    {
      noTone(BUZZER_PIN);
      buzzerOn = false;
      lastBeep = now;
    }
    else if (!buzzerOn && now - lastBeep >= 200)
    {
      tone(BUZZER_PIN, 2000);
      buzzerOn = true;
      lastBeep = now;
    }

    return;
  }

  // =================================================
  // 30-40 CM: MINIMUM BEEP
  // =================================================
  if (distance <= 40)
  {
    if (buzzerOn && now - lastBeep >= 150)
    {
      noTone(BUZZER_PIN);
      buzzerOn = false;
      lastBeep = now;
    }
    else if (!buzzerOn && now - lastBeep >= 700)
    {
      tone(BUZZER_PIN, 1500);
      buzzerOn = true;
      lastBeep = now;
    }

    return;
  }
}

// =====================================================
// READ SENSORS
// =====================================================
void readSensors()
{
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_INTERVAL)
  {
    lastSensorRead = now;

    distance = readDistance();

    mq2Value = analogRead(MQ2_PIN);

    pcfStatus = checkPCF();
  }

  if (now - lastDHTRead >= DHT_INTERVAL)
  {
    lastDHTRead = now;

    float newTemperature = dht.readTemperature();
    float newHumidity = dht.readHumidity();

    if (!isnan(newTemperature))
    {
      temperature = newTemperature;
    }

    if (!isnan(newHumidity))
    {
      humidity = newHumidity;
    }
  }
}

// =====================================================
// SEND DATA TO PYTHON SERVER
// =====================================================
void sendDataToServer()
{
  Serial.print("DATA,");

  Serial.print(distance, 1);
  Serial.print(",");

  Serial.print(mq2Value);
  Serial.print(",");

  if (isnan(temperature))
  {
    Serial.print(-999);
  }
  else
  {
    Serial.print(temperature, 1);
  }

  Serial.print(",");

  if (isnan(humidity))
  {
    Serial.print(-999);
  }
  else
  {
    Serial.print(humidity, 1);
  }

  Serial.print(",");

  Serial.println(pcfStatus ? 1 : 0);
}

// =====================================================
// SERIAL MONITOR
// =====================================================
void printSerialData()
{
  Serial.println();
  Serial.println("==============================");

  Serial.print("Distance: ");

  if (distance < 0)
  {
    Serial.println("ERROR");
  }
  else
  {
    Serial.print(distance, 1);
    Serial.println(" cm");
  }

  Serial.print("MQ-2: ");
  Serial.println(mq2Value);

  Serial.print("Temperature: ");

  if (isnan(temperature))
  {
    Serial.println("ERROR");
  }
  else
  {
    Serial.print(temperature, 1);
    Serial.println(" C");
  }

  Serial.print("Humidity: ");

  if (isnan(humidity))
  {
    Serial.println("ERROR");
  }
  else
  {
    Serial.print(humidity, 1);
    Serial.println(" %");
  }

  Serial.print("PCF8574T: ");

  Serial.println(
    pcfStatus ? "CONNECTED" : "NOT FOUND"
  );
}

// =====================================================
// LCD PAGE 1 - DISTANCE
// =====================================================
void displayDistance()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("DISTANCE");

  lcd.setCursor(0, 1);

  if (distance < 0)
  {
    lcd.print("ERROR");
  }
  else
  {
    lcd.print(distance, 1);
    lcd.print(" cm");
  }
}

// =====================================================
// LCD PAGE 2 - MQ2
// =====================================================
void displayMQ2()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("MQ2 GAS SENSOR");

  lcd.setCursor(0, 1);
  lcd.print("VALUE: ");
  lcd.print(mq2Value);
}

// =====================================================
// LCD PAGE 3 - DHT11
// =====================================================
void displayDHT()
{
  lcd.clear();

  lcd.setCursor(0, 0);

  if (isnan(temperature))
  {
    lcd.print("TEMP: ERROR");
  }
  else
  {
    lcd.print("TEMP:");
    lcd.print(temperature, 1);
    lcd.print(" C");
  }

  lcd.setCursor(0, 1);

  if (isnan(humidity))
  {
    lcd.print("HUM: ERROR");
  }
  else
  {
    lcd.print("HUM:");
    lcd.print(humidity, 1);
    lcd.print(" %");
  }
}

// =====================================================
// LCD PAGE 4 - BUZZER
// =====================================================
void displayBuzzer()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("BUZZER STATUS");

  lcd.setCursor(0, 1);

  if (distance < 0 || distance > 40)
  {
    lcd.print("OFF");
  }
  else if (distance > 30)
  {
    lcd.print("MINIMUM");
  }
  else if (distance > 20)
  {
    lcd.print("AVERAGE");
  }
  else
  {
    lcd.print("MAXIMUM");
  }
}

// =====================================================
// UPDATE LCD
// =====================================================
void updateDisplay()
{
  unsigned long now = millis();

  // Change page every 3 seconds
  if (now - lastPageChange >= PAGE_INTERVAL)
  {
    lastPageChange = now;

    currentPage++;

    if (currentPage > 3)
    {
      currentPage = 0;
    }
  }

  // Refresh current page every 500 ms
  if (now - lastLCDRefresh >= LCD_REFRESH_INTERVAL)
  {
    lastLCDRefresh = now;

    switch (currentPage)
    {
      case 0:
        displayDistance();
        break;

      case 1:
        displayMQ2();
        break;

      case 2:
        displayDHT();
        break;

      case 3:
        displayBuzzer();
        break;
    }
  }
}

// =====================================================
// SETUP
// =====================================================
void setup()
{
  Serial.begin(9600);

  Wire.begin();

  // LCD
  lcd.init();
  lcd.backlight();

  // DHT11
  dht.begin();

  // HC-SR04
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  // Startup screen
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ARDUINO NANO");

  lcd.setCursor(0, 1);
  lcd.print("SYSTEM START");

  delay(2000);

  lcd.clear();

  // Initial readings
  distance = readDistance();

  mq2Value = analogRead(MQ2_PIN);

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  pcfStatus = checkPCF();

  displayDistance();

  lastPageChange = millis();
  lastSensorRead = millis();
  lastDHTRead = millis();
  lastServerSend = millis();
  lastSerialPrint = millis();
  lastLCDRefresh = millis();
  lastBeep = millis();
}

// =====================================================
// LOOP
// =====================================================
void loop()
{
  readSensors();

  buzzerControl();

  updateDisplay();

  // Send data to Python SQL Server
  if (millis() - lastServerSend >= SERVER_INTERVAL)
  {
    lastServerSend = millis();

    sendDataToServer();
  }

  // Print debug data every 3 seconds
  if (millis() - lastSerialPrint >= 3000)
  {
    lastSerialPrint = millis();

    printSerialData();
  }

  delay(50);
}