/**
 * TouchHandler - Erweiterte Touch & Gesten-Erkennung für CYD
 * Unterstützt: Tap, DoubleTap, LongPress, Swipe, Widget-Interaction
 */

#pragma once

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <functional>

// Touch Pin Definition (für Standard CYD)
#define TOUCH_CS  13
#define TOUCH_IRQ 36

// Gesture Konstanten
#define TAP_TIMEOUT       250    // ms für Double-Tap Erkennung
#define LONGPRESS_TIMEOUT 800    // ms für Long-Press
#define SWIPE_THRESHOLD   30     // Pixel für Swipe Erkennung

// Touch Event Typen
enum TouchEventType {
  TOUCH_NONE,
  TOUCH_TAP,
  TOUCH_DOUBLE_TAP,
  TOUCH_LONG_PRESS,
  TOUCH_SWIPE_UP,
  TOUCH_SWIPE_DOWN,
  TOUCH_SWIPE_LEFT,
  TOUCH_SWIPE_RIGHT,
  TOUCH_DRAG_START,
  TOUCH_DRAG_MOVE,
  TOUCH_DRAG_END
};

// Touch Event Struktur
struct TouchEvent {
  TouchEventType type;
  uint16_t x;
  uint16_t y;
  uint16_t startX;
  uint16_t startY;
  uint32_t timestamp;
  uint32_t duration;
};

// Callback Typen
using TouchCallback = std::function<void(TouchEvent&)>;
using GestureCallback = std::function<void(TouchEventType, uint16_t, uint16_t)>;

class TouchHandler {
private:
  XPT2046_Touchscreen* ts;
  TFT_eSPI* tft;
  
  // State Tracking
  bool isTouching;
  bool wasTouching;
  uint32_t lastTouchTime;
  uint32_t touchStartTime;
  uint16_t lastX, lastY;
  uint16_t startX, startY;
  uint8_t tapCount;
  
  // Callbacks
  TouchCallback onTap;
  TouchCallback onDoubleTap;
  TouchCallback onLongPress;
  TouchCallback onSwipe;
  TouchCallback onDrag;
  
  // Konfiguration
  uint16_t longPressTimeout;
  uint16_t swipeThreshold;
  bool gestureEnabled;
  
  // Interne Helper
  TouchEventType detectGesture(uint16_t x, uint16_t y, uint32_t currentTime);
  void resetState();
  
public:
  TouchHandler(TFT_eSPI* display);
  ~TouchHandler();
  
  // Initialisierung
  bool begin();
  void update();
  
  // Callback Registration
  void setOnTap(TouchCallback callback);
  void setOnDoubleTap(TouchCallback callback);
  void setOnLongPress(TouchCallback callback);
  void setOnSwipe(TouchCallback callback);
  void setOnDrag(TouchCallback callback);
  
  // Konfiguration
  void setLongPressTimeout(uint16_t ms);
  void setSwipeThreshold(uint16_t pixels);
  void enableGestures(bool enabled);
  
  // State Abfragen
  bool isPressed() const { return isTouching; }
  uint16_t getX() const { return lastX; }
  uint16_t getY() const { return lastY; }
  TouchEvent getLastEvent() const { return lastEvent; }
  
  // Widget Interaction Helper
  bool pointInRect(uint16_t x, uint16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh);
  
private:
  TouchEvent lastEvent;
  bool longPressTriggered;
};

// Implementation (inline für Header-only)

TouchHandler::TouchHandler(TFT_eSPI* display) 
  : ts(nullptr), tft(display), isTouching(false), wasTouching(false),
    lastTouchTime(0), touchStartTime(0), lastX(0), lastY(0), 
    startX(0), startY(0), tapCount(0), longPressTimeout(LONGPRESS_TIMEOUT),
    swipeThreshold(SWIPE_THRESHOLD), gestureEnabled(true), longPressTriggered(false) {
}

TouchHandler::~TouchHandler() {
  if (ts) delete ts;
}

bool TouchHandler::begin() {
  ts = new XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);
  ts->begin();
  
  // Kalibrierungswerte (müssen ggf. angepasst werden)
  ts->setRotation(tft->getRotation());
  
  Serial.println("TouchHandler initialized");
  return true;
}

void TouchHandler::update() {
  if (!ts) return;
  
  uint32_t currentTime = millis();
  
  // Touch Status abfragen
  bool touched = ts->touched();
  
  if (touched && !isTouching) {
    // Touch Start
    isTouching = true;
    touchStartTime = currentTime;
    TS_Point p = ts->getPoint();
    
    startX = p.x;
    startY = p.y;
    lastX = startX;
    lastY = startY;
    
    longPressTriggered = false;
    
  } else if (touched && isTouching) {
    // Touch Move / Drag
    TS_Point p = ts->getPoint();
    uint16_t currentX = p.x;
    uint16_t currentY = p.y;
    
    // Nur bei signifikanter Bewegung
    if (abs(currentX - lastX) > 2 || abs(currentY - lastY) > 2) {
      lastX = currentX;
      lastY = currentY;
      
      // Drag Event
      if (onDrag && gestureEnabled) {
        TouchEvent event;
        event.type = TOUCH_DRAG_MOVE;
        event.x = lastX;
        event.y = lastY;
        event.startX = startX;
        event.startY = startY;
        event.timestamp = currentTime;
        event.duration = currentTime - touchStartTime;
        
        onDrag(event);
      }
    }
    
    // Long Press prüfen
    if (!longPressTriggered && (currentTime - touchStartTime) > longPressTimeout) {
      longPressTriggered = true;
      
      if (onLongPress && gestureEnabled) {
        TouchEvent event;
        event.type = TOUCH_LONG_PRESS;
        event.x = startX;
        event.y = startY;
        event.startX = startX;
        event.startY = startY;
        event.timestamp = currentTime;
        event.duration = currentTime - touchStartTime;
        
        onLongPress(event);
      }
    }
    
  } else if (!touched && isTouching) {
    // Touch End
    uint32_t duration = currentTime - touchStartTime;
    
    if (gestureEnabled && duration > longPressTimeout) {
      // War Long Press, schon behandelt
    } else if (gestureEnabled && (currentTime - lastTouchTime) < TAP_TIMEOUT) {
      // Double Tap
      tapCount++;
      if (tapCount >= 2) {
        if (onDoubleTap) {
          TouchEvent event;
          event.type = TOUCH_DOUBLE_TAP;
          event.x = startX;
          event.y = startY;
          event.startX = startX;
          event.startY = startY;
          event.timestamp = currentTime;
          event.duration = duration;
          
          onDoubleTap(event);
        }
        tapCount = 0;
      }
    } else if (gestureEnabled && duration < TAP_TIMEOUT) {
      // Einfacher Tap
      tapCount = 1;
      lastTouchTime = currentTime;
      
      if (onTap) {
        TouchEvent event;
        event.type = TOUCH_TAP;
        event.x = startX;
        event.y = startY;
        event.startX = startX;
        event.startY = startY;
        event.timestamp = currentTime;
        event.duration = duration;
        
        onTap(event);
      }
    } else if (gestureEnabled && duration >= TAP_TIMEOUT) {
      // Swipe erkennen
      int16_t deltaX = lastX - startX;
      int16_t deltaY = lastY - startY;
      
      if (abs(deltaX) > swipeThreshold || abs(deltaY) > swipeThreshold) {
        TouchEventType swipeType = TOUCH_NONE;
        
        if (abs(deltaX) > abs(deltaY)) {
          swipeType = (deltaX > 0) ? TOUCH_SWIPE_RIGHT : TOUCH_SWIPE_LEFT;
        } else {
          swipeType = (deltaY > 0) ? TOUCH_SWIPE_DOWN : TOUCH_SWIPE_UP;
        }
        
        if (onSwipe) {
          TouchEvent event;
          event.type = swipeType;
          event.x = lastX;
          event.y = lastY;
          event.startX = startX;
          event.startY = startY;
          event.timestamp = currentTime;
          event.duration = duration;
          
          onSwipe(event);
        }
      }
    }
    
    resetState();
  }
  
  wasTouching = isTouching;
}

void TouchHandler::resetState() {
  isTouching = false;
  longPressTriggered = false;
  // tapCount wird für Double-Tap Logik behalten
}

void TouchHandler::setOnTap(TouchCallback callback) {
  onTap = callback;
}

void TouchHandler::setOnDoubleTap(TouchCallback callback) {
  onDoubleTap = callback;
}

void TouchHandler::setOnLongPress(TouchCallback callback) {
  onLongPress = callback;
}

void TouchHandler::setOnSwipe(TouchCallback callback) {
  onSwipe = callback;
}

void TouchHandler::setOnDrag(TouchCallback callback) {
  onDrag = callback;
}

void TouchHandler::setLongPressTimeout(uint16_t ms) {
  longPressTimeout = ms;
}

void TouchHandler::setSwipeThreshold(uint16_t pixels) {
  swipeThreshold = pixels;
}

void TouchHandler::enableGestures(bool enabled) {
  gestureEnabled = enabled;
}

bool TouchHandler::pointInRect(uint16_t x, uint16_t y, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh) {
  return (x >= rx && x <= (rx + rw) && y >= ry && y <= (ry + rh));
}
