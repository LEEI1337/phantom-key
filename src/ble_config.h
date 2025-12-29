#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "macros.h"
#include "leds.h"

// ============================================
// BLE Config Service for ESP-Key
// Ermöglicht Web Bluetooth Konfiguration
// ============================================

// Service UUID (custom)
#define CONFIG_SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"

// Characteristic UUIDs
#define CHAR_MACRO_DATA_UUID       "12345678-1234-5678-1234-56789abcdef1"  // Macro read/write
#define CHAR_MACRO_INDEX_UUID      "12345678-1234-5678-1234-56789abcdef2"  // Which macro (0-8)
#define CHAR_LED_SETTINGS_UUID     "12345678-1234-5678-1234-56789abcdef3"  // LED HSV/Effect
#define CHAR_STATUS_UUID           "12345678-1234-5678-1234-56789abcdef4"  // Status/Commands
#define CHAR_DEVICE_INFO_UUID      "12345678-1234-5678-1234-56789abcdef5"  // Device info JSON

class BleConfigService {
public:
    BleConfigService(MacroStorage* macros, LedSystem* leds)
        : macroStorage(macros), ledSystem(leds) {}

    void begin() {
        // Get existing NimBLE server (created by BleKeyboard)
        pServer = NimBLEDevice::getServer();
        if (!pServer) {
            Serial.println("ERROR: No BLE Server found!");
            return;
        }

        // Create config service
        pService = pServer->createService(CONFIG_SERVICE_UUID);

        // Macro Index Characteristic (which macro to read/write)
        pCharMacroIndex = pService->createCharacteristic(
            CHAR_MACRO_INDEX_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
        );
        pCharMacroIndex->setValue(&currentMacroIndex, 1);
        pCharMacroIndex->setCallbacks(new MacroIndexCallbacks(this));

        // Macro Data Characteristic (the actual macro script)
        pCharMacroData = pService->createCharacteristic(
            CHAR_MACRO_DATA_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
        );
        pCharMacroData->setCallbacks(new MacroDataCallbacks(this));
        updateMacroData();

        // LED Settings Characteristic (hue, sat, bri, effect, speed, enabled)
        pCharLedSettings = pService->createCharacteristic(
            CHAR_LED_SETTINGS_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
        );
        pCharLedSettings->setCallbacks(new LedSettingsCallbacks(this));
        updateLedSettings();

        // Status Characteristic (for commands and status)
        pCharStatus = pService->createCharacteristic(
            CHAR_STATUS_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
        );
        pCharStatus->setValue("ready");
        pCharStatus->setCallbacks(new StatusCallbacks(this));

        // Device Info Characteristic (JSON with device info)
        pCharDeviceInfo = pService->createCharacteristic(
            CHAR_DEVICE_INFO_UUID,
            NIMBLE_PROPERTY::READ
        );
        updateDeviceInfo();

        // Start service
        pService->start();
        Serial.println("BLE Config Service started");
        Serial.printf("Service UUID: %s\n", CONFIG_SERVICE_UUID);
    }

    // Update macro data characteristic with current macro
    void updateMacroData() {
        if (pCharMacroData && macroStorage) {
            String script = macroStorage->get(currentMacroIndex);
            pCharMacroData->setValue(script.c_str());
        }
    }

    // Update LED settings characteristic
    void updateLedSettings() {
        if (pCharLedSettings && ledSystem) {
            uint8_t settings[6] = {
                ledSystem->getHue(),
                ledSystem->getSaturation(),
                ledSystem->getBrightness(),
                ledSystem->getEffect(),
                ledSystem->getSpeed(),
                ledSystem->isEnabled() ? 1 : 0
            };
            pCharLedSettings->setValue(settings, 6);
        }
    }

    // Update device info
    void updateDeviceInfo() {
        if (pCharDeviceInfo) {
            String info = "{";
            info += "\"name\":\"ESP-Key\",";
            info += "\"version\":\"2.0\",";
            info += "\"macros\":9,";
            info += "\"leds\":5,";
            info += "\"effects\":" + String(EFFECT_COUNT);
            info += "}";
            pCharDeviceInfo->setValue(info.c_str());
        }
    }

    // Notify status change
    void notifyStatus(const String& status) {
        if (pCharStatus) {
            pCharStatus->setValue(status.c_str());
            pCharStatus->notify();
        }
    }

    // Getters
    uint8_t getCurrentMacroIndex() { return currentMacroIndex; }
    void setCurrentMacroIndex(uint8_t idx) {
        currentMacroIndex = idx % 9;
        updateMacroData();
    }

    MacroStorage* getMacroStorage() { return macroStorage; }
    LedSystem* getLedSystem() { return ledSystem; }

private:
    NimBLEServer* pServer = nullptr;
    NimBLEService* pService = nullptr;
    NimBLECharacteristic* pCharMacroIndex = nullptr;
    NimBLECharacteristic* pCharMacroData = nullptr;
    NimBLECharacteristic* pCharLedSettings = nullptr;
    NimBLECharacteristic* pCharStatus = nullptr;
    NimBLECharacteristic* pCharDeviceInfo = nullptr;

    MacroStorage* macroStorage;
    LedSystem* ledSystem;
    uint8_t currentMacroIndex = 0;

    // Callback classes
    class MacroIndexCallbacks : public NimBLECharacteristicCallbacks {
    public:
        MacroIndexCallbacks(BleConfigService* svc) : service(svc) {}

        void onWrite(NimBLECharacteristic* pChar) override {
            std::string value = pChar->getValue();
            if (value.length() > 0) {
                uint8_t idx = value[0] % 9;
                service->setCurrentMacroIndex(idx);
                Serial.printf("Macro index set to: %d\n", idx);
            }
        }
    private:
        BleConfigService* service;
    };

    class MacroDataCallbacks : public NimBLECharacteristicCallbacks {
    public:
        MacroDataCallbacks(BleConfigService* svc) : service(svc) {}

        void onWrite(NimBLECharacteristic* pChar) override {
            std::string value = pChar->getValue();
            String script = String(value.c_str());

            uint8_t idx = service->getCurrentMacroIndex();
            if (service->getMacroStorage()->set(idx, script)) {
                service->getMacroStorage()->save(idx);
                Serial.printf("Macro %d saved (%d bytes)\n", idx, script.length());
                service->notifyStatus("saved:" + String(idx));
            } else {
                Serial.println("Macro save failed!");
                service->notifyStatus("error:save_failed");
            }
        }
    private:
        BleConfigService* service;
    };

    class LedSettingsCallbacks : public NimBLECharacteristicCallbacks {
    public:
        LedSettingsCallbacks(BleConfigService* svc) : service(svc) {}

        void onWrite(NimBLECharacteristic* pChar) override {
            std::string value = pChar->getValue();
            if (value.length() >= 6) {
                LedSystem* leds = service->getLedSystem();
                leds->setHue(value[0]);
                leds->setSaturation(value[1]);
                leds->setBrightness(value[2]);
                leds->setEffect(value[3]);
                leds->setSpeed(value[4]);
                leds->setEnabled(value[5] != 0);
                leds->saveSettings();

                Serial.printf("LED settings: H=%d S=%d B=%d E=%d\n",
                    value[0], value[1], value[2], value[3]);
                service->notifyStatus("led:saved");
            }
        }
    private:
        BleConfigService* service;
    };

    class StatusCallbacks : public NimBLECharacteristicCallbacks {
    public:
        StatusCallbacks(BleConfigService* svc) : service(svc) {}

        void onWrite(NimBLECharacteristic* pChar) override {
            std::string value = pChar->getValue();
            String cmd = String(value.c_str());

            Serial.printf("Command received: %s\n", cmd.c_str());

            if (cmd == "reset_macros") {
                service->getMacroStorage()->resetToDefaults();
                service->notifyStatus("macros:reset");
            }
            else if (cmd == "save_all") {
                service->getMacroStorage()->saveAll();
                service->getLedSystem()->saveSettings();
                service->notifyStatus("all:saved");
            }
            else if (cmd == "get_all_macros") {
                // Return all macros as JSON
                String json = service->getMacroStorage()->toJSON();
                pChar->setValue(json.c_str());
                pChar->notify();
            }
            else if (cmd == "get_led_json") {
                String json = service->getLedSystem()->toJSON();
                pChar->setValue(json.c_str());
                pChar->notify();
            }
        }
    private:
        BleConfigService* service;
    };
};

#endif // BLE_CONFIG_H
