/*
 * ESP-Key v2: BLE Macro Keyboard
 * ESP32-C3 Super Mini
 *
 * Features:
 * - 9 programmable macro buttons (Ducky Script)
 * - Volume control via potentiometer
 * - 5x NeoPixel with effects (like WLED)
 * - Web Bluetooth configuration
 * - Button combos for LED control
 */

#include <Arduino.h>
#include <WiFi.h>
#define USE_NIMBLE
#include <NimBLEDevice.h>
#include <BleKeyboard.h>
#include <Preferences.h>

#include "config.h"
#include "ducky.h"
#include "macros.h"
#include "leds.h"
#include "ble_config.h"

// ============================================
// GLOBALS
// ============================================
BleKeyboard bleKeyboard(DEVICE_NAME, DEVICE_MANUFACTURER, 100);
DuckyParser ducky(&bleKeyboard);
MacroStorage macroStorage;
LedSystem leds;
Preferences preferences;
BleConfigService* bleConfig = nullptr;

// Button states
bool buttonState[NUM_BUTTONS] = {false};
bool lastButtonState[NUM_BUTTONS] = {false};
unsigned long buttonPressTime[NUM_BUTTONS] = {0};
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};

// Volume control
#define POTI_SAMPLES 16
int potiReadings[POTI_SAMPLES];
int potiReadIndex = 0;
long potiTotal = 0;
int lastPotiValue = 0;
int smoothedPotiValue = 0;
unsigned long lastVolumeUpdate = 0;
bool potiInitialized = false;

// Auto-calibration
int potiMinSeen = 4095;
int potiMaxSeen = 0;
bool potiCalibrated = false;
unsigned long potiCalibrationStart = 0;

// Mode
bool configMode = false;
unsigned long configModeStartTime = 0;

// Combo detection
bool comboActive = false;
int comboSecondButton = -1;

// ============================================
// FORWARD DECLARATIONS
// ============================================
void setupButtons();
void setupBLE();
void handleButtons();
void handleVolume();
void handleCombos(int secondButton);
void checkConfigMode();
void executeMacro(int buttonIndex);
void sendMediaKey(uint16_t key);
int readPotiSmoothed();
void handleSerialConfig();
void processSerialCommand(String cmd);

// ============================================
// SETUP
// ============================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=================================");
    Serial.println("  ESP-Key v2 - BLE Macro Keyboard");
    Serial.println("  + Ducky Script + NeoPixel");
    Serial.println("=================================\n");

    // Initialize components
    setupButtons();
    macroStorage.begin();
    leds.begin();

    // Start BLE
    setupBLE();

    // Config via Serial (Web Serial API in Chrome)
    Serial.println("Config via USB: Open config page in Chrome and connect");

    Serial.println("\nSetup complete!");
    Serial.println("Hold Button 1+9 for 3 seconds to enter Config Mode");
}

// ============================================
// SERIAL CONFIG HANDLER
// ============================================
String serialBuffer = "";

void handleSerialConfig() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            serialBuffer.trim();
            if (serialBuffer.length() > 0) {
                processSerialCommand(serialBuffer);
            }
            serialBuffer = "";
        } else {
            serialBuffer += c;
        }
    }
}

void processSerialCommand(String cmd) {
    // GET_INFO - Device info
    if (cmd == "GET_INFO") {
        Serial.println("{\"name\":\"ESP-Key\",\"version\":\"2.0\",\"macros\":9,\"leds\":5}");
    }
    // GET_MACRO:n - Get macro n (0-8)
    else if (cmd.startsWith("GET_MACRO:")) {
        int idx = cmd.substring(10).toInt();
        if (idx >= 0 && idx < 9) {
            String script = macroStorage.get(idx);
            // Escape for JSON
            script.replace("\\", "\\\\");
            script.replace("\"", "\\\"");
            script.replace("\n", "\\n");
            script.replace("\r", "\\r");
            Serial.printf("{\"macro\":%d,\"script\":\"%s\"}\n", idx, script.c_str());
        }
    }
    // SET_MACRO:n:script - Set macro n
    else if (cmd.startsWith("SET_MACRO:")) {
        int firstColon = cmd.indexOf(':', 10);
        if (firstColon > 0) {
            int idx = cmd.substring(10, firstColon).toInt();
            String script = cmd.substring(firstColon + 1);
            // Unescape
            script.replace("\\n", "\n");
            script.replace("\\r", "\r");
            script.replace("\\\"", "\"");
            script.replace("\\\\", "\\");
            if (idx >= 0 && idx < 9 && macroStorage.set(idx, script)) {
                macroStorage.save(idx);
                Serial.printf("{\"ok\":true,\"macro\":%d}\n", idx);
            } else {
                Serial.println("{\"ok\":false,\"error\":\"invalid\"}");
            }
        }
    }
    // GET_LEDS - Get LED settings
    else if (cmd == "GET_LEDS") {
        Serial.printf("{\"hue\":%d,\"sat\":%d,\"bri\":%d,\"effect\":%d,\"speed\":%d,\"on\":%s}\n",
            leds.getHue(), leds.getSaturation(), leds.getBrightness(),
            leds.getEffect(), leds.getSpeed(), leds.isEnabled() ? "true" : "false");
    }
    // SET_LEDS:h,s,b,e,sp,on - Set LED settings
    else if (cmd.startsWith("SET_LEDS:")) {
        String params = cmd.substring(9);
        int vals[6];
        int idx = 0;
        int start = 0;
        for (int i = 0; i <= params.length() && idx < 6; i++) {
            if (i == params.length() || params[i] == ',') {
                vals[idx++] = params.substring(start, i).toInt();
                start = i + 1;
            }
        }
        if (idx >= 6) {
            leds.setHue(vals[0]);
            leds.setSaturation(vals[1]);
            leds.setBrightness(vals[2]);
            leds.setEffect(vals[3]);
            leds.setSpeed(vals[4]);
            leds.setEnabled(vals[5] != 0);
            leds.saveSettings();
            Serial.println("{\"ok\":true}");
        }
    }
    // RESET_MACROS - Reset to defaults
    else if (cmd == "RESET_MACROS") {
        macroStorage.resetToDefaults();
        Serial.println("{\"ok\":true,\"reset\":true}");
    }
    // PING
    else if (cmd == "PING") {
        Serial.println("PONG");
    }
    else {
        Serial.println("{\"error\":\"unknown command\"}");
    }
}

// ============================================
// LOOP
// ============================================
void loop() {
    // Handle Serial config commands
    handleSerialConfig();

    // Update LEDs (for animations)
    leds.update();

    // Check for config mode activation
    checkConfigMode();

    if (configMode) {
        // Config mode indicator
        delay(10);
    } else {
        // Normal keyboard mode
        if (bleKeyboard.isConnected()) {
            handleButtons();
            handleVolume();
        }
    }

    delay(10);
}

// ============================================
// BUTTON SETUP
// ============================================
void setupButtons() {
    Serial.println("Setting up buttons...");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }

    // Poti
    analogReadResolution(12);
    Serial.printf("Buttons: %d, Poti: GPIO %d\n", NUM_BUTTONS, POTI_PIN);
}

// ============================================
// BLE SETUP
// ============================================
void setupBLE() {
    Serial.println("Starting BLE Keyboard...");

    // Disable WiFi to save power and reduce interference
    WiFi.mode(WIFI_OFF);

    bleKeyboard.begin();

    // Increase connection stability
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Max power

    Serial.printf("Device: %s\n", DEVICE_NAME);
    Serial.println("BLE Power: MAX (+9dBm)");
}

// ============================================
// CHECK CONFIG MODE (Button 1+9 for 3 seconds)
// ============================================
void checkConfigMode() {
    bool btn1 = digitalRead(BUTTON_PINS[0]) == LOW;
    bool btn9 = digitalRead(BUTTON_PINS[8]) == LOW;

    if (btn1 && btn9) {
        if (configModeStartTime == 0) {
            configModeStartTime = millis();
        } else if (millis() - configModeStartTime >= 3000) {
            if (!configMode) {
                configMode = true;
                Serial.println(">>> CONFIG MODE ACTIVATED <<<");
                leds.showConfigMode();
            }
        }
    } else {
        configModeStartTime = 0;

        // Exit config mode when both buttons released
        if (configMode && !btn1 && !btn9) {
            configMode = false;
            Serial.println(">>> NORMAL MODE <<<");
            leds.showNormalMode();
            leds.saveSettings();
            macroStorage.saveAll();
        }
    }
}

// ============================================
// BUTTON HANDLING
// ============================================
void handleButtons() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool reading = digitalRead(BUTTON_PINS[i]) == LOW;

        if (reading != lastButtonState[i]) {
            lastDebounceTime[i] = millis();
        }

        if ((millis() - lastDebounceTime[i]) > DEBOUNCE_MS) {
            if (reading != buttonState[i]) {
                buttonState[i] = reading;

                if (buttonState[i]) {
                    // Button pressed
                    buttonPressTime[i] = millis();

                    // Check for combos (Button 1 + another)
                    if (i == 0) {
                        // Button 1 pressed - wait for combo
                        comboActive = false;
                        comboSecondButton = -1;
                    } else if (buttonState[0]) {
                        // Button 1 is held, this is a combo
                        handleCombos(i);
                        comboActive = true;
                        comboSecondButton = i;
                    } else {
                        // Normal button press - execute macro
                        executeMacro(i);
                        leds.flashButton(i % NUM_LEDS);
                    }
                } else {
                    // Button released
                    if (i == 0 && !comboActive) {
                        // Button 1 released without combo - execute macro
                        executeMacro(0);
                        leds.flashButton(0);
                    }
                    if (i == 0) {
                        comboActive = false;
                        comboSecondButton = -1;
                    }
                }
            }
        }

        lastButtonState[i] = reading;
    }
}

// ============================================
// COMBO HANDLING
// ============================================
void handleCombos(int secondButton) {
    Serial.printf("Combo: 1+%d\n", secondButton + 1);

    switch (secondButton) {
        case 1: // 1+2: Farbe hoch
            leds.changeHue(1);
            break;

        case 2: // 1+3: Farbe runter
            leds.changeHue(-1);
            break;

        case 3: // 1+4: Helligkeit HOCH
            leds.changeBrightness(1);
            break;

        case 4: // 1+5: Helligkeit RUNTER
            leds.changeBrightness(-1);
            break;

        case 5: // 1+6: Effekt wechseln
            leds.nextEffect();
            break;

        case 6: // 1+7: An/Aus Toggle
            leds.toggle();
            break;

        case 7: // 1+8: Speed cycling (slow → fast → slow)
            {
                int newSpeed = leds.getSpeed() + 50;
                if (newSpeed > 255) newSpeed = 25;  // Wrap to slow
                leds.setSpeed(newSpeed);
            }
            break;

        // 1+9 is config mode (handled separately)
    }
}

// ============================================
// EXECUTE MACRO
// ============================================
void executeMacro(int buttonIndex) {
    if (!bleKeyboard.isConnected()) return;

    String script = macroStorage.get(buttonIndex);
    if (script.length() > 0) {
        Serial.printf("Executing macro %d (%d bytes)\n", buttonIndex + 1, script.length());
        ducky.execute(script);
    }
}

// ============================================
// VOLUME HANDLING (with auto-calibration)
// ============================================
int readPotiRaw() {
    long sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += analogRead(POTI_PIN);
        delayMicroseconds(50);
    }
    return sum / 8;
}

int readPotiSmoothed() {
    int rawValue = readPotiRaw();

    // Update calibration (first 5 seconds or ongoing)
    if (!potiCalibrated || (millis() - potiCalibrationStart < 5000)) {
        if (rawValue < potiMinSeen) potiMinSeen = rawValue;
        if (rawValue > potiMaxSeen) potiMaxSeen = rawValue;

        // Need at least 500 range for calibration
        if (potiMaxSeen - potiMinSeen > 500) {
            potiCalibrated = true;
        }
    }

    // Use calibrated or default range
    int minVal = potiCalibrated ? potiMinSeen : POTI_MIN;
    int maxVal = potiCalibrated ? potiMaxSeen : POTI_MAX;

    // Add 5% deadzone on each end
    int deadzone = (maxVal - minVal) / 20;
    minVal += deadzone;
    maxVal -= deadzone;

    // Map to 0-100 (inverted: low ADC = high volume)
    int mappedValue = constrain(rawValue, minVal, maxVal);
    mappedValue = map(mappedValue, minVal, maxVal, 100, 0);

    // Moving average filter
    potiTotal -= potiReadings[potiReadIndex];
    potiReadings[potiReadIndex] = mappedValue;
    potiTotal += mappedValue;
    potiReadIndex = (potiReadIndex + 1) % POTI_SAMPLES;

    return potiTotal / POTI_SAMPLES;
}

void handleVolume() {
    if (millis() - lastVolumeUpdate < VOLUME_UPDATE_MS) return;
    lastVolumeUpdate = millis();

    if (!potiInitialized) {
        potiCalibrationStart = millis();
        int initVal = readPotiSmoothed();
        for (int i = 0; i < POTI_SAMPLES; i++) {
            potiReadings[i] = initVal;
        }
        potiTotal = initVal * POTI_SAMPLES;
        lastPotiValue = initVal;
        potiInitialized = true;
        Serial.printf("Poti initialized at %d, calibration started\n", initVal);
        return;
    }

    smoothedPotiValue = readPotiSmoothed();
    int diff = smoothedPotiValue - lastPotiValue;

    // Hysteresis: need at least 2 steps change
    if (abs(diff) >= 2) {
        // Send multiple key presses for larger changes
        int steps = abs(diff) / 2;
        steps = min(steps, 5);  // Max 5 steps at once

        for (int i = 0; i < steps; i++) {
            if (diff > 0) {
                sendMediaKey(0xE9); // Volume Up
            } else {
                sendMediaKey(0xEA); // Volume Down
            }
            delay(30);  // Small delay between key presses
        }

        lastPotiValue = smoothedPotiValue;
    }
}

// ============================================
// SEND MEDIA KEY
// ============================================
void sendMediaKey(uint16_t key) {
    if (!bleKeyboard.isConnected()) return;

    MediaKeyReport report = {(uint8_t)(key & 0xFF), (uint8_t)(key >> 8)};
    bleKeyboard.write(report);
}
