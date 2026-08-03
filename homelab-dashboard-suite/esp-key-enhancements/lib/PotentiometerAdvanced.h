/**
 * @file PotentiometerAdvanced.h
 * @brief Erweiterte Potentiometer-Logik für ESP-Key
 * @description EMA-Smoothing, Multi-Mode, Gesture-Erkennung
 */

#ifndef POTENTIOMETER_ADVANCED_H
#define POTENTIOMETER_ADVANCED_H

#include <Arduino.h>
#include "ErrorCodes.h"

// Betriebsmodi
enum class PotMode {
    ABSOLUTE,      // Direkter Wert (0-4095)
    RELATIVE,      // Delta-Bewegung (-1 bis +1)
    INCREMENTAL,   // Zähler hoch/runter
    MOMENTARY      // Nur bei Bewegung aktiv
};

// Bewegungsrichtung
enum class PotDirection {
    NONE,
    CLOCKWISE,
    COUNTER_CLOCKWISE
};

// Event-Typen
enum class PotEvent {
    VALUE_CHANGED,
    MODE_SWITCHED,
    LONG_PRESS_CW,    // Lange Drehung rechts
    LONG_PRESS_CCW,   // Lange Drehung links
    RAPID_SPIN        // Schnelle Drehung erkannt
};

// Callback-Typen
typedef std::function<void(int value, PotMode mode)> ValueCallback;
typedef std::function<void(PotEvent event, PotDirection dir)> EventCallback;
typedef std::function<void(PotMode newMode)> ModeCallback;

class PotentiometerAdvanced {
private:
    const uint8_t pin;
    
    // Raw Values
    int rawValue = 0;
    int lastRawValue = 0;
    
    // Smoothed Values
    float smoothedValue = 0;
    float emaAlpha = 0.1; // Exponential Moving Average Factor
    
    // Mode Management
    PotMode currentMode = PotMode::ABSOLUTE;
    int incrementalCounter = 0;
    int relativeDelta = 0;
    
    // Thresholds
    const int DEADZONE = 15;       // Minimale Änderung
    const int RAPID_THRESHOLD = 200; // Für Rapid Spin Erkennung
    const unsigned long LONG_PRESS_DURATION = 1000; // ms
    
    // Timing
    unsigned long lastChangeTime = 0;
    unsigned long pressStartTime = 0;
    bool isBeingTurned = false;
    
    // Direction Tracking
    PotDirection lastDirection = PotDirection::NONE;
    unsigned long directionStartTime = 0;
    
    // Callbacks
    ValueCallback onValueCallback = nullptr;
    EventCallback onEventCallback = nullptr;
    ModeCallback onModeCallback = nullptr;
    
    // Interne Methoden
    int readRaw();
    void applySmoothing();
    PotDirection detectDirection(int oldVal, int newVal);
    void checkGestures();
    void triggerEvent(PotEvent event, PotDirection dir);
    
public:
    /**
     * @brief Konstruktor
     * @param adcPin ADC Pin Nummer (0-34 für ESP32)
     */
    explicit PotentiometerAdvanced(uint8_t adcPin);
    ~PotentiometerAdvanced();
    
    /**
     * @brief Initialisiert Potentiometer
     * @param mode Start-Modus
     * @return ErrorCode OK bei Erfolg
     */
    ErrorCode begin(PotMode mode = PotMode::ABSOLUTE);
    
    /**
     * @brief Haupt-Loop, muss regelmäßig aufgerufen werden
     * @param interval Aufruf-Intervall in ms (empfohlen: 10-50ms)
     */
    void loop(unsigned long interval = 20);
    
    /**
     * @brief Setzt Callback für Wertänderungen
     */
    void onValue(ValueCallback callback);
    
    /**
     * @brief Setzt Callback für Events
     */
    void onEvent(EventCallback callback);
    
    /**
     * @brief Setzt Callback für Moduswechsel
     */
    void onModeChange(ModeCallback callback);
    
    /**
     * @brief Wechselt Betriebsmodus
     */
    ErrorCode setMode(PotMode mode);
    
    /**
     * @brief Gibt aktuellen Modus zurück
     */
    PotMode getMode() const { return currentMode; }
    
    /**
     * @brief Gibt geglätteten Wert zurück (0-4095)
     */
    int getValue() const { return (int)smoothedValue; }
    
    /**
     * @brief Gibt normalisierten Wert zurück (0.0-1.0)
     */
    float getNormalizedValue() const { return smoothedValue / 4095.0f; }
    
    /**
     * @brief Gibt relativen Delta-Wert zurück
     */
    int getRelativeDelta() const { return relativeDelta; }
    
    /**
     * @brief Gibt inkrementellen Zähler zurück
     */
    int getIncrementalCount() const { return incrementalCounter; }
    
    /**
     * @brief Setzt EMA Smoothing Faktor
     * @param alpha 0.0-1.0 (höher = weniger Smoothing)
     */
    void setSmoothingFactor(float alpha);
    
    /**
     * @brief Setzt Deadzone
     * @param zone Minimale Änderung die erkannt wird
     */
    void setDeadzone(int zone);
    
    /**
     * @brief Reset inkrementellen Zähler
     */
    void resetIncrementalCounter();
    
    /**
     * @brief Kalibriert Min/Max Werte
     * @param samples Anzahl Messungen für Kalibrierung
     */
    void calibrate(int samples = 100);
};

#endif // POTENTIOMETER_ADVANCED_H
