# ESP-Key: BLE Macro Keyboard

## Hardware
- **MCU**: ESP32-C3 Super Mini
- **Buttons**: 9x Taster
- **Volume**: 1x Potentiometer (10k)
- **Verbindung**: Bluetooth Low Energy (BLE)

## Funktionen

### 1. Makro-Keyboard
- Jeder Button kann ein **Makro** ausführen
- Makro-Typen:
  - **Text**: Bis zu 1000+ Zeichen (E-Mail, Passwort, Code-Snippets)
  - **Tasten-Kombos**: Ctrl+C, Alt+Tab, Win+L, etc.
  - **Gemischt**: Text + Tasten kombiniert (wie USBKitty/Rubber Ducky)
  - **Delays**: Pausen zwischen Aktionen
- **Mehrere Makros pro Button** möglich (einstellbar)
- **Profile**: Verschiedene Makro-Sets umschaltbar

### 2. Lautstärke
- Potentiometer steuert System-Lautstärke
- Smooth/geglättet (kein Zittern)
- Volle Range nutzbar

### 3. Konfiguration (WICHTIG!)
- **Auslöser**: 2 Buttons gleichzeitig halten → Config-Modus
- **Web Bluetooth**: Konfiguration über Browser (Chrome/Edge)
- **Keine Installation nötig**: Webseite öffnen, verbinden, fertig
- **Plattformen**: Windows, Linux, macOS, Android
- **NICHT unterstützt**: iOS (Apple blockiert Web Bluetooth)

## Architektur

```
┌─────────────────────────────────────────────────────────────┐
│                      NORMAL MODE                             │
│                                                              │
│   [Button] ──→ [Makro aus NVS laden] ──→ [BLE HID senden]   │
│   [Poti]   ──→ [Smoothing] ──→ [Volume Up/Down senden]      │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                      CONFIG MODE                             │
│         (Button 1 + Button 9 gleichzeitig halten)           │
│                                                              │
│   [ESP32 BLE Config Service] ←──→ [Web Browser]             │
│                                                              │
│   Browser (Chrome):                                          │
│   ┌─────────────────────────────────────────┐               │
│   │  esp-key Configurator                   │               │
│   │  ┌─────────────────────────────────┐    │               │
│   │  │ Button 1: [Email eingeben    ] │    │               │
│   │  │ Button 2: [Ctrl+C            ] │    │               │
│   │  │ Button 3: [Passwort123!      ] │    │               │
│   │  │ ...                            │    │               │
│   │  └─────────────────────────────────┘    │               │
│   │  [Speichern]  [Profile]  [Export]       │               │
│   └─────────────────────────────────────────┘               │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Makro-Format (Rubber Ducky Style)

```
STRING Hallo Welt!
DELAY 500
CTRL c
DELAY 100
ALT TAB
STRING Eingefügt:
CTRL v
ENTER
```

## Speicher (NVS)

- **9 Makro-Slots** (erweiterbar mit Profilen)
- **Max 1024 Bytes pro Makro**
- **Format**: Komprimiertes Binärformat oder Text

## BLE Services

### HID Service (Normal Mode)
- Standard BLE HID Keyboard
- Consumer Control (Media Keys)

### Config Service (Config Mode)
- **UUID**: Custom
- **Characteristics**:
  - `MACRO_READ`: Makro lesen (Button-Index → Makro-Daten)
  - `MACRO_WRITE`: Makro schreiben (Button-Index + Daten)
  - `CONFIG`: Allgemeine Einstellungen
  - `SAVE`: In NVS speichern

## Web Configurator

- **Hosting**: GitHub Pages (kostenlos, HTTPS)
- **Technologie**: HTML + JavaScript + Web Bluetooth API
- **Features**:
  - Verbindung zum ESP32 via BLE
  - Makro-Editor mit Syntax-Highlighting
  - Drag & Drop Reihenfolge
  - Profile verwalten
  - Import/Export (JSON)
  - Live-Test (Makro direkt senden)

## Entscheidungen

- **Display**: Nein
- **Feedback**: 5x NeoPixel (WS2812B) auf GPIO 5
- **Config-Modus**: Button 1 + Button 9 für 3 Sekunden halten
- **Batterie**: Noch offen

## NeoPixel System (wie WLED)

### Hardware
- **5x WS2812B** auf GPIO 5
- **Stromversorgung**: 5V oder 3.3V (mit Levelshifter falls nötig)

### Tasten-Kombos für LED-Steuerung

| Kombo | Funktion |
|-------|----------|
| 1+2 | Farbe hoch (Farbrad) |
| 1+3 | Farbe runter |
| 1+4 | Helligkeit +/- / Aus (Toggle) |
| 1+5 | Effekt durchschalten |
| 1+9 (3 Sek) | Config-Modus |

### Effekte (5 Standard)

| # | Effekt | Beschreibung |
|---|--------|--------------|
| 0 | Solid | Einfarbig |
| 1 | Breathing | Pulsieren |
| 2 | Rainbow | Regenbogen-Zyklus |
| 3 | Chase | Lauflicht |
| 4 | Sparkle | Funkeln |

### Status-Anzeigen

| Zustand | LED-Anzeige |
|---------|-------------|
| Keyboard-Modus | Gewählter Effekt/Farbe |
| Button gedrückt | Kurz heller |
| Config-Modus | Langsames Pulsieren Weiß |
| BLE verbunden | 1x Grün blinken |
| BLE getrennt | Langsam Rot pulsieren |

### UI-Einstellungen (Web Configurator)

- **Color Picker**: HSV Farbrad
- **Helligkeit**: Slider 0-255
- **Effekt**: Dropdown mit Vorschau
- **Geschwindigkeit**: Slider für Effekt-Tempo
- **Standard-Modus**: Was beim Einschalten aktiv ist
- **Auto-Off**: LEDs nach X Minuten aus
