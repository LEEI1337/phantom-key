/**
 * @file ScreenManager.h
 * @brief Display-Management für CYD
 * @description Verwaltet Screens, Transitions und Rendering
 */

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <Arduino.h>
#include <LovyanGFX.h>
#include "ErrorCodes.h"
#include "Widget.h"

// Screen IDs
enum class ScreenId {
    DASHBOARD_MAIN = 0,
    DASHBOARD_PROXMOX,
    DASHBOARD_DOCKER,
    DASHBOARD_NETWORK,
    DASHBOARD_GPU,
    SETTINGS,
    DEBUG_CONSOLE,
    MAX_SCREENS
};

// Transition Types
enum class TransitionType {
    NONE,
    FADE,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN
};

// Callback für Screen-Changes
typedef std::function<void(ScreenId newScreen)> ScreenChangeCallback;

class ScreenManager {
private:
    LGFX tft;
    
    ScreenId currentScreen = ScreenId::DASHBOARD_MAIN;
    ScreenId previousScreen = ScreenId::DASHBOARD_MAIN;
    
    bool isTransitioning = false;
    unsigned long transitionStart = 0;
    const uint16_t TRANSITION_DURATION = 300; // ms
    
    // Widget Lists pro Screen
    std::vector<Widget*> screens[(int)ScreenId::MAX_SCREENS];
    
    ScreenChangeCallback onScreenChangeCallback = nullptr;
    
    void renderTransition(TransitionType type);
    void clearScreen();
    void drawWidgets();
    
public:
    ScreenManager();
    ~ScreenManager();
    
    /**
     * @brief Initialisiert Display
     * @return ErrorCode OK bei Erfolg
     */
    ErrorCode begin();
    
    /**
     * @brief Haupt-Loop für Animationen
     */
    void loop();
    
    /**
     * @brief Wechselt zu anderem Screen
     * @param screenId Ziel-Screen
     * @param transition Transition-Typ
     * @return ErrorCode
     */
    ErrorCode switchToScreen(ScreenId screenId, TransitionType transition = TransitionType::FADE);
    
    /**
     * @brief Fügt Widget zu Screen hinzu
     */
    void addWidget(ScreenId screen, Widget* widget);
    
    /**
     * @brief Entfernt Widget von Screen
     */
    void removeWidget(ScreenId screen, uint8_t widgetIndex);
    
    /**
     * @brief Aktualisiert alle Widgets auf aktuellem Screen
     */
    void refreshWidgets();
    
    /**
     * @brief Setzt Callback für Screen-Changes
     */
    void onScreenChange(ScreenChangeCallback callback);
    
    /**
     * @brief Gibt aktuellen Screen zurück
     */
    ScreenId getCurrentScreen() const { return currentScreen; }
    
    /**
     * @brief Zeigt FPS Counter im Debug-Modus
     */
    void showFPS(bool enable);
    
    /**
     * @brief Setzt Helligkeit
     * @param brightness 0-255
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief Zeigt Ladebildschirm
     */
    void showLoadingScreen(const String& message);
    
    /**
     * @brief Zeigt Fehlerbildschirm
     */
    void showErrorScreen(ErrorCode error);
};

#endif // SCREEN_MANAGER_H
