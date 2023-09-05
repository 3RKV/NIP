#include <ezButton.h>
#include <GyverHX711.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <GyverPortal.h>
#include <BluetoothLE.h>

// #define TEST_MOTOR
// #define SLEEP_TEST
#define PS002
#define WIFI_LOGIN "Radient_lab"
#define WIFI_PASSWORD "TYU_!jqw"
#define BUTTON_PIN_BITMASK 0x10 // HEX:2^4
#define MOVE_OPEN_PIN 3
#define MOVE_CLOSE_PIN 10


#ifdef PS002
RTC_DATA_ATTR int bootCount = 0;
float K_PS002 = 64 / 8388608; //1,65в(1/2 опорного  напряжения)(32(коэффицент усиления)*2^24(бит))
float kBar = 0.087472073;//0.0827337;
GyverHX711 sensor(5, 6, HX_GAIN32_B); // data, sck
uint32_t zeroPS002 = 896200;
// 3426407;//411006;
#endif

unsigned long timer = 0;
uint8_t direction;

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


bool wifiConnect = 1;

// GyverPortal ui;

// GPlog ps002Log("PS002");

const String NAME = "ECP32C3";

BluetoothLE Bluetooth;
//------------WebUI build------------
// void build()
// {
//   GP.BUILD_BEGIN(2000);
//   GP.THEME(GP_DARK);
//   // GP.AREA_LOG(ps002Log, 5);
//   GP.TITLE("v0.0.1");
//   GP.BUILD_END();
// }

// void action()
// {
//   if (ui.update())
//   {
//     ui.updateLog(ps002Log);
//   }
// }

//-----------------------------------

void zeroPS002Update()
{
  int printZero = 0;
  // zeroPS002 = 0;
  for (int i = 0; i < 1000; i++)
    printZero += sensor.read();
  printZero /= 1000;
  Serial.println("zeroPS002: " + (String)printZero);
  // Bluetooth.printPS002("zeroPS002: " + (String)printZero);
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Serial initialised");
  delay(500);
  Bluetooth.init();

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

// ui.attachBuild(build);
// ui.attach(action);
// ui.start();

// ps002Log.start(128);
SKIP_WEB_UI_BUILD:
  //--Enable OTA update--
  ArduinoOTA.begin();
  // delay(300);

  //-----------------------------------


  sensor.sleepMode(false);
  ++bootCount;
  pinMode(4,INPUT_PULLDOWN);
  esp_deep_sleep_enable_gpio_wakeup(BUTTON_PIN_BITMASK, ESP_GPIO_WAKEUP_GPIO_HIGH);
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  delay(3000);
  if (bootCount<3)esp_deep_sleep_start();


  pinMode(MOVE_OPEN_PIN, OUTPUT);
  pinMode(MOVE_CLOSE_PIN, OUTPUT);


  // delay(1000);
  // zeroPS002Update();

}

#ifdef PS002
float getPressurePS002()
{
  int32_t reading = 0;
  // for (uint32_t i = 0; i < 16; i++)
  reading = sensor.read() - zeroPS002;
  if (reading < 0)
    reading = 0;
  Bluetooth.printPS002("ADC: " + (String)reading);
  // reading /= 16;
  float pressure = kBar *  (reading * K_PS002);// + b;
  // float atm = (reading * K_PS002);// - 1.7;
  Bluetooth.printK("Bar:" + (String)pressure);
  delay(200);
  return pressure;
}
#endif


void Moving(bool action)
{
  uint8_t state = 1;
  if (action)
  {
    // if (buttonMove.getCount() == 1)
    //   direction = MOVE_OPEN_PIN;
    // else if (buttonMove.getCount() == 3)
    //   direction = MOVE_CLOSE_PIN;
    // if (buttonMove.getCount() == 2)
    //   state = 0;
    // if (buttonMove.getCount() >= 3)
    //   buttonMove.resetCount();
  }
  else
    direction = MOVE_CLOSE_PIN;

  digitalWrite(direction, state);
  if (state == 0)
    timer = 0;
  else
    timer = millis();
}

void loop()
{
  ArduinoOTA.handle();
  // ui.tick();

#ifdef PS002
  getPressurePS002();
#endif

  if (direction == MOVE_OPEN_PIN)
    Serial.println("Pressure: " + (String)getPressurePS002() + "Bar");
  if (getPressurePS002() < 1.50)
    Moving(0);
  if (millis() - timer >= 12500)
  {
    digitalWrite(MOVE_OPEN_PIN, LOW);
    digitalWrite(MOVE_CLOSE_PIN, LOW);
    timer = 0;
  }

}