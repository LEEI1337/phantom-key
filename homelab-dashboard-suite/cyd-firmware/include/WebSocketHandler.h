/**
 * @file WebSocketHandler.h
 * @brief WebSocket Client für CYD Dashboard
 * @description Verbindet sich mit Backend, empfängt Live-Daten, sendet Touch-Events
 */

#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "ErrorCodes.h"
#include "RingBuffer.h"

// Callback-Typen
typedef std::function<void(const String& service, float value)> DataCallback;
typedef std::function<void(ErrorCode error)> ErrorCallback;
typedef std::function<void(bool connected)> ConnectionCallback;

class WebSocketHandler {
private:
    WebSocketsClient webSocket;
    String serverIP;
    uint16_t serverPort;
    String authToken;
    
    bool isConnected = false;
    unsigned long lastReconnectAttempt = 0;
    unsigned long reconnectInterval = 5000;
    
    // Callbacks
    DataCallback onDataCallback = nullptr;
    ErrorCallback onErrorCallback = nullptr;
    ConnectionCallback onConnectionCallback = nullptr;
    
    // Interne Buffer für verschiedene Services
    RingBuffer<float>* proxmoxBuffer = nullptr;
    RingBuffer<float>* dockerBuffer = nullptr;
    RingBuffer<float>* gpuBuffer = nullptr;
    RingBuffer<float>* networkBuffer = nullptr;
    
    void handleWebSocketMessage(uint8_t* payload, size_t length);
    void parseServiceData(JsonObject& doc);
    void parseConfigUpdate(JsonObject& doc);
    void parseErrorResponse(JsonObject& doc);
    
public:
    WebSocketHandler();
    ~WebSocketHandler();
    
    /**
     * @brief Initialisiert WebSocket Verbindung
     * @param ip Server IP Adresse
     * @param port Server Port
     * @param token Authentifizierungs-Token
     * @return ErrorCode OK bei Erfolg
     */
    ErrorCode begin(const String& ip, uint16_t port, const String& token);
    
    /**
     * @brief Haupt-Loop, muss regelmäßig aufgerufen werden
     */
    void loop();
    
    /**
     * @brief Setzt Callback für eingehende Daten
     */
    void onData(DataCallback callback);
    
    /**
     * @brief Setzt Callback für Fehler
     */
    void onError(ErrorCallback callback);
    
    /**
     * @brief Setzt Callback für Verbindungsstatus
     */
    void onConnection(ConnectionCallback callback);
    
    /**
     * @brief Registriert RingBuffer für einen Service
     */
    void registerBuffer(const String& serviceName, RingBuffer<float>* buffer);
    
    /**
     * @brief Sendet Touch-Event an Backend
     */
    void sendTouchEvent(const String& widgetId, const String& action, int x, int y);
    
    /**
     * @brief Sendet Konfigurations-Update
     */
    void sendConfigUpdate(const String& key, const String& value);
    
    /**
     * @brief Prüft Verbindungsstatus
     */
    bool connected() const { return isConnected; }
    
    /**
     * @brief Erzwingt Reconnect
     */
    void reconnect();
};

#endif // WEBSOCKET_HANDLER_H
