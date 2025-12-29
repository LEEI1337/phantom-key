#ifndef KEYS_H
#define KEYS_H

#include <BleKeyboard.h>

// ============================================
// Key Definitions for ESP-Key
// ============================================

// Key types
enum KeyType {
    KEY_TYPE_NONE = 0,
    KEY_TYPE_CHAR,      // Single character (a-z, 0-9, etc.)
    KEY_TYPE_SPECIAL,   // Special keys (Enter, Esc, etc.)
    KEY_TYPE_MEDIA,     // Media keys (Play, Volume, etc.)
    KEY_TYPE_MODIFIER,  // Modifier keys (Ctrl, Alt, etc.)
    KEY_TYPE_COMBO      // Key combination (Ctrl+C, etc.)
};

// Key mapping structure
struct KeyMapping {
    const char* name;       // Display name for UI
    KeyType type;           // Type of key
    uint16_t keyCode;       // Primary key code (16-bit for media keys)
    uint8_t modifier;       // Modifier (for combos)
};

// Available keys for mapping
const KeyMapping AVAILABLE_KEYS[] = {
    // === NONE ===
    {"Keine", KEY_TYPE_NONE, 0, 0},

    // === LETTERS ===
    {"A", KEY_TYPE_CHAR, 'a', 0},
    {"B", KEY_TYPE_CHAR, 'b', 0},
    {"C", KEY_TYPE_CHAR, 'c', 0},
    {"D", KEY_TYPE_CHAR, 'd', 0},
    {"E", KEY_TYPE_CHAR, 'e', 0},
    {"F", KEY_TYPE_CHAR, 'f', 0},
    {"G", KEY_TYPE_CHAR, 'g', 0},
    {"H", KEY_TYPE_CHAR, 'h', 0},
    {"I", KEY_TYPE_CHAR, 'i', 0},
    {"J", KEY_TYPE_CHAR, 'j', 0},
    {"K", KEY_TYPE_CHAR, 'k', 0},
    {"L", KEY_TYPE_CHAR, 'l', 0},
    {"M", KEY_TYPE_CHAR, 'm', 0},
    {"N", KEY_TYPE_CHAR, 'n', 0},
    {"O", KEY_TYPE_CHAR, 'o', 0},
    {"P", KEY_TYPE_CHAR, 'p', 0},
    {"Q", KEY_TYPE_CHAR, 'q', 0},
    {"R", KEY_TYPE_CHAR, 'r', 0},
    {"S", KEY_TYPE_CHAR, 's', 0},
    {"T", KEY_TYPE_CHAR, 't', 0},
    {"U", KEY_TYPE_CHAR, 'u', 0},
    {"V", KEY_TYPE_CHAR, 'v', 0},
    {"W", KEY_TYPE_CHAR, 'w', 0},
    {"X", KEY_TYPE_CHAR, 'x', 0},
    {"Y", KEY_TYPE_CHAR, 'y', 0},
    {"Z", KEY_TYPE_CHAR, 'z', 0},

    // === NUMBERS ===
    {"0", KEY_TYPE_CHAR, '0', 0},
    {"1", KEY_TYPE_CHAR, '1', 0},
    {"2", KEY_TYPE_CHAR, '2', 0},
    {"3", KEY_TYPE_CHAR, '3', 0},
    {"4", KEY_TYPE_CHAR, '4', 0},
    {"5", KEY_TYPE_CHAR, '5', 0},
    {"6", KEY_TYPE_CHAR, '6', 0},
    {"7", KEY_TYPE_CHAR, '7', 0},
    {"8", KEY_TYPE_CHAR, '8', 0},
    {"9", KEY_TYPE_CHAR, '9', 0},

    // === FUNCTION KEYS ===
    {"F1", KEY_TYPE_SPECIAL, KEY_F1, 0},
    {"F2", KEY_TYPE_SPECIAL, KEY_F2, 0},
    {"F3", KEY_TYPE_SPECIAL, KEY_F3, 0},
    {"F4", KEY_TYPE_SPECIAL, KEY_F4, 0},
    {"F5", KEY_TYPE_SPECIAL, KEY_F5, 0},
    {"F6", KEY_TYPE_SPECIAL, KEY_F6, 0},
    {"F7", KEY_TYPE_SPECIAL, KEY_F7, 0},
    {"F8", KEY_TYPE_SPECIAL, KEY_F8, 0},
    {"F9", KEY_TYPE_SPECIAL, KEY_F9, 0},
    {"F10", KEY_TYPE_SPECIAL, KEY_F10, 0},
    {"F11", KEY_TYPE_SPECIAL, KEY_F11, 0},
    {"F12", KEY_TYPE_SPECIAL, KEY_F12, 0},

    // === SPECIAL KEYS ===
    {"Enter", KEY_TYPE_SPECIAL, KEY_RETURN, 0},
    {"Escape", KEY_TYPE_SPECIAL, KEY_ESC, 0},
    {"Backspace", KEY_TYPE_SPECIAL, KEY_BACKSPACE, 0},
    {"Tab", KEY_TYPE_SPECIAL, KEY_TAB, 0},
    {"Space", KEY_TYPE_CHAR, ' ', 0},
    {"Delete", KEY_TYPE_SPECIAL, KEY_DELETE, 0},
    {"Insert", KEY_TYPE_SPECIAL, KEY_INSERT, 0},
    {"Home", KEY_TYPE_SPECIAL, KEY_HOME, 0},
    {"End", KEY_TYPE_SPECIAL, KEY_END, 0},
    {"Page Up", KEY_TYPE_SPECIAL, KEY_PAGE_UP, 0},
    {"Page Down", KEY_TYPE_SPECIAL, KEY_PAGE_DOWN, 0},
    {"Arrow Up", KEY_TYPE_SPECIAL, KEY_UP_ARROW, 0},
    {"Arrow Down", KEY_TYPE_SPECIAL, KEY_DOWN_ARROW, 0},
    {"Arrow Left", KEY_TYPE_SPECIAL, KEY_LEFT_ARROW, 0},
    {"Arrow Right", KEY_TYPE_SPECIAL, KEY_RIGHT_ARROW, 0},
    {"Print Screen", KEY_TYPE_SPECIAL, KEY_PRTSC, 0},

    // === MEDIA KEYS (Consumer Control) ===
    {"Play/Pause", KEY_TYPE_MEDIA, 0xCD, 0},      // Play/Pause
    {"Stop", KEY_TYPE_MEDIA, 0xB7, 0},            // Stop
    {"Next Track", KEY_TYPE_MEDIA, 0xB5, 0},      // Next Track
    {"Prev Track", KEY_TYPE_MEDIA, 0xB6, 0},      // Previous Track
    {"Mute", KEY_TYPE_MEDIA, 0xE2, 0},            // Mute
    {"Volume Up", KEY_TYPE_MEDIA, 0xE9, 0},       // Volume Up
    {"Volume Down", KEY_TYPE_MEDIA, 0xEA, 0},     // Volume Down

    // === COMMON COMBOS ===
    {"Ctrl+C (Copy)", KEY_TYPE_COMBO, 'c', KEY_LEFT_CTRL},
    {"Ctrl+V (Paste)", KEY_TYPE_COMBO, 'v', KEY_LEFT_CTRL},
    {"Ctrl+X (Cut)", KEY_TYPE_COMBO, 'x', KEY_LEFT_CTRL},
    {"Ctrl+Z (Undo)", KEY_TYPE_COMBO, 'z', KEY_LEFT_CTRL},
    {"Ctrl+Y (Redo)", KEY_TYPE_COMBO, 'y', KEY_LEFT_CTRL},
    {"Ctrl+S (Save)", KEY_TYPE_COMBO, 's', KEY_LEFT_CTRL},
    {"Ctrl+A (Select All)", KEY_TYPE_COMBO, 'a', KEY_LEFT_CTRL},
    {"Ctrl+F (Find)", KEY_TYPE_COMBO, 'f', KEY_LEFT_CTRL},
    {"Alt+Tab", KEY_TYPE_COMBO, KEY_TAB, KEY_LEFT_ALT},
    {"Alt+F4", KEY_TYPE_COMBO, KEY_F4, KEY_LEFT_ALT},
    {"Win+D (Desktop)", KEY_TYPE_COMBO, 'd', KEY_LEFT_GUI},
    {"Win+L (Lock)", KEY_TYPE_COMBO, 'l', KEY_LEFT_GUI},
    {"Win+E (Explorer)", KEY_TYPE_COMBO, 'e', KEY_LEFT_GUI},
};

const int NUM_AVAILABLE_KEYS = sizeof(AVAILABLE_KEYS) / sizeof(AVAILABLE_KEYS[0]);

#endif // KEYS_H
