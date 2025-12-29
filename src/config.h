#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// ESP-Key Configuration
// ESP32-C3 Super Mini BLE Macro Keyboard
// ============================================

// Device Name (appears in Bluetooth)
#define DEVICE_NAME "ESP-Key"
#define DEVICE_MANUFACTURER "HomeLab"

// ============================================
// PIN DEFINITIONS
// ============================================
// ESP32-C3 Super Mini: GPIO 5,6,7 reserved for USB!
// GPIO 8: Onboard LED (active LOW)
// GPIO 9: BOOT button

// Buttons (directly usable GPIOs)
// FINALES Mapping basierend auf Test: 936852417
#define BTN_1_PIN   4   // Phys. Button 1 = GPIO 4
#define BTN_2_PIN   9   // Phys. Button 2 = GPIO 9 (BOOT)
#define BTN_3_PIN   2   // Phys. Button 3 = GPIO 2
#define BTN_4_PIN  21   // Phys. Button 4 = GPIO 21
#define BTN_5_PIN   8   // Phys. Button 5 = GPIO 8 (korrekt)
#define BTN_6_PIN   3   // Phys. Button 6 = GPIO 3
#define BTN_7_PIN   1   // Phys. Button 7 = GPIO 1
#define BTN_8_PIN  10   // Phys. Button 8 = GPIO 10
#define BTN_9_PIN  20   // Phys. Button 9 = GPIO 20

// Potentiometer - GPIO 0 (ADC, umgelötet!)
#define POTI_PIN    0
#define POTI_MIN    100     // ADC Minimum (Kalibrierung)
#define POTI_MAX    3900    // ADC Maximum (Kalibrierung)

// Number of buttons
#define NUM_BUTTONS 9

// Button pins array
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {
    BTN_1_PIN, BTN_2_PIN, BTN_3_PIN,
    BTN_4_PIN, BTN_5_PIN, BTN_6_PIN,
    BTN_7_PIN, BTN_8_PIN, BTN_9_PIN
};

// ============================================
// TIMING
// ============================================
#define DEBOUNCE_MS         50      // Button debounce time
#define VOLUME_UPDATE_MS    150     // Volume check interval
#define VOLUME_THRESHOLD    150     // ADC change threshold (höher = weniger empfindlich)
#define LONG_PRESS_MS       500     // Long press detection

// ============================================
// WIFI AP MODE (for configuration)
// ============================================
#define AP_SSID     "ESP-Key-Setup"
#define AP_PASSWORD "12345678"      // Min 8 chars
#define AP_CHANNEL  1

// ============================================
// NVS KEYS
// ============================================
#define NVS_NAMESPACE   "espkey"
#define NVS_KEY_MAP     "keymap"
#define NVS_WIFI_SSID   "wifi_ssid"
#define NVS_WIFI_PASS   "wifi_pass"

#endif // CONFIG_H
