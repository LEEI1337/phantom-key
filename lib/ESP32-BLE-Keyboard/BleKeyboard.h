/*
 * ESP32 BLE Keyboard Library
 * Based on T-vK/ESP32-BLE-Keyboard
 * With Arduino 3.x / NimBLE 2.x compatibility fixes
 */

#ifndef ESP32_BLE_KEYBOARD_H
#define ESP32_BLE_KEYBOARD_H

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <Arduino.h>
#include "Print.h"

#if defined(USE_NIMBLE)
  #include <NimBLEDevice.h>
  #include <NimBLEHIDDevice.h>
  #define HID_KEYBOARD 0x03C1
#else
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
  #include <BLE2902.h>
  #include <BLEHIDDevice.h>
#endif

#include "BleConnectionStatus.h"
#include "KeyboardOutputCallbacks.h"

// Modifier keys
#define KEY_LEFT_CTRL   0x80
#define KEY_LEFT_SHIFT  0x81
#define KEY_LEFT_ALT    0x82
#define KEY_LEFT_GUI    0x83
#define KEY_RIGHT_CTRL  0x84
#define KEY_RIGHT_SHIFT 0x85
#define KEY_RIGHT_ALT   0x86
#define KEY_RIGHT_GUI   0x87

// Special keys
#define KEY_UP_ARROW    0xDA
#define KEY_DOWN_ARROW  0xD9
#define KEY_LEFT_ARROW  0xD8
#define KEY_RIGHT_ARROW 0xD7
#define KEY_BACKSPACE   0xB2
#define KEY_TAB         0xB3
#define KEY_RETURN      0xB0
#define KEY_ESC         0xB1
#define KEY_INSERT      0xD1
#define KEY_PRTSC       0xCE
#define KEY_DELETE      0xD4
#define KEY_PAGE_UP     0xD3
#define KEY_PAGE_DOWN   0xD6
#define KEY_HOME        0xD2
#define KEY_END         0xD5
#define KEY_CAPS_LOCK   0xC1
#define KEY_NUM_LOCK    0xDB
#define KEY_SCROLL_LOCK 0xCF

// Function keys
#define KEY_F1          0xC2
#define KEY_F2          0xC3
#define KEY_F3          0xC4
#define KEY_F4          0xC5
#define KEY_F5          0xC6
#define KEY_F6          0xC7
#define KEY_F7          0xC8
#define KEY_F8          0xC9
#define KEY_F9          0xCA
#define KEY_F10         0xCB
#define KEY_F11         0xCC
#define KEY_F12         0xCD
#define KEY_F13         0xF0
#define KEY_F14         0xF1
#define KEY_F15         0xF2
#define KEY_F16         0xF3
#define KEY_F17         0xF4
#define KEY_F18         0xF5
#define KEY_F19         0xF6
#define KEY_F20         0xF7
#define KEY_F21         0xF8
#define KEY_F22         0xF9
#define KEY_F23         0xFA
#define KEY_F24         0xFB

typedef uint8_t MediaKeyReport[2];

class BleKeyboard : public Print {
private:
    BleConnectionStatus* connectionStatus;

#if defined(USE_NIMBLE)
    NimBLEHIDDevice* hid;
    NimBLECharacteristic* inputKeyboard;
    NimBLECharacteristic* outputKeyboard;
    NimBLECharacteristic* inputMediaKeys;
#else
    BLEHIDDevice* hid;
    BLECharacteristic* inputKeyboard;
    BLECharacteristic* outputKeyboard;
    BLECharacteristic* inputMediaKeys;
    BLEAdvertising* advertising;
#endif

    KeyboardOutputCallbacks* keyboardOutputCallbacks;

    uint8_t _keyReport[8];
    MediaKeyReport _mediaKeyReport;

    std::string deviceName;
    std::string deviceManufacturer;
    uint8_t batteryLevel;

    uint8_t _delay;

    void rawAction(uint8_t msg[], size_t size);

public:
    BleKeyboard(std::string deviceName = "ESP-Key",
                std::string deviceManufacturer = "HomeLab",
                uint8_t batteryLevel = 100);

    void begin(void);
    void end(void);

    bool isConnected(void);

    void setBatteryLevel(uint8_t level);
    void setDelay(uint8_t delay) { _delay = delay; }

    void setName(std::string name) { deviceName = name; }

    size_t press(uint8_t k);
    size_t release(uint8_t k);
    size_t releaseAll(void);

    size_t write(uint8_t c) override;
    size_t write(const MediaKeyReport c);
    size_t write(const uint8_t *buffer, size_t size) override;
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_KEYBOARD_H
