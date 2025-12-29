#include "KeyboardOutputCallbacks.h"

#if defined(CONFIG_BT_ENABLED)

KeyboardOutputCallbacks::KeyboardOutputCallbacks(void) {}

#if defined(USE_NIMBLE)

// NimBLE 1.4.x API (Arduino 2.x)
void KeyboardOutputCallbacks::onWrite(NimBLECharacteristic* me) {
    std::string value = me->getValue();
    if (value.length() > 0) {
        ledState = value[0];
    }
}

#else

void KeyboardOutputCallbacks::onWrite(BLECharacteristic* me) {
    // Arduino 3.x Bluedroid: getValue() gibt String zurück, nicht std::string
    String value = me->getValue();
    if (value.length() > 0) {
        ledState = value[0];
    }
}

#endif

uint8_t KeyboardOutputCallbacks::getLedState(void) {
    return ledState;
}

#endif // CONFIG_BT_ENABLED
