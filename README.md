# ESP-Key v2 - BLE Macro Keyboard

A programmable BLE macro keyboard based on ESP32-C3 Super Mini with NeoPixel effects and Ducky Script support.

## Quick Start - Konfiguration

### Online Config (empfohlen)

**https://leei1337.github.io/esp-key/**

1. ESP-Key per USB verbinden
2. Link oben in Chrome/Edge öffnen
3. "Verbinden" klicken → USB-Port wählen
4. Makros und LEDs konfigurieren
5. "Speichern" klicken - fertig!

> **Tipp:** Button 1 öffnet automatisch diese Config-Seite!

### Schnellzugriff per Button

| OS | Button 1 Aktion |
|----|-----------------|
| Windows | Win+R → öffnet Config-Seite |
| Linux (KDE) | Alt+F2 → öffnet Config-Seite |
| Linux (GNOME) | Super → URL tippen → Enter |

## Features

- **9 programmable macro buttons** with Ducky Script support
- **Volume control** via potentiometer
- **5 NeoPixels** with WLED-style effects
- **Web Serial configuration** - no software installation needed
- **Button combos** for LED control
- **Persistent storage** - settings survive power cycles

## Hardware

| Component | Specification |
|-----------|--------------|
| MCU | ESP32-C3 Super Mini |
| Buttons | 9x tactile switches (active LOW) |
| Potentiometer | 10k linear for volume |
| LEDs | 5x WS2812B NeoPixels |

### Pinout

| Function | GPIO |
|----------|------|
| Button 1 | 0 |
| Button 2 | 1 |
| Button 3 | 2 |
| Button 4 | 3 |
| Button 5 | 4 |
| Button 6 | 5 |
| Button 7 | 6 |
| Button 8 | 7 |
| Button 9 | 10 |
| NeoPixels | 8 |
| Potentiometer | 20 (ADC) |

## Button Functions

### Normal Mode
Each button (1-9) executes its programmed Ducky Script macro.

### LED Control Combos
Hold **Button 1** and press another button:

| Combo | Function |
|-------|----------|
| 1 + 2 | Hue + (change color) |
| 1 + 3 | Hue - (change color) |
| 1 + 4 | Brightness + |
| 1 + 5 | Brightness - |
| 1 + 6 | Next effect |
| 1 + 7 | Toggle LEDs on/off |
| 1 + 8 | Cycle speed (slow → fast → slow) |
| 1 + 9 | Config mode (hold 3 seconds) |

### LED Effects
1. **Solid** - Static color
2. **Breathing** - Pulsing brightness
3. **Rainbow** - Color cycling
4. **Chase** - Running light
5. **Sparkle** - Random flashes

## Default Macros

| Button | Default Action |
|--------|----------------|
| 1 | **Config-Seite öffnen** (Win+R → URL) |
| 2 | Copy (Ctrl+C) |
| 3 | Paste (Ctrl+V) |
| 4 | Cut (Ctrl+X) |
| 5 | Undo (Ctrl+Z) |
| 6 | Select All (Ctrl+A) |
| 7 | Save (Ctrl+S) |
| 8 | Alt+Tab |
| 9 | Show Desktop (Win+D) |

## Ducky Script Syntax

```
REM This is a comment
STRING Hello World
DELAY 500
ENTER
TAB
ESCAPE
BACKSPACE
DELETE
INSERT
HOME
END
PAGEUP
PAGEDOWN
UP / DOWN / LEFT / RIGHT
F1-F12
PRINTSCREEN
SCROLLLOCK
PAUSE
CAPSLOCK
NUMLOCK

CTRL <key>
ALT <key>
SHIFT <key>
GUI <key>    (Windows/Super key)

CTRL ALT DELETE
CTRL SHIFT ESCAPE
```

### Example Macros

**Open Terminal (Linux):**
```
CTRL ALT t
```

**Lock Screen (Windows):**
```
GUI l
```

**Screenshot:**
```
PRINTSCREEN
```

**Email Signature:**
```
STRING Best regards,
ENTER
STRING John Doe
```

## Building & Flashing

### Requirements
- PlatformIO CLI or IDE
- USB cable

### Build
```bash
pio run
```

### Flash
```bash
pio run -t upload
```

### Monitor Serial
```bash
pio device monitor
```

## Serial Protocol

For custom integrations, the ESP-Key accepts these commands via USB Serial (115200 baud):

| Command | Description |
|---------|-------------|
| `PING` | Returns `PONG` |
| `GET_INFO` | Returns device info JSON |
| `GET_MACRO:n` | Get macro n (0-8) |
| `SET_MACRO:n:script` | Set macro n |
| `GET_LEDS` | Get LED settings |
| `SET_LEDS:h,s,b,e,sp,on` | Set LED settings |
| `RESET_MACROS` | Reset to defaults |

## Troubleshooting

### BLE not connecting
- Remove device from PC Bluetooth settings
- Re-pair the device

### Button 1 öffnet Config nicht (Linux)
Auf manchen Linux-Systemen funktioniert Win+R nicht. Alternativen:
- **GNOME:** Super-Taste → URL eintippen → Enter
- **KDE:** Alt+F2 → URL eintippen → Enter
- Oder einfach den Link manuell öffnen: https://leei1337.github.io/esp-key/

### LEDs not working
- Check NeoPixel data wire connection (GPIO 8)
- Verify power supply can handle LED current

### Macros not saving
- Wait for "OK" response after SET_MACRO
- Check if NVS storage is full

## License

MIT License

## Credits

- ESP32-BLE-Keyboard library
- NimBLE-Arduino
- Adafruit NeoPixel
