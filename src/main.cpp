#include <ezButton.h>
#include <GyverHX711.h>
#include <XGZP6828D.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <GyverPortal.h>
#include <BluetoothLE.h>

// #define TEST_MOTOR
// #define SLEEP_TEST
#define PS002
#define XGZ
#define WIFI_LOGIN "Radient_lab"
#define WIFI_PASSWORD "TYU_!jqw"

#ifdef TEST_MOTOR
#define MOVE_OPEN_PIN 6
#define MOVE_CLOSE_PIN 7
#endif

#ifdef XGZ
#define K_XGZ 32
int32_t zeroXGZ;
XGZP6828D mysensor(K_XGZ);
#endif

#ifdef PS002
RTC_DATA_ATTR int bootCount = 0;
float K_PS002 = 121 / 16777215.00;
float kBar = 0.0827337;
float b = 0.0103435;
GyverHX711 sensor(5, 6, HX_GAIN32_B); // data, sck
uint32_t zeroPS002 = 3449503;
// 3426407;//411006;
#endif

#ifdef TEST_MOTOR
unsigned long timer = 0;
uint8_t direction;
#endif

#ifdef SLEEP_TEST
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
#endif

bool wifiConnect = 1;

GyverPortal ui;

GPlog xgzLog("XGZ");
GPlog ps002Log("PS002");

const String NAME = "ECP32C3";

BluetoothLE Bluetooth;
//------------WebUI build------------
void build()
{
  GP.BUILD_BEGIN(2000);
  GP.THEME(GP_DARK);
  GP.AREA_LOG(xgzLog,5);
  GP.AREA_LOG(ps002Log,5);
  GP.TITLE("v0.0.1");
  GP.BUILD_END();
}

void action() {
  if (ui.update()) {
    ui.updateLog(xgzLog);
    ui.updateLog(ps002Log);
  }
}

//-----------------------------------

void zeroPS002Update()
{
  delay(1000);
  zeroPS002 = 0;
  for (int i = 0; i < 100; i++)
    zeroPS002 += sensor.read();
  zeroPS002 /= 100;
  Serial.println("zeroPS002: " + (String)zeroPS002);
}

void zeroXGZUpdate()
{
  delay(1000);
  zeroXGZ = 0;
  float t;
  float z;
  for (int i = 0; i < 100; i++)
  {
    mysensor.readSensor(t, z);
    zeroXGZ += z;
  }
  zeroXGZ /= 100;
  Serial.println("zeroXGZ: " + (String)zeroXGZ);
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

  ui.attachBuild(build);
  ui.attach(action);
  ui.start();

  ps002Log.start(128);
  xgzLog.start(128);

  //--Enable OTA update--
  ArduinoOTA.begin();
  delay(300);
  // ui.log.clear();
  SKIP_WEB_UI_BUILD:
  //-----------------------------------

  #ifdef SLEEP_TEST
  sensor.sleepMode(false);
  ++bootCount;
  esp_deep_sleep_enable_gpio_wakeup(16, ESP_GPIO_WAKEUP_GPIO_HIGH);
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  #endif

  #ifdef XGZ
  if (!mysensor.begin()) // initialize and check the device
  {
    Serial.println("Device XGZ not responding.");
    while (true)
      delay(10);
  }
  zeroXGZUpdate();
  #endif

  #ifdef TEST_MOTOR
  buttonMove.setDebounceTime(100);
  buttonMove.setCountMode(COUNT_RISING);
  pinMode(MOVE_OPEN_PIN, OUTPUT);
  pinMode(MOVE_CLOSE_PIN, OUTPUT);
  #endif

  #ifdef PS002
  zeroPS002Update();
  #endif
}

#ifdef PS002
float getPressurePS002()
{
  int32_t reading = 0;
  for (uint32_t i = 0; i < 10; i++)
  reading += sensor.read() - zeroPS002;
  reading /= 10;
  float val = (reading * K_PS002);
  float atm = kBar * val + b;
  return atm;
}
#endif

#ifdef XGZ
float getPressureXGZ()
{
  float val;
  for (uint32_t i = 0; i < 10; i++)
  {
    float temperature;
    mysensor.readSensor(temperature, val);
    val -= zeroXGZ;
    val += val;
  }
  val /= 10;
  val *= 0.000986923;
  return val;
}
#endif

#ifdef TEST_MOTOR
void Moving(bool action)
{
  uint8_t state = 1;
  if (action)
  {
    if (buttonMove.getCount() == 1)
      direction = MOVE_OPEN_PIN;
    else if (buttonMove.getCount() == 3)
      direction = MOVE_CLOSE_PIN;
    if (buttonMove.getCount() == 2)
      state = 0;
    if (buttonMove.getCount() >= 3)
      buttonMove.resetCount();
  }
  else
    direction = MOVE_CLOSE_PIN;

  digitalWrite(direction, state);
  if (state == 0)
    timer = 0;
  else
    timer = millis();
}

#endif

void loop()
{
  ArduinoOTA.handle();
  ui.tick();

#ifdef PS002
  // Serial.println("PS002 pressure: " + (String)getPressurePS002() + "ATM");
  Bluetooth.printPS002("PS002 pressure: " + (String)getPressurePS002() + "ATM");
  // ps002Log.println("PS002 pressure: " + (String)getPressurePS002() + "ATM");
#endif

#ifdef XGZ
  // Serial.println("XGZ pressure: " + (String)getPressureXGZ() + "ATM");
  Bluetooth.printXGZ("XGZ pressure: " + (String)getPressureXGZ() + "ATM");
  // xgzLog.println("XGZ pressure: " + (String)getPressureXGZ() + "ATM");
#endif

#ifdef TEST_MOTOR
  buttonCheck();
  if (direction == MOVE_OPEN_PIN)
    Serial.println("Pressure: " + (String)getPressure() + "Bar");
  if (getPressure() < 1.50)
    Moving(0);
  if (millis() - timer >= 12500)
  {
    digitalWrite(MOVE_OPEN_PIN, LOW);
    digitalWrite(MOVE_CLOSE_PIN, LOW);
    timer = 0;
  }
#endif
}