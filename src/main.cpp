#include <Arduino.h>
#include <ezButton.h>
#include "HX710B.h"
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <GyverPortal.h>
#include <BluetoothLE.h>

// #define SLEEP
#define PS002
// #define WIFIUPDATE
#ifdef WIFIUPDATE
#define WIFI_LOGIN "Radient_lab"
#define WIFI_PASSWORD "TYU_!jqw"
#endif
#define BUTTON_PIN_BITMASK 0x10 // HEX:2^4
#define MOVE_OPEN_PIN 3
#define MOVE_CLOSE_PIN 10
#define VOLT_PIN 0
#define VOLTAGE_DIVIDER 1
#define HX710_DOUT 5
#define HX710_SCLK 6

#ifdef PS002
RTC_DATA_ATTR int bootCount = 0;
float incomingPressureValue = 0.00;

HX710B pressure_sensor;
#endif

uint8_t act = 0;
unsigned long timer = 0;

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case 1:
    Serial.println("Not a wakeup cause, used to disable all wakeup sources with esp_sleep_disable_wakeup_source");
    break;
  case 2:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case 3:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case 4:
    Serial.println("Wakeup caused by timer");
    break;
  case 5:
    Serial.println("Wakeup caused by touchpad");
    break;
  case 6:
    Serial.println("Wakeup caused by ULP program");
    break;
  case 7:
    Serial.println("Wakeup caused by GPIO");
    break;
  default:
    Serial.println("Wakeup was not caused by deep sleep");
    break;
  }
}
#ifdef WIFIUPDATE
bool wifiConnect = 1;
#endif

const String NAME = "ECP32C3";

// BluetoothLE Bluetooth;
class ExportBluetoothLECallback : public BluetoothLECallback
{
public:
  virtual void pressureSettingsChanged(String newValue) override
  {
    incomingPressureValue = newValue.toFloat();
    Serial.println("Callback:" + (String)incomingPressureValue);
  };
};

ExportBluetoothLECallback Callback;

void zeroPS002Update()
{
  long oldOffset = pressure_sensor.get_offset();
  long offset = pressure_sensor.read_average(1) + oldOffset;
  Serial.printf("Start value: %ld \n", offset);
  delay(500);
  // pressure_sensor.set_offset(offset);
  Serial.printf("Offset: %ld\n", pressure_sensor.get_offset());
  delay(500);
}

void deepsleep()
{
#ifdef SLEEP
  // delay(3000);
  // if (bootCount < 3)
  esp_deep_sleep_start();
#endif
}

float getVolts()
{
  digitalWrite(VOLTAGE_DIVIDER, HIGH);
  uint16_t sensorValue = analogRead(VOLT_PIN);
  float voltage = sensorValue * (3.3 / 4095.0);
  voltage /= 1.16;
  voltage *= 2;
  Serial.println((String)voltage);
  digitalWrite(VOLTAGE_DIVIDER, LOW);
  BluetoothLE::printPS002("Bat: " + (String)voltage + "V");
  return voltage;
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Serial initialised");
  delay(500);
  BluetoothLE::init();

  pressure_sensor.begin(HX710_DOUT, HX710_SCLK);
  pressure_sensor.set_offset(724307);

#ifdef WIFIUPDATE
  //-------connet to Wifi-------------
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_LOGIN, WIFI_PASSWORD);
  uint32_t notConnectedCounter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.println("Wifi connecting...");
    notConnectedCounter++;
    //--event:wifi not found--
    if (notConnectedCounter > 10)
    {
      wifiConnect = 0;
      Serial.println("!!WiFi not connecting. Turn on WiFi and reboot your device to reconnect!!");
      goto SKIP_WEB_UI_BUILD;
    }
  }
  Serial.println("Wifi connected, IP address: ");
  Serial.println(WiFi.localIP());
  ArduinoOTA.begin();
SKIP_WEB_UI_BUILD:
#endif

#ifdef SLEEP
  sensor.sleepMode(false);
  ++bootCount;
  pinMode(4, INPUT_PULLDOWN);
  esp_deep_sleep_enable_gpio_wakeup(BUTTON_PIN_BITMASK, ESP_GPIO_WAKEUP_GPIO_HIGH);
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
#endif
  //-----------------------------------
  pinMode(MOVE_OPEN_PIN, OUTPUT);
  pinMode(MOVE_CLOSE_PIN, OUTPUT);

  pinMode(VOLTAGE_DIVIDER, OUTPUT);
  pinMode(VOLT_PIN, INPUT);

  // delay(1000);
  zeroPS002Update();

  BluetoothLE::setCallback(&Callback);
#ifdef SLEEP
  // deepsleep();
#endif
  // getVolts();
}

#ifdef PS002

float getPressurePS002()
{
  long int pressure = pressure_sensor.read_average(1);
  // if (pressure < 0)
  //   pressure = 0;

  // float kPa = pressure_sensor.kPascal();
  // float kPa =0.00000762939453125 * pressure_sensor.read_average(1) * 0.087472073;
    float Bar =  0.0000152587890625*pressure_sensor.read_average(1)*0.087472073;
  // if (kPa < 0)
  //   kPa = 0;
  Serial.printf("Pressure: %d \t Bar: %03.2f\n", pressure, Bar);
  BluetoothLE::printK("Bar: " + (String)Bar);
  BluetoothLE::printPS002("ADC: " +  (String)pressure);
  return pressure;
}
#endif

void Moving(bool action)
{
  digitalWrite(MOVE_OPEN_PIN, LOW);
  digitalWrite(MOVE_CLOSE_PIN, LOW);
  uint8_t direction;
  uint8_t state = 1;
  (action) ? direction = MOVE_OPEN_PIN : direction = MOVE_CLOSE_PIN;
  (action) ? act = 1 : act = 2;
  digitalWrite(direction, state);
  // (state == 0) ? timer = 0 :
  timer = millis();
}

void loop()
{
#ifdef WIFIUPDATE
  ArduinoOTA.handle();
#endif
  getPressurePS002();

  /* if (incomingPressureValue != 0)
  {
    if (getVolts() >= 3.50)
    {
      if ((getPressurePS002() > incomingPressureValue) && (act == 0))
        Moving(1);
      else if ((getPressurePS002() <= incomingPressureValue) && (act == 1))
        Moving(0);
    }
    else if (act == 1)
    {
      String LowEnergy = "Bat:LOW ENERGY!";
      BluetoothLE::printPS002(LowEnergy);
      Moving(0);
      // deepsleep();
    }
  }
  else if (act == 1)
    Moving(0);

  if ((millis() - timer >= 12500) && (act != 0))
  {
    digitalWrite(MOVE_OPEN_PIN, LOW);
    digitalWrite(MOVE_CLOSE_PIN, LOW);
    timer = 0;
    if (act == 2)
    {
      act = 0;
      incomingPressureValue = 0;
    }
  } */

  // #ifdef PS002
  //   getPressurePS002();
  // #endif
}