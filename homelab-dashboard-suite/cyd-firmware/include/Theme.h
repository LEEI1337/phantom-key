#ifndef THEME_H
#define THEME_H

#include <TFT_eSPI.h>

// Farbpalette für das Dashboard
namespace Theme {
    // Hauptfarben
    constexpr uint32_t BACKGROUND      = 0x0F1419; // Dunkles Blau-Grau
    constexpr uint32_t CARD_BG         = 0x1A2028; // Etwas heller für Karten
    constexpr uint32_t CARD_BORDER     = 0x2D3748; // Rahmenfarbe
    
    // Status Farben
    constexpr uint32_t SUCCESS         = 0x00C853; // Grün
    constexpr uint32_t WARNING         = 0xFFAB00; // Gelb/Orange
    constexpr uint32_t ERROR           = 0xFF3D00; // Rot
    constexpr uint32_t INFO            = 0x29B6F6; // Hellblau
    
    // Akzentfarben
    constexpr uint32_t PRIMARY         = 0x7E57C2; // Lila
    constexpr uint32_t SECONDARY       = 0xEC407A; // Pink
    constexpr uint32_t ACCENT          = 0x26C6DA; // Cyan
    
    // Text Farben
    constexpr uint32_t TEXT_PRIMARY    = 0xFFFFFF;
    constexpr uint32_t TEXT_SECONDARY  = 0xB0BEC5;
    constexpr uint32_t TEXT_DISABLED   = 0x546E7A;
    
    // Gradient Farben für Charts
    constexpr uint32_t GRADIENT_START  = 0x7E57C2;
    constexpr uint32_t GRADIENT_END    = 0x26C6DA;
    
    // Spezialfarben für Dienste
    constexpr uint32_t PROXMOX_COLOR   = 0xE67E22; // Orange
    constexpr uint32_t DOCKER_COLOR    = 0x2496ED; // Blau
    constexpr uint32_t OMV_COLOR       = 0x5DACDF; // Hellblau
    constexpr uint32_t PFSENSE_COLOR   = 0x212121; // Dunkelgrau
    constexpr uint32_t GPU_COLOR       = 0x76B900; // NVIDIA Grün
    constexpr uint32_t HA_COLOR        = 0x41BDF5; // Home Assistant Blau
}

// Schriftarten und Größen
namespace Fonts {
    constexpr uint8_t SMALL      = 2;
    constexpr uint8_t MEDIUM     = 4;
    constexpr uint8_t LARGE      = 6;
    constexpr uint8_t XLARGE     = 8;
}

// Abstände und Größen
namespace Layout {
    constexpr int PADDING_SMALL   = 4;
    constexpr int PADDING_MEDIUM  = 8;
    constexpr int PADDING_LARGE   = 16;
    constexpr int RADIUS_SMALL    = 4;
    constexpr int RADIUS_MEDIUM   = 8;
    constexpr int RADIUS_LARGE    = 12;
}

#endif // THEME_H
