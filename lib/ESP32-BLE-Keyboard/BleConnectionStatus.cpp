#include "BleConnectionStatus.h"

#if defined(CONFIG_BT_ENABLED)

BleConnectionStatus::BleConnectionStatus(void) {}

#if defined(USE_NIMBLE)

// NimBLE 1.4.x API (Arduino 2.x)
void BleConnectionStatus::onConnect(NimBLEServer* pServer) {
    this->connected = true;
    this->pServer = pServer;
}

void BleConnectionStatus::onDisconnect(NimBLEServer* pServer) {
    this->connected = false;
    // Restart advertising
    NimBLEDevice::startAdvertising();
}

#else

void BleConnectionStatus::onConnect(BLEServer* pServer) {
    this->connected = true;
}

void BleConnectionStatus::onDisconnect(BLEServer* pServer) {
    this->connected = false;
    pServer->getAdvertising()->start();
}

#endif

#endif // CONFIG_BT_ENABLED
