#ifndef POTI_HANDLER_H
#define POTI_HANDLER_H

#include <Arduino.h>
#include "ConfigManager.h"
#include "ErrorCodes.h"

// Fortgeschrittene Potentiometer-Verarbeitung
// Mit Smoothing, Modi und Event-System

enum class PotiMode {
    VOLUME = 0,
    LIGHT = 1,
    SCROLL = 2,
    CUSTOM = 3
};

class PotiHandler {
private:
    int pin;
    PotiMode mode;
    float smoothingFactor;
    int steps;
    bool inverted;
    
    int lastRawValue;
    float smoothedValue;
    int currentValue;
    uint32_t lastChangeTime;
    bool hasChanged;
    
    // Callbacks für Events
    std::function<void(int)> onChangeCallback;
    std::function<void()> onMinCallback;
    std::function<void()> onMaxCallback;
    
    // Für Scroll-Modus
    int scrollAccumulator;
    const int SCROLL_THRESHOLD = 15;
    
public:
    PotiHandler() 
        : pin(-1), mode(PotiMode::LIGHT), smoothingFactor(0.1f),
          steps(100), inverted(false), lastRawValue(0), 
          smoothedValue(0), currentValue(0), lastChangeTime(0),
          hasChanged(false), scrollAccumulator(0) {}
    
    ErrorCode begin(int _pin, const PotiConfig& config) {
        pin = _pin;
        mode = static_cast<PotiMode>(config.mode);
        smoothingFactor = config.smoothingFactor;
        steps = config.steps;
        inverted = config.inverted;
        
        if (pin < 0 || pin > 39) {
            ErrorHandler::set(Errors::SENSOR_NOT_FOUND);
            return Errors::SENSOR_NOT_FOUND;
        }
        
        pinMode(pin, INPUT);
        lastRawValue = analogRead(pin);
        smoothedValue = lastRawValue;
        currentValue = mapValue(lastRawValue);
        
        LOG_INFO("Poti initialized");
        return Errors::OK;
    }
    
    void update() {
        int rawValue = analogRead(pin);
        
        // Exponential Moving Average für Smoothing
        smoothedValue = smoothedValue * (1.0f - smoothingFactor) + 
                       rawValue * smoothingFactor;
        
        int mappedValue = mapValue(static_cast<int>(smoothedValue));
        
        // Hysterese um Flackern zu vermeiden
        if (abs(mappedValue - currentValue) > 2) {
            currentValue = mappedValue;
            hasChanged = true;
            lastChangeTime = millis();
            
            if (onChangeCallback) {
                onChangeCallback(currentValue);
            }
            
            // Min/Max Events
            if (currentValue <= 5 && onMinCallback) {
                onMinCallback();
            } else if (currentValue >= 95 && onMaxCallback) {
                onMaxCallback();
            }
        }
        
        // Scroll-Modus speziell behandeln
        if (mode == PotiMode::SCROLL) {
            int delta = mappedValue - lastRawValue;
            scrollAccumulator += delta;
            
            if (abs(scrollAccumulator) > SCROLL_THRESHOLD) {
                int scrollSteps = scrollAccumulator / SCROLL_THRESHOLD;
                // Scroll-Event könnte hier getriggert werden
                scrollAccumulator = 0;
            }
        }
        
        lastRawValue = mappedValue;
        hasChanged = false;
    }
    
    int getValue() const { return currentValue; }
    float getSmoothedValue() const { return smoothedValue; }
    bool changed() const { return hasChanged; }
    
    void setMode(PotiMode m) { mode = m; }
    PotiMode getMode() const { return mode; }
    
    void setSmoothing(float factor) { 
        smoothingFactor = constrain(factor, 0.01f, 0.99f); 
    }
    
    void setInverted(bool inv) { inverted = inv; }
    
    // Callbacks setzen
    void onChange(std::function<void(int)> cb) { onChangeCallback = cb; }
    void onMin(std::function<void()> cb) { onMinCallback = cb; }
    void onMax(std::function<void()> cb) { onMaxCallback = cb; }
    
    // Wert auf einen Dienst senden (MQTT, etc.)
    void sendToService(const String& topic) {
        // Wird vom Hauptprogramm implementiert
        // Beispiel: mqttClient.publish(topic, String(currentValue).c_str());
    }
    
private:
    int mapValue(int raw) {
        // ESP32 ADC: 0-4095
        int value = map(raw, 0, 4095, 0, steps);
        value = constrain(value, 0, steps);
        
        if (inverted) {
            value = steps - value;
        }
        
        return value;
    }
};

#endif // POTI_HANDLER_H
