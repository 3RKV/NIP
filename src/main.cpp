#include <ezButton.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <GyverPortal.h>
#include <BluetoothLE.h>

#define SLEEP
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
#define PRESSURE_PIN 5
#ifdef PS002
RTC_DATA_ATTR int bootCount = 0;
float kPS002Bar = 0.0030769;             // Коэффицент линейного уравнения
float kPS002Volts = 0.000805860805860;   // 4095 значений 12ти битного ADC / 700кПа максимального значения датчика
uint8_t b = 0.053719;                     
uint32_t zeroPS002 = 121;                // Программный ноль датчика

float incomingPressureValue = 0.00;
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
  int printZero = 0;
  for (int i = 0; i < 1000; i++)
    printZero += analogRead(PRESSURE_PIN);
  printZero /= 1000;
  Serial.println("zeroPS002: " + (String)printZero);
  // BluetoothLE::printPS002("zeroPS002: " + (String)printZero);
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

  pinMode(PRESSURE_PIN,INPUT);
  // delay(1000);
  zeroPS002Update();

  BluetoothLE::setCallback(&Callback);
#ifdef SLEEP
  // deepsleep();
#endif
  getVolts();
}

#ifdef PS002
float getPressurePS002()
{
  int32_t reading = 0;
  for (uint8_t i =0; i<200; reading += analogRead(PRESSURE_PIN), i++)
  reading/=200;
  if (reading < zeroPS002)
    reading = zeroPS002;
  float pressure = ((reading - zeroPS002) * kPS002Bar);
  pressure += b;
  BluetoothLE::printK("Bar:" + (String)pressure);
   BluetoothLE::printPS002("ADC-ZERO:" + (String)(reading-zeroPS002));
  // BluetoothLE::printPS002("ADC: " + (String)reading);
  Serial.printf("ADC: %d;\t ADC-Zero: %d;\t Bar: %4.1f \n", reading,(reading-zeroPS002), pressure);
  delay(200);
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
  if (incomingPressureValue != 0)
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
  }

  // #ifdef PS002
  //   getPressurePS002();
  // #endif
}