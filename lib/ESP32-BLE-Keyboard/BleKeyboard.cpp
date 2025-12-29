/*
 * ESP32 BLE Keyboard Library
 * Based on T-vK/ESP32-BLE-Keyboard
 * With Arduino 3.x / NimBLE 2.x compatibility fixes
 */

#include "BleKeyboard.h"

#if defined(CONFIG_BT_ENABLED)

// HID Report Descriptor for Keyboard + Consumer Control
static const uint8_t _hidReportDescriptor[] = {
    // Keyboard
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xa1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xe0,        //   Usage Minimum (224)
    0x29, 0xe7,        //   Usage Maximum (231)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x03,        //   Input (Constant)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x65,        //   Usage Maximum (101)
    0x81, 0x00,        //   Input (Data, Array)
    0xc0,              // End Collection

    // LED Output Report
    0x05, 0x08,        // Usage Page (LEDs)
    0x19, 0x01,        // Usage Minimum (1)
    0x29, 0x05,        // Usage Maximum (5)
    0x95, 0x05,        // Report Count (5)
    0x75, 0x01,        // Report Size (1)
    0x91, 0x02,        // Output (Data, Variable, Absolute)
    0x95, 0x01,        // Report Count (1)
    0x75, 0x03,        // Report Size (3)
    0x91, 0x03,        // Output (Constant)

    // Consumer Control (Media Keys)
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0x3C, 0x02,  //   Usage Maximum (572)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0x3C, 0x02,  //   Logical Maximum (572)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x10,        //   Report Size (16)
    0x81, 0x00,        //   Input (Data, Array)
    0xC0               // End Collection
};

BleKeyboard::BleKeyboard(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel)
    : deviceName(std::move(deviceName)),
      deviceManufacturer(std::move(deviceManufacturer)),
      batteryLevel(batteryLevel),
      hid(nullptr),
      inputKeyboard(nullptr),
      outputKeyboard(nullptr),
      inputMediaKeys(nullptr),
      connectionStatus(new BleConnectionStatus()),
      keyboardOutputCallbacks(new KeyboardOutputCallbacks()),
      _delay(8) {
    memset(_keyReport, 0, sizeof(_keyReport));
    memset(_mediaKeyReport, 0, sizeof(_mediaKeyReport));
}

void BleKeyboard::begin(void) {
#if defined(USE_NIMBLE)
    // NimBLE 1.4.x (Arduino 2.x / ESP-IDF 4.x)
    NimBLEDevice::init(deviceName);
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(connectionStatus);

    // NimBLE 1.4.x Security - relaxed for better compatibility
    NimBLEDevice::setSecurityAuth(true, false, true);  // bonding, NO mitm, sc

    hid = new NimBLEHIDDevice(pServer);
    inputKeyboard = hid->inputReport(1);
    outputKeyboard = hid->outputReport(1);
    inputMediaKeys = hid->inputReport(2);

    outputKeyboard->setCallbacks(keyboardOutputCallbacks);

    hid->manufacturer()->setValue(deviceManufacturer);
    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    hid->hidInfo(0x00, 0x01);
    hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    hid->setBatteryLevel(batteryLevel);
    hid->startServices();

    // NimBLE 1.4.x Advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();

#else
    // Arduino 3.x Bluedroid: braucht String statt std::string
    BLEDevice::init(String(deviceName.c_str()));
    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(connectionStatus);

    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    hid = new BLEHIDDevice(pServer);
    inputKeyboard = hid->inputReport(1);
    outputKeyboard = hid->outputReport(1);
    inputMediaKeys = hid->inputReport(2);

    outputKeyboard->setCallbacks(keyboardOutputCallbacks);

    // Arduino 3.x: setValue braucht String
    hid->manufacturer()->setValue(String(deviceManufacturer.c_str()));
    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    hid->hidInfo(0x00, 0x01);
    hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    hid->setBatteryLevel(batteryLevel);
    hid->startServices();

    advertising = pServer->getAdvertising();
    advertising->setAppearance(HID_KEYBOARD);
    advertising->addServiceUUID(hid->hidService()->getUUID());
    advertising->start();
#endif
}

void BleKeyboard::end(void) {
}

bool BleKeyboard::isConnected(void) {
    return connectionStatus->connected;
}

void BleKeyboard::setBatteryLevel(uint8_t level) {
    batteryLevel = level;
    if (hid != nullptr) {
        hid->setBatteryLevel(batteryLevel);
    }
}

void BleKeyboard::rawAction(uint8_t msg[], size_t size) {
    inputKeyboard->setValue(msg, size);
    inputKeyboard->notify();
    delay(_delay);
}

// ASCII to HID keycode lookup table
static const uint8_t _asciimap[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2a, 0x2b, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00,
    0x2c, 0x1e, 0x34, 0x20, 0x21, 0x22, 0x24, 0x34,
    0x26, 0x27, 0x25, 0x2e, 0x36, 0x2d, 0x37, 0x38,
    0x27, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x33, 0x33, 0x36, 0x2e, 0x37, 0x38,
    0x1f, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
    0x1b, 0x1c, 0x1d, 0x2f, 0x31, 0x30, 0x23, 0x2d,
    0x35, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
    0x1b, 0x1c, 0x1d, 0x2f, 0x31, 0x30, 0x35, 0x00
};

size_t BleKeyboard::press(uint8_t k) {
    if (!isConnected()) return 0;

    uint8_t modifier = 0;
    uint8_t key = 0;

    if (k >= KEY_LEFT_CTRL && k <= KEY_RIGHT_GUI) {
        modifier = 1 << (k - KEY_LEFT_CTRL);
        _keyReport[0] |= modifier;
        rawAction(_keyReport, sizeof(_keyReport));
        return 1;
    }

    if (k >= 0xB0) {
        switch (k) {
            case KEY_UP_ARROW:    key = 0x52; break;
            case KEY_DOWN_ARROW:  key = 0x51; break;
            case KEY_LEFT_ARROW:  key = 0x50; break;
            case KEY_RIGHT_ARROW: key = 0x4F; break;
            case KEY_BACKSPACE:   key = 0x2A; break;
            case KEY_TAB:         key = 0x2B; break;
            case KEY_RETURN:      key = 0x28; break;
            case KEY_ESC:         key = 0x29; break;
            case KEY_INSERT:      key = 0x49; break;
            case KEY_PRTSC:       key = 0x46; break;
            case KEY_DELETE:      key = 0x4C; break;
            case KEY_PAGE_UP:     key = 0x4B; break;
            case KEY_PAGE_DOWN:   key = 0x4E; break;
            case KEY_HOME:        key = 0x4A; break;
            case KEY_END:         key = 0x4D; break;
            case KEY_CAPS_LOCK:   key = 0x39; break;
            case KEY_NUM_LOCK:    key = 0x53; break;
            case KEY_SCROLL_LOCK: key = 0x47; break;
            case KEY_F1:  key = 0x3A; break;
            case KEY_F2:  key = 0x3B; break;
            case KEY_F3:  key = 0x3C; break;
            case KEY_F4:  key = 0x3D; break;
            case KEY_F5:  key = 0x3E; break;
            case KEY_F6:  key = 0x3F; break;
            case KEY_F7:  key = 0x40; break;
            case KEY_F8:  key = 0x41; break;
            case KEY_F9:  key = 0x42; break;
            case KEY_F10: key = 0x43; break;
            case KEY_F11: key = 0x44; break;
            case KEY_F12: key = 0x45; break;
            default: return 0;
        }
    } else if (k < 128) {
        key = _asciimap[k];
        if (key == 0) return 0;

        if (k >= 'A' && k <= 'Z') {
            modifier = 0x02;
        } else if (k == '!' || k == '@' || k == '#' || k == '$' ||
                   k == '%' || k == '^' || k == '&' || k == '*' ||
                   k == '(' || k == ')' || k == '_' || k == '+' ||
                   k == '{' || k == '}' || k == '|' || k == ':' ||
                   k == '"' || k == '<' || k == '>' || k == '?' ||
                   k == '~') {
            modifier = 0x02;
        }
    }

    _keyReport[0] |= modifier;
    for (int i = 2; i < 8; i++) {
        if (_keyReport[i] == 0) {
            _keyReport[i] = key;
            break;
        }
    }

    rawAction(_keyReport, sizeof(_keyReport));
    return 1;
}

size_t BleKeyboard::release(uint8_t k) {
    if (!isConnected()) return 0;

    if (k >= KEY_LEFT_CTRL && k <= KEY_RIGHT_GUI) {
        uint8_t modifier = 1 << (k - KEY_LEFT_CTRL);
        _keyReport[0] &= ~modifier;
        rawAction(_keyReport, sizeof(_keyReport));
        return 1;
    }

    uint8_t key = 0;

    if (k >= 0xB0) {
        switch (k) {
            case KEY_UP_ARROW:    key = 0x52; break;
            case KEY_DOWN_ARROW:  key = 0x51; break;
            case KEY_LEFT_ARROW:  key = 0x50; break;
            case KEY_RIGHT_ARROW: key = 0x4F; break;
            case KEY_BACKSPACE:   key = 0x2A; break;
            case KEY_TAB:         key = 0x2B; break;
            case KEY_RETURN:      key = 0x28; break;
            case KEY_ESC:         key = 0x29; break;
            case KEY_INSERT:      key = 0x49; break;
            case KEY_PRTSC:       key = 0x46; break;
            case KEY_DELETE:      key = 0x4C; break;
            case KEY_PAGE_UP:     key = 0x4B; break;
            case KEY_PAGE_DOWN:   key = 0x4E; break;
            case KEY_HOME:        key = 0x4A; break;
            case KEY_END:         key = 0x4D; break;
            case KEY_CAPS_LOCK:   key = 0x39; break;
            case KEY_F1:  key = 0x3A; break;
            case KEY_F2:  key = 0x3B; break;
            case KEY_F3:  key = 0x3C; break;
            case KEY_F4:  key = 0x3D; break;
            case KEY_F5:  key = 0x3E; break;
            case KEY_F6:  key = 0x3F; break;
            case KEY_F7:  key = 0x40; break;
            case KEY_F8:  key = 0x41; break;
            case KEY_F9:  key = 0x42; break;
            case KEY_F10: key = 0x43; break;
            case KEY_F11: key = 0x44; break;
            case KEY_F12: key = 0x45; break;
            default: return 0;
        }
    } else if (k < 128) {
        key = _asciimap[k];
    }

    for (int i = 2; i < 8; i++) {
        if (_keyReport[i] == key) {
            _keyReport[i] = 0;
            break;
        }
    }
    _keyReport[0] = 0;

    rawAction(_keyReport, sizeof(_keyReport));
    return 1;
}

size_t BleKeyboard::releaseAll(void) {
    memset(_keyReport, 0, sizeof(_keyReport));
    rawAction(_keyReport, sizeof(_keyReport));
    return 1;
}

size_t BleKeyboard::write(uint8_t c) {
    press(c);
    release(c);
    return 1;
}

size_t BleKeyboard::write(const MediaKeyReport c) {
    if (!isConnected()) return 0;

    inputMediaKeys->setValue((uint8_t*)c, sizeof(MediaKeyReport));
    inputMediaKeys->notify();
    delay(_delay);

    uint8_t release[2] = {0, 0};
    inputMediaKeys->setValue(release, sizeof(release));
    inputMediaKeys->notify();
    delay(_delay);

    return 1;
}

size_t BleKeyboard::write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    while (size--) {
        if (*buffer) {
            if (write(*buffer)) n++;
            else break;
        }
        buffer++;
    }
    return n;
}

#endif // CONFIG_BT_ENABLED
