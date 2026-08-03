#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <cstdint>
#include <string>

// Einheitliches Error-Handling System für alle Komponenten

// Error Level
enum class ErrorLevel : uint8_t {
    INFO     = 0x00,
    WARNING  = 0x01,
    ERROR    = 0x02,
    CRITICAL = 0x03
};

// Error Categories
enum class ErrorCategory : uint8_t {
    SYSTEM      = 0x00,
    NETWORK     = 0x01,
    SENSOR      = 0x02,
    STORAGE     = 0x03,
    UI          = 0x04,
    COMMUNICATION = 0x05,
    CONFIG      = 0x06,
    MEMORY      = 0x07
};

// Error Code Struktur
struct ErrorCode {
    ErrorLevel level;
    ErrorCategory category;
    uint8_t code;
    
    constexpr ErrorCode(ErrorLevel l, ErrorCategory c, uint8_t n) 
        : level(l), category(c), code(n) {}
    
    constexpr uint16_t toUint() const {
        return (static_cast<uint16_t>(level) << 12) | 
               (static_cast<uint16_t>(category) << 8) | 
               code;
    }
    
    static constexpr ErrorCode fromUint(uint16_t val) {
        return ErrorCode(
            static_cast<ErrorLevel>((val >> 12) & 0x0F),
            static_cast<ErrorCategory>((val >> 8) & 0x0F),
            val & 0xFF
        );
    }
};

// Vordefinierte Error Codes
namespace Errors {
    // System Errors (0x00xx)
    constexpr ErrorCode OK                  = {ErrorLevel::INFO, ErrorCategory::SYSTEM, 0x00};
    constexpr ErrorCode SYS_INIT_FAILED     = {ErrorLevel::CRITICAL, ErrorCategory::SYSTEM, 0x01};
    constexprErrorCode SYS_WATCHDOG_RESET   = {ErrorLevel::WARNING, ErrorCategory::SYSTEM, 0x02};
    constexpr ErrorCode SYS_LOW_MEMORY      = {ErrorLevel::WARNING, ErrorCategory::SYSTEM, 0x03};
    
    // Network Errors (0x01xx)
    constexpr ErrorCode NET_WIFI_CONNECT    = {ErrorLevel::ERROR, ErrorCategory::NETWORK, 0x01};
    constexpr ErrorCode NET_WIFI_LOST       = {ErrorLevel::ERROR, ErrorCategory::NETWORK, 0x02};
    constexpr ErrorCode NET_MQTT_CONNECT    = {ErrorLevel::ERROR, ErrorCategory::NETWORK, 0x03};
    constexpr ErrorCode NET_MQTT_PUBLISH    = {ErrorLevel::WARNING, ErrorCategory::NETWORK, 0x04};
    constexpr ErrorCode NET_WEBSOCKET_FAIL  = {ErrorLevel::ERROR, ErrorCategory::NETWORK, 0x05};
    constexpr ErrorCode NET_TIMEOUT         = {ErrorLevel::WARNING, ErrorCategory::NETWORK, 0x06};
    constexpr ErrorCode NET_DNS_FAILED      = {ErrorLevel::ERROR, ErrorCategory::NETWORK, 0x07};
    
    // Sensor Errors (0x02xx)
    constexpr ErrorCode SENSOR_READ_FAILED  = {ErrorLevel::WARNING, ErrorCategory::SENSOR, 0x01};
    constexpr ErrorCode SENSOR_OUT_OF_RANGE = {ErrorLevel::WARNING, ErrorCategory::SENSOR, 0x02};
    constexpr ErrorCode SENSOR_NOT_FOUND    = {ErrorLevel::ERROR, ErrorCategory::SENSOR, 0x03};
    
    // Storage Errors (0x03xx)
    constexpr ErrorCode STORAGE_WRITE_FAIL  = {ErrorLevel::ERROR, ErrorCategory::STORAGE, 0x01};
    constexpr ErrorCode STORAGE_READ_FAIL   = {ErrorLevel::ERROR, ErrorCategory::STORAGE, 0x02};
    constexpr ErrorCode STORAGE_FULL        = {ErrorLevel::WARNING, ErrorCategory::STORAGE, 0x03};
    constexpr ErrorCode STORAGE_CORRUPT     = {ErrorLevel::CRITICAL, ErrorCategory::STORAGE, 0x04};
    
    // UI Errors (0x04xx)
    constexpr ErrorCode UI_DRAW_FAILED      = {ErrorLevel::WARNING, ErrorCategory::UI, 0x01};
    constexpr ErrorCode UI_TOUCH_ERROR      = {ErrorLevel::WARNING, ErrorCategory::UI, 0x02};
    
    // Communication Errors (0x05xx)
    constexpr ErrorCode COMM_INVALID_DATA   = {ErrorLevel::WARNING, ErrorCategory::COMMUNICATION, 0x01};
    constexpr ErrorCode COMM_CHECKSUM_FAIL  = {ErrorLevel::ERROR, ErrorCategory::COMMUNICATION, 0x02};
    constexpr ErrorCode COMM_TIMEOUT        = {ErrorLevel::WARNING, ErrorCategory::COMMUNICATION, 0x03};
    
    // Config Errors (0x06xx)
    constexpr ErrorCode CONFIG_NOT_FOUND    = {ErrorLevel::ERROR, ErrorCategory::CONFIG, 0x01};
    constexpr ErrorCode CONFIG_INVALID      = {ErrorLevel::ERROR, ErrorCategory::CONFIG, 0x02};
    constexpr ErrorCode CONFIG_SAVE_FAILED  = {ErrorLevel::ERROR, ErrorCategory::CONFIG, 0x03};
    
    // Memory Errors (0x07xx)
    constexpr ErrorCode MEM_ALLOC_FAILED    = {ErrorLevel::CRITICAL, ErrorCategory::MEMORY, 0x01};
    constexpr ErrorCode MEM_FRAGMENTED      = {ErrorLevel::WARNING, ErrorCategory::MEMORY, 0x02};
}

// Error Handler Klasse
class ErrorHandler {
private:
    static ErrorCode lastError;
    static uint32_t errorCount;
    static uint32_t lastErrorTime;
    
public:
    static void set(ErrorCode err) {
        lastError = err;
        errorCount++;
        lastErrorTime = millis();
        
        // Log output basierend auf Level
        if (err.level == ErrorLevel::CRITICAL || err.level == ErrorLevel::ERROR) {
            Serial.printf("[ERROR] %04X at %lu\n", err.toUint(), lastErrorTime);
        } else if (err.level == ErrorLevel::WARNING) {
            Serial.printf("[WARN] %04X at %lu\n", err.toUint(), lastErrorTime);
        }
    }
    
    static ErrorCode get() { return lastError; }
    
    static bool isOk() { return lastError.toUint() == Errors::OK.toUint(); }
    
    static uint32_t getCount() { return errorCount; }
    
    static uint32_t getTimeSinceLast() { return millis() - lastErrorTime; }
    
    static void clear() {
        lastError = Errors::OK;
    }
    
    static std::string getDescription(ErrorCode err) {
        switch(err.toUint()) {
            case 0x0000: return "OK - No error";
            case 0x0001: return "System initialization failed";
            case 0x0101: return "WiFi connection failed";
            case 0x0102: return "WiFi connection lost";
            case 0x0103: return "MQTT connection failed";
            case 0x0301: return "Storage write failed";
            case 0x0701: return "Memory allocation failed";
            default: return "Unknown error";
        }
    }
};

// Globale Variablen für ErrorHandler
ErrorCode ErrorHandler::lastError = Errors::OK;
uint32_t ErrorHandler::errorCount = 0;
uint32_t ErrorHandler::lastErrorTime = 0;

// Debug Macros
#ifdef DEBUG
    #define LOG_INFO(msg) Serial.println("[INFO] " msg)
    #define LOG_WARN(msg) Serial.println("[WARN] " msg)
    #define LOG_ERROR(msg) Serial.println("[ERROR] " msg)
    #define LOG_DEBUG(msg) Serial.println("[DEBUG] " msg)
#else
    #define LOG_INFO(msg)
    #define LOG_WARN(msg)
    #define LOG_ERROR(msg)
    #define LOG_DEBUG(msg)
#endif

#define CHECK_ERROR(expr) do { \
    if (!(expr)) { \
        ErrorHandler::set(Errors::SYS_INIT_FAILED); \
        return false; \
    } \
} while(0)

#define RETURN_ON_ERROR(expr) do { \
    auto _err = (expr); \
    if (_err.toUint() != Errors::OK.toUint()) { \
        return _err; \
    } \
} while(0)

#endif // ERROR_CODES_H
