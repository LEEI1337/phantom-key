#ifndef DUCKY_H
#define DUCKY_H

#include <Arduino.h>
#include <BleKeyboard.h>

// ============================================
// Ducky Script Parser for ESP-Key
// Unterstützt: STRING, DELAY, Modifier+Key, Specials
// ============================================

class DuckyParser {
public:
    DuckyParser(BleKeyboard* kb) : keyboard(kb) {}

    // Führt ein komplettes Script aus
    void execute(const String& script) {
        if (!keyboard->isConnected()) return;

        int pos = 0;
        while (pos < script.length()) {
            // Finde Zeilenende
            int endPos = script.indexOf('\n', pos);
            if (endPos == -1) endPos = script.length();

            String line = script.substring(pos, endPos);
            line.trim();

            if (line.length() > 0 && !line.startsWith("//") && !line.startsWith("REM")) {
                executeLine(line);
            }

            pos = endPos + 1;
        }
    }

private:
    BleKeyboard* keyboard;

    void executeLine(const String& line) {
        // STRING - Text tippen
        if (line.startsWith("STRING ")) {
            String text = line.substring(7);
            keyboard->print(text);
        }
        // DELAY - Warten
        else if (line.startsWith("DELAY ")) {
            int ms = line.substring(6).toInt();
            delay(ms);
        }
        // ENTER
        else if (line == "ENTER" || line == "RETURN") {
            keyboard->write(KEY_RETURN);
        }
        // TAB
        else if (line == "TAB") {
            keyboard->write(KEY_TAB);
        }
        // SPACE
        else if (line == "SPACE") {
            keyboard->write(' ');
        }
        // ESCAPE
        else if (line == "ESCAPE" || line == "ESC") {
            keyboard->write(KEY_ESC);
        }
        // BACKSPACE
        else if (line == "BACKSPACE" || line == "BKSP") {
            keyboard->write(KEY_BACKSPACE);
        }
        // DELETE
        else if (line == "DELETE" || line == "DEL") {
            keyboard->write(KEY_DELETE);
        }
        // Arrow Keys
        else if (line == "UP" || line == "UPARROW") {
            keyboard->write(KEY_UP_ARROW);
        }
        else if (line == "DOWN" || line == "DOWNARROW") {
            keyboard->write(KEY_DOWN_ARROW);
        }
        else if (line == "LEFT" || line == "LEFTARROW") {
            keyboard->write(KEY_LEFT_ARROW);
        }
        else if (line == "RIGHT" || line == "RIGHTARROW") {
            keyboard->write(KEY_RIGHT_ARROW);
        }
        // Function Keys
        else if (line.startsWith("F") && line.length() <= 3) {
            int fnum = line.substring(1).toInt();
            if (fnum >= 1 && fnum <= 12) {
                keyboard->write(KEY_F1 + fnum - 1);
            }
        }
        // GUI/WIN - Windows-Taste
        else if (line.startsWith("GUI ") || line.startsWith("WIN ") || line.startsWith("WINDOWS ")) {
            String key = line.substring(line.indexOf(' ') + 1);
            key.toLowerCase();
            pressModifierCombo(KEY_LEFT_GUI, key);
        }
        else if (line == "GUI" || line == "WIN" || line == "WINDOWS") {
            keyboard->write(KEY_LEFT_GUI);
        }
        // CTRL
        else if (line.startsWith("CTRL ") || line.startsWith("CONTROL ")) {
            String key = line.substring(line.indexOf(' ') + 1);
            key.toLowerCase();
            pressModifierCombo(KEY_LEFT_CTRL, key);
        }
        // ALT
        else if (line.startsWith("ALT ")) {
            String key = line.substring(4);
            key.toLowerCase();
            pressModifierCombo(KEY_LEFT_ALT, key);
        }
        // SHIFT
        else if (line.startsWith("SHIFT ")) {
            String key = line.substring(6);
            key.toLowerCase();
            pressModifierCombo(KEY_LEFT_SHIFT, key);
        }
        // CTRL+ALT
        else if (line.startsWith("CTRL-ALT ") || line.startsWith("CTRL ALT ")) {
            String key = line.substring(9);
            key.toLowerCase();
            keyboard->press(KEY_LEFT_CTRL);
            keyboard->press(KEY_LEFT_ALT);
            pressKey(key);
            delay(50);
            keyboard->releaseAll();
        }
        // CTRL+SHIFT
        else if (line.startsWith("CTRL-SHIFT ") || line.startsWith("CTRL SHIFT ")) {
            String key = line.substring(11);
            key.toLowerCase();
            keyboard->press(KEY_LEFT_CTRL);
            keyboard->press(KEY_LEFT_SHIFT);
            pressKey(key);
            delay(50);
            keyboard->releaseAll();
        }
        // ALT+TAB (special)
        else if (line == "ALT-TAB" || line == "ALT TAB") {
            keyboard->press(KEY_LEFT_ALT);
            keyboard->press(KEY_TAB);
            delay(50);
            keyboard->releaseAll();
        }
        // PRINTSCREEN
        else if (line == "PRINTSCREEN" || line == "PRTSC") {
            keyboard->write(KEY_PRTSC);
        }
        // HOME / END / PAGEUP / PAGEDOWN
        else if (line == "HOME") {
            keyboard->write(KEY_HOME);
        }
        else if (line == "END") {
            keyboard->write(KEY_END);
        }
        else if (line == "PAGEUP" || line == "PGUP") {
            keyboard->write(KEY_PAGE_UP);
        }
        else if (line == "PAGEDOWN" || line == "PGDN") {
            keyboard->write(KEY_PAGE_DOWN);
        }
        // INSERT
        else if (line == "INSERT" || line == "INS") {
            keyboard->write(KEY_INSERT);
        }
        // CAPSLOCK
        else if (line == "CAPSLOCK" || line == "CAPS") {
            keyboard->write(KEY_CAPS_LOCK);
        }
        // NUMLOCK
        else if (line == "NUMLOCK") {
            keyboard->write(KEY_NUM_LOCK);
        }
        // SCROLLLOCK
        else if (line == "SCROLLLOCK") {
            keyboard->write(KEY_SCROLL_LOCK);
        }
        // MENU (Context Menu)
        else if (line == "MENU" || line == "APP") {
            keyboard->write(0xED); // KEY_MENU
        }
        // Einzelner Buchstabe/Zeichen
        else if (line.length() == 1) {
            keyboard->write(line[0]);
        }
    }

    void pressModifierCombo(uint8_t modifier, const String& key) {
        keyboard->press(modifier);
        pressKey(key);
        delay(50);
        keyboard->releaseAll();
    }

    void pressKey(const String& key) {
        if (key.length() == 1) {
            keyboard->press(key[0]);
        }
        else if (key == "enter" || key == "return") {
            keyboard->press(KEY_RETURN);
        }
        else if (key == "tab") {
            keyboard->press(KEY_TAB);
        }
        else if (key == "space") {
            keyboard->press(' ');
        }
        else if (key == "esc" || key == "escape") {
            keyboard->press(KEY_ESC);
        }
        else if (key == "delete" || key == "del") {
            keyboard->press(KEY_DELETE);
        }
        else if (key == "backspace") {
            keyboard->press(KEY_BACKSPACE);
        }
        else if (key == "up") {
            keyboard->press(KEY_UP_ARROW);
        }
        else if (key == "down") {
            keyboard->press(KEY_DOWN_ARROW);
        }
        else if (key == "left") {
            keyboard->press(KEY_LEFT_ARROW);
        }
        else if (key == "right") {
            keyboard->press(KEY_RIGHT_ARROW);
        }
        else if (key.startsWith("f") && key.length() <= 3) {
            int fnum = key.substring(1).toInt();
            if (fnum >= 1 && fnum <= 12) {
                keyboard->press(KEY_F1 + fnum - 1);
            }
        }
    }
};

#endif // DUCKY_H
