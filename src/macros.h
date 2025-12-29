#ifndef MACROS_H
#define MACROS_H

#include <Arduino.h>
#include <Preferences.h>

// ============================================
// Macro Storage for ESP-Key
// Speichert bis zu 9 Makros mit je 1024 Bytes in NVS
// ============================================

#define MAX_MACROS 9
#define MAX_MACRO_SIZE 1024
#define MACRO_NAMESPACE "macros"

// Standard-Makros (werden geladen wenn NVS leer)
const char* DEFAULT_MACROS[MAX_MACROS] = {
    // Button 1: ESP-Key Config Page öffnen (Windows + Linux KDE)
    "REM Open ESP-Key Config\n"
    "GUI r\n"
    "DELAY 400\n"
    "STRING https://leei1337.github.io/esp-key/\n"
    "DELAY 100\n"
    "ENTER\n",

    // Button 2: Copy (Ctrl+C)
    "CTRL c\n",

    // Button 3: Paste (Ctrl+V)
    "CTRL v\n",

    // Button 4: Cut (Ctrl+X)
    "CTRL x\n",

    // Button 5: Undo (Ctrl+Z)
    "CTRL z\n",

    // Button 6: Select All (Ctrl+A)
    "CTRL a\n",

    // Button 7: Save (Ctrl+S)
    "CTRL s\n",

    // Button 8: Alt+Tab
    "ALT TAB\n",

    // Button 9: Win+D (Desktop zeigen)
    "GUI d\n"
};

class MacroStorage {
public:
    MacroStorage() {}

    void begin() {
        prefs.begin(MACRO_NAMESPACE, false);
        loadAll();
    }

    // Lädt alle Makros aus NVS
    void loadAll() {
        for (int i = 0; i < MAX_MACROS; i++) {
            String key = "m" + String(i);
            if (prefs.isKey(key.c_str())) {
                macros[i] = prefs.getString(key.c_str(), "");
            } else {
                // Standard-Makro setzen
                macros[i] = String(DEFAULT_MACROS[i]);
                save(i); // Speichern
            }
        }
        Serial.println("Macros loaded from NVS");
    }

    // Gibt Makro für Button zurück
    String get(int buttonIndex) {
        if (buttonIndex < 0 || buttonIndex >= MAX_MACROS) return "";
        return macros[buttonIndex];
    }

    // Setzt Makro für Button
    bool set(int buttonIndex, const String& script) {
        if (buttonIndex < 0 || buttonIndex >= MAX_MACROS) return false;
        if (script.length() > MAX_MACRO_SIZE) return false;

        macros[buttonIndex] = script;
        return true;
    }

    // Speichert ein Makro in NVS
    bool save(int buttonIndex) {
        if (buttonIndex < 0 || buttonIndex >= MAX_MACROS) return false;

        String key = "m" + String(buttonIndex);
        prefs.putString(key.c_str(), macros[buttonIndex]);
        Serial.printf("Macro %d saved (%d bytes)\n", buttonIndex, macros[buttonIndex].length());
        return true;
    }

    // Speichert alle Makros
    void saveAll() {
        for (int i = 0; i < MAX_MACROS; i++) {
            save(i);
        }
    }

    // Setzt alle Makros auf Standard zurück
    void resetToDefaults() {
        for (int i = 0; i < MAX_MACROS; i++) {
            macros[i] = String(DEFAULT_MACROS[i]);
        }
        saveAll();
        Serial.println("Macros reset to defaults");
    }

    // Gibt alle Makros als JSON zurück (für Web Config)
    String toJSON() {
        String json = "{\"macros\":[";
        for (int i = 0; i < MAX_MACROS; i++) {
            if (i > 0) json += ",";
            json += "{\"id\":" + String(i) + ",\"script\":\"";
            // Escape special characters
            String escaped = macros[i];
            escaped.replace("\\", "\\\\");
            escaped.replace("\"", "\\\"");
            escaped.replace("\n", "\\n");
            escaped.replace("\r", "\\r");
            json += escaped;
            json += "\"}";
        }
        json += "]}";
        return json;
    }

private:
    Preferences prefs;
    String macros[MAX_MACROS];
};

#endif // MACROS_H
