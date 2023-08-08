#include "BluetoothLE.h"

NimBLEServer *BluetoothLE::pServer = NULL;

NimBLEAdvertising *BluetoothLE::pAdvertising;

NimBLEService *BluetoothLE::pValueService = NULL;

NimBLECharacteristic *BluetoothLE::pPS002Characteristic = NULL;

NimBLECharacteristic *BluetoothLE::XGZCharacteristic = NULL;

NimBLECharacteristic *BluetoothLE::KCharacteristic = NULL;

void BluetoothLE::init()
{
    NimBLEDevice::init("ESP32C3");
    pServer = NimBLEDevice::createServer();
    pValueService = pServer->createService(SERVICE_UUID);
    pPS002Characteristic = pValueService->createCharacteristic(
        PS002_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY);

    XGZCharacteristic = pValueService->createCharacteristic(
        XGZ_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY);
    
    KCharacteristic = pValueService->createCharacteristic(
        K_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::NOTIFY);

    pValueService->start();

    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID); // Добавление сервиса в службу рассылки
    pAdvertising->setAppearance(0x0180);
    pAdvertising->start();
    pPS002Characteristic->setValue("");
}

void BluetoothLE::printPS002(String &val)
{
    std::string str(val.c_str());
    pPS002Characteristic->setValue<std::string>(str);
    pPS002Characteristic->notify(true);
}

void BluetoothLE::printXGZ(String &val)
{
    std::string str(val.c_str());
    XGZCharacteristic->setValue<std::string>(str);
    XGZCharacteristic->notify(true);
}

void BluetoothLE::printK(String &val)
{
    std::string str(val.c_str());
    KCharacteristic->setValue<std::string>(str);
    KCharacteristic->notify(true);
}