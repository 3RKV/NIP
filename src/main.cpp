#include <ezButton.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <GyverPortal.h>
#include <BluetoothLE.h>
#include <CS1237.h>

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
#define SCK 6
#define DOUT 5

#ifdef PS002
RTC_DATA_ATTR int bootCount = 0;
float kPS002Bar = 0.0000094428637;           // Коэффицент линейного уравнения
float kPS002Volts = 0.000000196695351; // 0.04172325;// 4095 значений 12ти битного ADC / 700кПа максимального значения датчика
uint8_t b = 0.0693713;
//P(v) = 48.3355809*x-0.0272062
int32_t zeroPS002 = 5900;
// 97; // 81; // Программный ноль датчика

bool powerOnTrigger_f = false;

bool power = 0;

bool readADC_f = false;

float incomingPressureValue = 0.00;
#endif

uint8_t act = 0;
unsigned long timer = 0;

String print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  String reason = "";

  switch (wakeup_reason)
  {
  case 1:
    reason = "ERROR"; //"Not a wakeup cause, used to disable all wakeup sources with esp_sleep_disable_wakeup_source";
    break;
  case 2:
    reason = "RTC_IO"; //"Wakeup caused by external signal using RTC_IO";
    break;
  case 3:
    reason = "RTC_CNTL"; //"Wakeup caused by external signal using RTC_CNTL";
    break;
  case 4:
    reason = "TIMER"; //"Wakeup caused by timer";
    break;
  case 5:
    reason = "TOUCH"; //"Wakeup caused by touchpad";
    break;
  case 6:
    reason = "ULP"; //"Wakeup caused by ULP program";
    break;
  case 7:
    reason = "GPIO"; //"Wakeup caused by GPIO";
    break;
  default:
    reason = "UNKN"; //"Wakeup was not caused by deep sleep";
    break;
  }
  return reason;
}
#ifdef WIFIUPDATE
bool wifiConnect = 1;
#endif

const String NAME = "ESP32C3";

CS1237 ADC(SCK, DOUT);

class ExportBluetoothLECallback : public BluetoothLECallback
{
public:
  virtual void pressureSettingsChanged(String newValue) override
  {
    incomingPressureValue = newValue.toFloat();
    Serial.println("Callback:" + (String)incomingPressureValue);
  };
  virtual void connectionStatusChanged(bool Status) override
  {
    if (!Status)
    // {
    //   Serial.println("connected");
    //   powerOnTrigger_f = false;
    // }
    // else
    {
      Serial.println("disconnected");
      powerOnTrigger_f = false;
    }
  };
};

ExportBluetoothLECallback Callback;

void zeroPS002Update()
{
  int printZero = 0;
  for (int i = 0; i < 10; printZero += ADC.reading(), i++)
    ;
  printZero /= 10;
  Serial.println("zeroPS002: " + (String)printZero);
  // BluetoothLE::printPS002("zeroPS002: " + (String)printZero);
}

float getVolts()
{
  uint16_t sensorValue = 0;
  digitalWrite(VOLTAGE_DIVIDER, HIGH);
  delay(5);
  for (uint8_t i = 0; i < 10; sensorValue += analogRead(VOLT_PIN), i++)
    ;
  digitalWrite(VOLTAGE_DIVIDER, LOW);
  sensorValue /= 10;
  float voltage = sensorValue * (3.3 / 4095.0);
  voltage /= 1.16;
  voltage *= 2;
  // Serial.printf("Bat:%4.1fV \n", voltage);
  BluetoothLE::printBat("Bat: " + (String)voltage + "V");
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
  if (bootCount < 2)
    esp_deep_sleep_enable_gpio_wakeup(GPIO_NUM_4, ESP_GPIO_WAKEUP_GPIO_HIGH);
  else if (bootCount >= 2)
  {
    esp_sleep_enable_timer_wakeup(120000000);
  }
  Serial.println("Boot number: " + String(bootCount));
  Serial.println(print_wakeup_reason());
#endif
  //-----------------------------------
  pinMode(MOVE_OPEN_PIN, OUTPUT);
  pinMode(MOVE_CLOSE_PIN, OUTPUT);

  pinMode(VOLTAGE_DIVIDER, OUTPUT);
  pinMode(VOLT_PIN, INPUT);
  if (ADC.configure(PGA_2, SPEED_1280, CHANNEL_A))
    Serial.println("success");
  else
    Serial.println("failed");
  // gpio_install_isr_service( ESP_INTR_FLAG_SHARED);

  // ADC.end_reading();

  BluetoothLE::setCallback(&Callback);
  getVolts();

  ADC.sleep(true);

  ADC.start_reading();
  // delay(1000);
  // zeroPS002Update();
#ifdef SLEEP
// Serial.println("Sleep");
// esp_deep_sleep_start();
#endif
  Serial.println("Initialise complete");
}

#ifdef PS002
float getPressurePS002()
{
  int32_t reading = 0;
  for (uint8_t i = 0; i < 10; i++)
  {
    reading += ADC.reading();
    delay(5);
  }
  reading /= 10;
  int32_t printZero = reading;
  if (reading < zeroPS002)
    reading = zeroPS002;
  float pressure = ((reading - zeroPS002) * kPS002Bar)-b;
  // pressure += b;
  BluetoothLE::printK("Psi:" + (String)pressure);
  // BluetoothLE::printPS002("ADC-ZERO:" + (String)(reading - zeroPS002));
  BluetoothLE::printPS002("ADC: " + (String)(reading)); //-zeroPS002));
  // Serial.printf("ADC: %d;\t ADC-Zero: %d;\t Volts: %8.5f V \n", printZero, (reading - zeroPS002), pressure);
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

  // getPressurePS002();
  // if (bootCount < 2) esp_deep_sleep_start();

  /*   String msg = "";
    if (!powerOnTrigger_f)
    {
      if (BluetoothLE::getStatusConnection())
      {
        Serial.println("Connected");
        BluetoothLE::printStatus("Connected");
        powerOnTrigger_f = true;
      }
    }
    else
    {
      if (getVolts() <= 3.40)
      {
        Serial.printf("Bat:%4.1fV \n", getVolts());
        BluetoothLE::printStatus("Boot number: " + String(bootCount));
        BluetoothLE::printPS002("Wakeup reason:" + print_wakeup_reason());
        BluetoothLE::printBat("Bat: " + (String)getVolts() + "V");
        BluetoothLE::printStatus("LOW ENERGY!");
        Moving(0);
        esp_deep_sleep_start();
      }
      else
      {
        if (act == 0)
        {
          Serial.printf("Bat:%4.1fV \n", getVolts());
          BluetoothLE::printK("Boot number: " + String(bootCount));
          BluetoothLE::printPS002("Wakeup reason:" + print_wakeup_reason());
          BluetoothLE::printStatus("Motor open");
          Moving(1);
          Serial.printf("Bat:%4.1fV \n", getVolts());
          BluetoothLE::printBat("Bat: " + (String)getVolts() + "V");
        }
        if (millis() - timer >= 12500)
        {
          digitalWrite(MOVE_OPEN_PIN, LOW);
          digitalWrite(MOVE_CLOSE_PIN, LOW);
          timer = 0;
          if (act == 1)
          {
            BluetoothLE::printStatus("Motor closed");
            Moving(0);
            Serial.printf("Bat:%4.1fV \n", getVolts());
            BluetoothLE::printBat("Bat: " + (String)getVolts() + "V");
          }
          else if (act == 2)
          {
            Serial.printf("Bat:%4.1fV \n", getVolts());
            act = 3;
            Serial.println("Deepsleep");
            esp_deep_sleep_start();
          }
        }
      }
      getPressurePS002();
    } */
#ifdef WIFIUPDATE
  ArduinoOTA.handle();
#endif

  /*   if (incomingPressureValue != 0)
    {
      if (!readADC_f)
      {
        ADC.start_reading();
        readADC_f = true;
      }
      float pressure = getPressurePS002();
      // Serial.println("inc Val != 0");
      if (getVolts() <= 3.40)
      {
        String LowEnergy = "LOW ENERGY!";
        BluetoothLE::printPS002(LowEnergy);
        // Moving(0);
        incomingPressureValue = 0;
        act = 0;
        // deepsleep();
      }
      else if (getVolts() >= 3.50)
      {
        if ((pressure > incomingPressureValue) && (act == 0))
          Moving(1);
        else if ((pressure <= incomingPressureValue) && (act == 1))
          Moving(0);
      }
    }
    else if (act == 1)
      Moving(0);

    if ((millis() - timer >= 12500) && (act != 0))
    {
      digitalWrite(MOVE_OPEN_PIN, LOW);
      digitalWrite(MOVE_CLOSE_PIN, LOW);
      if (readADC_f)
      {
        readADC_f = false;
        ADC.end_reading();
      }
      timer = 0;
      if (act == 2)
      {
        act = 0;
        incomingPressureValue = 0;
      }
    } */
  getPressurePS002();
  if (incomingPressureValue == 2)
  {
    Moving(0);
    incomingPressureValue = 0;
  }
  if (incomingPressureValue == 1)
  {
    Moving(1);
    incomingPressureValue = 0;
  }
  if ((millis() - timer >= 12500) && (act != 0))
  {
    digitalWrite(MOVE_OPEN_PIN, LOW);
    digitalWrite(MOVE_CLOSE_PIN, LOW);
    if (readADC_f)
    {
      readADC_f = false;
      ADC.end_reading();
    }
    timer = 0;
    if (act == 2)
    {
      act = 0;
      incomingPressureValue = 0;
    }
  }
}
