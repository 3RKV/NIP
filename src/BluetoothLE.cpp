#include "BluetoothLE.h"

NimBLEServer *BluetoothLE::pServer = NULL;

NimBLEAdvertising *BluetoothLE::pAdvertising;

NimBLEService *BluetoothLE::pValueService = NULL;

NimBLECharacteristic *BluetoothLE::pPS002Characteristic = NULL;

NimBLECharacteristic *BluetoothLE::pressureCharacteristic = NULL;

NimBLECharacteristic *BluetoothLE::KCharacteristic = NULL;

NimBLECharacteristic *BluetoothLE::statusCharacteristic = NULL;

NimBLECharacteristic *BluetoothLE::batCharacteristic = NULL;

BluetoothLECallback *BluetoothLE::_bluetoothLECallback = NULL;

void BluetoothLE::pressureCharacteristicCallbacks::onWrite(NimBLECharacteristic *pressureCharacteristic)
{
    String pressure = pressureCharacteristic->getValue(); //<float>();
    if (_bluetoothLECallback != NULL)
        _bluetoothLECallback->pressureSettingsChanged(pressure);
}

void BluetoothLE::ServerCallbacks::onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
{
    // deviceConnected = true;
    Serial.println("Device connected");
    _bluetoothLECallback->connectionStatusChanged(true);
}

void BluetoothLE::ServerCallbacks::onDisconnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
{
    // deviceConnected = false;
    Serial.println("Device disconnected");
    _bluetoothLECallback->connectionStatusChanged(false);
}

void BluetoothLE::init()
{
    NimBLEDevice::init("ESP32C3");
    pServer = NimBLEDevice::createServer();
    pValueService = pServer->createService(SERVICE_UUID);
    
    pPS002Characteristic = pValueService->createCharacteristic(
        PS002_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY);

    KCharacteristic = pValueService->createCharacteristic(
        K_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY);

    statusCharacteristic = pValueService->createCharacteristic(
        STATUS_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY);

    batCharacteristic = pValueService->createCharacteristic(
        BAT_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY);

    pressureCharacteristic = pValueService->createCharacteristic(
        CONTROL_PRESSURE_UUID,
        NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY);

    pValueService->start();

    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID); // Добавление сервиса в службу рассылки
    pAdvertising->setAppearance(0x0180);
    pAdvertising->start();
    // pPS002Characteristic->setValue("");
    // KCharacteristic
    pressureCharacteristic->setCallbacks(new pressureCharacteristicCallbacks());
}

void BluetoothLE::printPS002(const String &val)
{
    std::string str(val.c_str());
    pPS002Characteristic->setValue<std::string>(str);
    pPS002Characteristic->notify(true);
}

void BluetoothLE::printK(const String &val)
{
    std::string str(val.c_str());
    KCharacteristic->setValue<std::string>(str);
    KCharacteristic->notify(true);
}

void BluetoothLE::printStatus(const String &val)
{
    std::string str(val.c_str());
    statusCharacteristic->setValue<std::string>(str);
    statusCharacteristic->notify(true);
}

void BluetoothLE::printBat(const String &val)
{
    std::string str(val.c_str());
    batCharacteristic->setValue<std::string>(str);
    batCharacteristic->notify(true);
}

void BluetoothLE::setCallback(BluetoothLECallback *callback)
{
    _bluetoothLECallback = callback;
}

bool BluetoothLE::getStatusConnection()
{
    return pServer->getConnectedCount();
}
