#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

// ============================================
// NeoPixel LED System for ESP-Key
// 5 LEDs mit Effekten (wie WLED)
// ============================================

#define LED_PIN 5
#define NUM_LEDS 5
#define LED_NAMESPACE "leds"

// Effekt-IDs
enum LedEffect {
    EFFECT_SOLID = 0,
    EFFECT_BREATHING,
    EFFECT_RAINBOW,
    EFFECT_CHASE,
    EFFECT_SPARKLE,
    EFFECT_COUNT
};

const char* EFFECT_NAMES[] = {
    "Solid",
    "Breathing",
    "Rainbow",
    "Chase",
    "Sparkle"
};

class LedSystem {
public:
    LedSystem() : strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800) {}

    void begin() {
        strip.begin();
        strip.setBrightness(brightness);
        strip.show();

        // Lade Einstellungen aus NVS
        prefs.begin(LED_NAMESPACE, false);
        loadSettings();

        Serial.printf("LEDs initialized: %d LEDs on GPIO %d\n", NUM_LEDS, LED_PIN);
    }

    void loadSettings() {
        hue = prefs.getUChar("hue", 0);
        saturation = prefs.getUChar("sat", 255);
        brightness = prefs.getUChar("bri", 50);
        currentEffect = prefs.getUChar("effect", EFFECT_SOLID);
        speed = prefs.getUChar("speed", 50);
        enabled = prefs.getBool("enabled", true);

        strip.setBrightness(brightness);
    }

    void saveSettings() {
        prefs.putUChar("hue", hue);
        prefs.putUChar("sat", saturation);
        prefs.putUChar("bri", brightness);
        prefs.putUChar("effect", currentEffect);
        prefs.putUChar("speed", speed);
        prefs.putBool("enabled", enabled);
        Serial.println("LED settings saved");
    }

    // Muss regelmäßig aufgerufen werden für Animationen
    void update() {
        if (!enabled) {
            strip.clear();
            strip.show();
            return;
        }

        unsigned long now = millis();
        if (now - lastUpdate < (256 - speed) / 10 + 10) return;
        lastUpdate = now;

        switch (currentEffect) {
            case EFFECT_SOLID:
                effectSolid();
                break;
            case EFFECT_BREATHING:
                effectBreathing();
                break;
            case EFFECT_RAINBOW:
                effectRainbow();
                break;
            case EFFECT_CHASE:
                effectChase();
                break;
            case EFFECT_SPARKLE:
                effectSparkle();
                break;
        }

        strip.show();
        animStep++;
    }

    // Farbe ändern (Farbrad 0-255)
    void changeHue(int8_t delta) {
        hue += delta * 8;
        Serial.printf("Hue: %d\n", hue);
    }

    // Helligkeit ändern
    void changeBrightness(int8_t delta) {
        int newBri = brightness + delta * 25;
        brightness = constrain(newBri, 0, 255);
        strip.setBrightness(brightness);
        Serial.printf("Brightness: %d\n", brightness);
    }

    // Toggle On/Off
    void toggle() {
        enabled = !enabled;
        Serial.printf("LEDs: %s\n", enabled ? "ON" : "OFF");
    }

    // Nächster Effekt
    void nextEffect() {
        currentEffect = (currentEffect + 1) % EFFECT_COUNT;
        animStep = 0;
        Serial.printf("Effect: %s\n", EFFECT_NAMES[currentEffect]);
    }

    // Vorheriger Effekt
    void prevEffect() {
        currentEffect = (currentEffect + EFFECT_COUNT - 1) % EFFECT_COUNT;
        animStep = 0;
        Serial.printf("Effect: %s\n", EFFECT_NAMES[currentEffect]);
    }

    // Status-Anzeigen
    void showConnected() {
        flashColor(0, 255, 0, 200); // Grün
    }

    void showDisconnected() {
        flashColor(255, 0, 0, 200); // Rot
    }

    void showConfigMode() {
        // Weißes Pulsieren wird im Update gehandelt
        setConfigMode(true);
    }

    void showNormalMode() {
        setConfigMode(false);
    }

    void flashButton(int buttonIndex) {
        if (buttonIndex < NUM_LEDS) {
            // Kurz aufblitzen
            uint32_t oldColor = strip.getPixelColor(buttonIndex);
            strip.setPixelColor(buttonIndex, strip.Color(255, 255, 255));
            strip.show();
            delay(50);
            strip.setPixelColor(buttonIndex, oldColor);
            strip.show();
        }
    }

    // Getter
    uint8_t getHue() { return hue; }
    uint8_t getSaturation() { return saturation; }
    uint8_t getBrightness() { return brightness; }
    uint8_t getEffect() { return currentEffect; }
    uint8_t getSpeed() { return speed; }
    bool isEnabled() { return enabled; }

    // Setter
    void setHue(uint8_t h) { hue = h; }
    void setSaturation(uint8_t s) { saturation = s; }
    void setBrightness(uint8_t b) { brightness = b; strip.setBrightness(b); }
    void setEffect(uint8_t e) { currentEffect = e % EFFECT_COUNT; animStep = 0; }
    void setSpeed(uint8_t s) { speed = s; }
    void setEnabled(bool e) { enabled = e; }

    // JSON für Web Config
    String toJSON() {
        String json = "{";
        json += "\"hue\":" + String(hue) + ",";
        json += "\"sat\":" + String(saturation) + ",";
        json += "\"bri\":" + String(brightness) + ",";
        json += "\"effect\":" + String(currentEffect) + ",";
        json += "\"speed\":" + String(speed) + ",";
        json += "\"enabled\":" + String(enabled ? "true" : "false");
        json += "}";
        return json;
    }

private:
    Adafruit_NeoPixel strip;
    Preferences prefs;

    uint8_t hue = 0;
    uint8_t saturation = 255;
    uint8_t brightness = 50;
    uint8_t currentEffect = EFFECT_SOLID;
    uint8_t speed = 50;
    bool enabled = true;
    bool configMode = false;

    unsigned long lastUpdate = 0;
    uint16_t animStep = 0;

    void setConfigMode(bool mode) {
        configMode = mode;
        if (mode) {
            currentEffect = EFFECT_BREATHING;
            hue = 0;
            saturation = 0; // Weiß
        }
    }

    void flashColor(uint8_t r, uint8_t g, uint8_t b, int duration) {
        for (int i = 0; i < NUM_LEDS; i++) {
            strip.setPixelColor(i, strip.Color(r, g, b));
        }
        strip.show();
        delay(duration);
    }

    // HSV zu RGB Konvertierung
    uint32_t hsvToColor(uint8_t h, uint8_t s, uint8_t v) {
        return strip.ColorHSV(h * 256, s, v);
    }

    // === EFFEKTE ===

    void effectSolid() {
        uint32_t color = hsvToColor(hue, saturation, 255);
        for (int i = 0; i < NUM_LEDS; i++) {
            strip.setPixelColor(i, color);
        }
    }

    void effectBreathing() {
        // Sinusförmiges Pulsieren
        uint8_t breath = (sin(animStep * 0.05) + 1) * 127;
        uint32_t color = hsvToColor(hue, saturation, breath);
        for (int i = 0; i < NUM_LEDS; i++) {
            strip.setPixelColor(i, color);
        }
    }

    void effectRainbow() {
        for (int i = 0; i < NUM_LEDS; i++) {
            uint8_t pixelHue = hue + (i * 256 / NUM_LEDS) + animStep;
            strip.setPixelColor(i, hsvToColor(pixelHue, saturation, 255));
        }
    }

    void effectChase() {
        strip.clear();
        int pos = animStep % NUM_LEDS;
        strip.setPixelColor(pos, hsvToColor(hue, saturation, 255));
        // Trail
        int trail1 = (pos + NUM_LEDS - 1) % NUM_LEDS;
        int trail2 = (pos + NUM_LEDS - 2) % NUM_LEDS;
        strip.setPixelColor(trail1, hsvToColor(hue, saturation, 128));
        strip.setPixelColor(trail2, hsvToColor(hue, saturation, 64));
    }

    void effectSparkle() {
        // Zufälliges Funkeln
        for (int i = 0; i < NUM_LEDS; i++) {
            if (random(10) == 0) {
                strip.setPixelColor(i, hsvToColor(hue, saturation, 255));
            } else {
                uint32_t current = strip.getPixelColor(i);
                // Langsam abdimmen
                uint8_t r = ((current >> 16) & 0xFF) * 0.9;
                uint8_t g = ((current >> 8) & 0xFF) * 0.9;
                uint8_t b = (current & 0xFF) * 0.9;
                strip.setPixelColor(i, strip.Color(r, g, b));
            }
        }
    }
};

#endif // LEDS_H
