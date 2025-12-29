#ifndef BLE_CONNECTION_STATUS_H
#define BLE_CONNECTION_STATUS_H

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#if defined(USE_NIMBLE)
  #include <NimBLEDevice.h>

  class BleConnectionStatus : public NimBLEServerCallbacks {
  public:
    BleConnectionStatus(void);
    bool connected = false;
    NimBLEServer* pServer = nullptr;
    // NimBLE 1.4.x API (Arduino 2.x)
    void onConnect(NimBLEServer* pServer) override;
    void onDisconnect(NimBLEServer* pServer) override;
  };
#else
  #include <BLEServer.h>

  class BleConnectionStatus : public BLEServerCallbacks {
  public:
    BleConnectionStatus(void);
    bool connected = false;
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
  };
#endif

#endif // CONFIG_BT_ENABLED
#endif // BLE_CONNECTION_STATUS_H
