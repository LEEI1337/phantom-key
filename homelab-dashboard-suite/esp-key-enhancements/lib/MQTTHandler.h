/**
 * @file MQTTHandler.h
 * @brief MQTT Client für ESP-Key Enhancements
 * @description Verbindet sich mit MQTT Broker, publiziert/subscribiert Topics
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "ErrorCodes.h"

// Callback-Typen
typedef std::function<void(const String& topic, const String& payload)> MqttMessageCallback;
typedef std::function<void(bool connected)> MqttConnectionCallback;

class MQTTHandler {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    String brokerIP;
    uint16_t brokerPort;
    String clientId;
    String username;
    String password;
    
    bool isConnected = false;
    unsigned long lastReconnectAttempt = 0;
    unsigned long reconnectInterval = 5000;
    unsigned long lastPingTime = 0;
    const unsigned long PING_INTERVAL = 30000; // 30s
    
    // Callbacks
    MqttMessageCallback onMessageCallback = nullptr;
    MqttConnectionCallback onConnectionCallback = nullptr;
    
    // Interne Methoden
    void callback(char* topic, byte* payload, unsigned int length);
    void processMessage(const String& topic, const String& payload);
    
public:
    MQTTHandler();
    ~MQTTHandler();
    
    /**
     * @brief Initialisiert MQTT Client
     * @param ip Broker IP Adresse
     * @param port Broker Port (meist 1883)
     * @param clientID Eindeutige Client ID
     * @param user Username (optional, leer für keine Auth)
     * @param pass Password (optional)
     * @return ErrorCode OK bei Erfolg
     */
    ErrorCode begin(const String& ip, uint16_t port, 
                    const String& clientID, 
                    const String& user = "", 
                    const String& pass = "");
    
    /**
     * @brief Haupt-Loop, muss regelmäßig aufgerufen werden
     */
    void loop();
    
    /**
     * @brief Setzt Callback für eingehende Nachrichten
     */
    void onMessage(MqttMessageCallback callback);
    
    /**
     * @brief Setzt Callback für Verbindungsstatus
     */
    void onConnection(MqttConnectionCallback callback);
    
    /**
     * @brief Subscribe zu Topic
     * @param topic Topic Name (kann Wildcards enthalten)
     * @param qos Quality of Service (0, 1, 2)
     * @return ErrorCode
     */
    ErrorCode subscribe(const String& topic, uint8_t qos = 0);
    
    /**
     * @brief Unsubscribe von Topic
     */
    ErrorCode unsubscribe(const String& topic);
    
    /**
     * @brief Publish Nachricht
     * @param topic Topic Name
     * @param payload Nachricht
     * @param retained Als retained message speichern
     * @return ErrorCode
     */
    ErrorCode publish(const String& topic, const String& payload, bool retained = false);
    
    /**
     * @brief Publish JSON Objekt
     */
    ErrorCode publishJson(const String& topic, JsonObject& doc, bool retained = false);
    
    /**
     * @brief Prüft Verbindungsstatus
     */
    bool connected() const { return isConnected; }
    
    /**
     * @brief Erzwingt Reconnect
     */
    void reconnect();
    
    /**
     * @brief Setzt Last Will Testament
     */
    void setWill(const String& topic, const String& payload, bool retained = false);
};

#endif // MQTT_HANDLER_H
