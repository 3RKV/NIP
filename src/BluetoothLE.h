#pragma once
#ifndef BLUETOOTH_LE_H
#define BLUETOOTH_LE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>

// #include <ArduinoJson.h>

#include <Update.h>

#define SERVICE_UUID "9a0d5a3c-2557-11ee-be56-0242ac120002"
#define PS002_CHARACTERISTIC_UUID "b321ef58-3068-11ee-be56-0242ac120002" //"PS_002"
#define K_CHARACTERISTIC_UUID "81e30c1a-35b8-11ee-be56-0242ac120002"
#define CONTROL_PRESSURE_UUID "1e44ef8c-4bae-11ee-be56-0242ac120002"
#define STATUS_UUID "9eb476ea-717b-11ee-b962-0242ac120002"
#define BAT_UUID "5fd65da4-71ac-11ee-b962-0242ac120002"

class BluetoothLECallback
{
public:
    virtual void pressureSettingsChanged(String newValue){};
    virtual void connectionStatusChanged(bool connectionStatus){};
};

// class ExportBluetoothLECallback : public BluetoothLECallback
// {
// public:
//     String pressureSettingsChanged(String);
// };

class BluetoothLE
{
public:
    static void init();
    static void printPS002(const String &val);
    static void printK(const String &val);
    static void printStatus(const String &val);
    static void printBat(const String &val);
    static void setCallback(BluetoothLECallback *callback);
    static bool getStatusConnection();

private:
    class ServerCallbacks : public NimBLEServerCallbacks
    {
        void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc);
        void onDisconnect(NimBLEServer *pServer, ble_gap_conn_desc *desc);
        void onAuthenticationComplete(ble_gap_conn_desc *desc);
    };

    class pressureCharacteristicCallbacks : public NimBLECharacteristicCallbacks
    {
        void onWrite(NimBLECharacteristic *pressureCharacteristic);
    };

    static NimBLEServer *pServer;
    static NimBLEAdvertising *pAdvertising;

    // Обрантый вызов по умолчнию
    static BluetoothLECallback _defaultBluetoothLECallback;

    // Установленный обрантый вызов
    static BluetoothLECallback *_bluetoothLECallback;

    static NimBLEService *pValueService;
    static NimBLECharacteristic *pPS002Characteristic;
    static NimBLECharacteristic *KCharacteristic;
    static NimBLECharacteristic *pressureCharacteristic;
    static NimBLECharacteristic *statusCharacteristic;
    static NimBLECharacteristic *batCharacteristic;

    // Обрантый вызов по умолчнию
};

#endif