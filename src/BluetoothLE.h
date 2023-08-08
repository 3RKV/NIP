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

// HOOKAH CONTROL
#define SERVICE_UUID                                     "9a0d5a3c-2557-11ee-be56-0242ac120002"
#define PS002_CHARACTERISTIC_UUID                        "b321ef58-3068-11ee-be56-0242ac120002"         //"PS_002"
#define XGZ_CHARACTERISTIC_UUID                          "b321f3c2-3068-11ee-be56-0242ac120002"         //"XGZ"   
#define K_CHARACTERISTIC_UUID                            "81e30c1a-35b8-11ee-be56-0242ac120002"                   


class BluetoothLECallback{
    public:
        virtual void pressureSettingsChanged(){};

};

class BluetoothLE{
    public:

        static void init();
        static void printPS002(String &val);
        static void printXGZ(String &val);
        static void printK(String &val);
        
    private:
        class ServerCallbacks: public NimBLEServerCallbacks {
            void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc);
            void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc);
            void onAuthenticationComplete(ble_gap_conn_desc* desc);
        };

        static NimBLEServer *pServer;
        static NimBLEAdvertising *pAdvertising;

        // Обрантый вызов по умолчнию
        static BluetoothLECallback _defaultBluetoothLECallback;

        // Установленный обрантый вызов
        static BluetoothLECallback* _bluetoothLECallback;

        static NimBLEService *pValueService;
        static NimBLECharacteristic *pPS002Characteristic;
        static NimBLECharacteristic *XGZCharacteristic;
        static NimBLECharacteristic *KCharacteristic;

                // Обрантый вызов по умолчнию
};

#endif