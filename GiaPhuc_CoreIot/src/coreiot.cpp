#include "coreiot.h"

// ----------- CONFIGURE THESE! -----------
const char* coreIOT_Server = "app.coreiot.io";  
const char* coreIOT_Token = "yr1akiur4otyf6wzqa94";   // Device Access Token
const int   mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);


void reconnect() {
  // Non-blocking reconnection attempt
  if (client.connected()) {
    return;  // Already connected
  }
  
  // Check WiFi status (non-blocking)
  if (WiFi.status() != WL_CONNECTED) {
    return;  // WiFi not ready, will retry next time
  }
  
  if (WiFi.dnsIP() == IPAddress(0, 0, 0, 0)) {
    return;  // DNS not ready, will retry next time
  }
  
  // Try to connect (non-blocking attempt)
  Serial.print("[CoreIOT] Attempting MQTT connection...");
  if (client.connect("ESP32", coreIOT_Token, "")) {
    Serial.println("✅ Connected to CoreIOT!");
    client.subscribe("v1/devices/me/rpc/request/+");
    Serial.println("📡 Subscribed to v1/devices/me/rpc/request/+");
  } else {
    Serial.print("❌ MQTT failed (rc=");
    Serial.print(client.state());
    Serial.println("), will retry later");
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  // Allocate a temporary buffer for the message
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Payload: ");
  Serial.println(message);

  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* method = doc["method"];
  if (strcmp(method, "setStateLED") == 0) {
    // Check params type (could be boolean, int, or string according to your RPC)
    // Example: {"method": "setValueLED", "params": "ON"}
    const char* params = doc["params"];

    if (strcmp(params, "ON") == 0) {
      Serial.println("Device turned ON.");
      //TODO

    } else {   
      Serial.println("Device turned OFF.");
      //TODO

    }
  } else {
    Serial.print("Unknown method: ");
    Serial.println(method);
  }
}


void setup_coreiot(){
  Serial.println("\n[CoreIOT] Module initialized, waiting for WiFi...");
  
  // Setup MQTT client first (non-blocking)
  client.setServer(coreIOT_Server, mqttPort);
  client.setCallback(callback);
  
  Serial.println("[CoreIOT] Ready! Will connect when WiFi is available.");
}

void coreiot_task(void *pvParameters){
    setup_coreiot();
    
    uint32_t lastPublish = 0;
    const uint32_t publishInterval = 10000;  // Publish every 10 seconds
    uint32_t lastReconnectAttempt = 0;
    const uint32_t reconnectInterval = 5000;  // Retry connection every 5 seconds

    while(1) {
        // Non-blocking MQTT connection management
        uint32_t now = millis();
        
        // Try to reconnect every 5 seconds if not connected
        if (!client.connected() && (now - lastReconnectAttempt) > reconnectInterval) {
            lastReconnectAttempt = now;
            Serial.println("[CoreIOT] Checking connection...");
            reconnect();
        }
        
        // Process MQTT messages (non-blocking)
        if (client.connected()) {
            client.loop();
            
            // Publish telemetry data at intervals (only if connected)
            if ((now - lastPublish) > publishInterval) {
                lastPublish = now;
                String payload = "{\"temperature\":" + String(glob_temperature, 2) + 
                                ",\"humidity\":" + String(glob_humidity, 2) + "}";
                
                if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
                    Serial.println("[CoreIOT] 📤 Published: " + payload);
                } else {
                    Serial.println("[CoreIOT] ❌ Publish failed!");
                }
            }
        }
        
        // Yield CPU to other tasks
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}