#ifndef KEYBOARD_OUTPUT_CALLBACKS_H
#define KEYBOARD_OUTPUT_CALLBACKS_H

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#if defined(USE_NIMBLE)
  #include <NimBLEDevice.h>

  class KeyboardOutputCallbacks : public NimBLECharacteristicCallbacks {
  public:
    KeyboardOutputCallbacks(void);
    // NimBLE 1.4.x API (Arduino 2.x)
    void onWrite(NimBLECharacteristic* me) override;
    uint8_t getLedState(void);
  private:
    uint8_t ledState = 0;
  };
#else
  #include <BLECharacteristic.h>

  class KeyboardOutputCallbacks : public BLECharacteristicCallbacks {
  public:
    KeyboardOutputCallbacks(void);
    void onWrite(BLECharacteristic* me) override;
    uint8_t getLedState(void);
  private:
    uint8_t ledState = 0;
  };
#endif

#endif // CONFIG_BT_ENABLED
#endif // KEYBOARD_OUTPUT_CALLBACKS_H
