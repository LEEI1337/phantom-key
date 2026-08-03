/**
 * @file TouchHandler.h
 * @brief Touch-Input Handler für CYD
 * @description Verarbeitet Touch-Events, Gesten und Widget-Interaktionen
 */

#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Arduino.h>
#include <XPT2046_Touchscreen.h>
#include "ErrorCodes.h"

// Touch-Konstanten
#define TOUCH_THRESHOLD 400
#define DEBOUNCE_DELAY 50
#define LONG_PRESS_DURATION 1000
#define DOUBLE_TAP_TIMEOUT 300

// Gesten-Typen
enum class GestureType {
    TAP,
    DOUBLE_TAP,
    LONG_PRESS,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN,
    NONE
};

// Touch-Event Struktur
struct TouchEvent {
    uint16_t x;
    uint16_t y;
    GestureType gesture;
    unsigned long timestamp;
    uint8_t widgetId;
};

// Callback-Typen
typedef std::function<void(TouchEvent event)> TouchCallback;
typedef std::function<void(GestureType gesture, uint8_t widgetId)> GestureCallback;

class TouchHandler {
private:
    XPT2046_Touchscreen ts;
    
    // Pin-Konfiguration (CYD Standard)
    const uint8_t CS_PIN = 33;
    const uint16_t TS_MINX = 150;
    const uint16_t TS_MAXX = 3800;
    const uint16_t TS_MINY = 180;
    const uint16_t TS_MAXY = 3800;
    
    // State Tracking
    bool isPressed = false;
    bool wasPressed = false;
    uint16_t lastX = 0;
    uint16_t lastY = 0;
    unsigned long pressStartTime = 0;
    unsigned long lastTapTime = 0;
    
    // Swipe Detection
    uint16_t swipeStartX = 0;
    uint16_t swipeStartY = 0;
    const uint16_t SWIPE_THRESHOLD = 50;
    
    // Callbacks
    TouchCallback onTouchCallback = nullptr;
    GestureCallback onGestureCallback = nullptr;
    
    // Interne Methoden
    GestureType detectGesture();
    void processTouch();
    uint8_t getWidgetId(uint16_t x, uint16_t y);
    
public:
    TouchHandler();
    ~TouchHandler();
    
    /**
     * @brief Initialisiert Touchscreen
     * @return ErrorCode OK bei Erfolg
     */
    ErrorCode begin();
    
    /**
     * @brief Haupt-Loop, muss regelmäßig aufgerufen werden
     */
    void loop();
    
    /**
     * @brief Setzt Callback für Touch-Events
     */
    void onTouch(TouchCallback callback);
    
    /**
     * @brief Setzt Callback für Gesten
     */
    void onGesture(GestureCallback callback);
    
    /**
     * @brief Prüft ob Touchscreen berührt wird
     */
    bool isTouched() const { return isPressed; }
    
    /**
     * @brief Gibt letzte Touch-Position zurück
     */
    void getLastPosition(uint16_t& x, uint16_t& y) const {
        x = lastX;
        y = lastY;
    }
    
    /**
     * @brief Kalibriert Touchscreen
     * @param minX, maxX, minY, maxY Kalibrierungswerte
     */
    void calibrate(uint16_t minX, uint16_t maxX, uint16_t minY, uint16_t maxY);
    
    /**
     * @brief Setzt Empfindlichkeit
     * @param threshold Niedrigerer Wert = empfindlicher
     */
    void setThreshold(uint16_t threshold);
};

#endif // TOUCH_HANDLER_H
