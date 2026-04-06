#include "coreiot.h"

#include "global.h"
#include "taskTempHumi.h"

// ----------- CORE IOT CONFIG -----------
static const char* COREIOT_SERVER = "app.coreiot.io";
static const int COREIOT_PORT = 1883;
static const char* COREIOT_TOKEN = "w09XtPjzV9UJ4IpSXSih";  // ThingsBoard/CoreIoT access token

// ----------- GATEWAY MQTT CONFIG -----------
static const char* GATEWAY_HOST = "192.168.10.114";
static const int GATEWAY_PORT = 1883;
static const char* GATEWAY_USERNAME = "mqttclient";
static const char* GATEWAY_PASSWORD = "12345678";
static const char* GATEWAY_TOPIC = "tinyml/gateway/telemetry";

static const char* COREIOT_TELEMETRY_TOPIC = "v1/devices/me/telemetry";

enum class UplinkMode {
    Gateway,
    DirectCoreIoT
};

WiFiClient gatewayNetClient;
WiFiClient coreiotNetClient;
PubSubClient gatewayMqttClient(gatewayNetClient);
PubSubClient coreiotMqttClient(coreiotNetClient);

static UplinkMode g_uplinkMode = UplinkMode::DirectCoreIoT;

static void rpcCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("RPC arrived [");
    Serial.print(topic);
    Serial.println("]");

    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* method = doc["method"] | "";
    if (strcmp(method, "setStateLED") == 0) {
        const char* params = doc["params"] | "";
        if (strcmp(params, "ON") == 0) {
            Serial.println("Device turned ON.");
        } else {
            Serial.println("Device turned OFF.");
        }
    } else {
        Serial.print("Unknown RPC method: ");
        Serial.println(method);
    }
}

static bool connectGatewayBroker() {
    if (gatewayMqttClient.connected()) {
        return true;
    }

    String clientId = "ESP32-GW-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.println("Connecting to Gateway broker...");

    bool ok = gatewayMqttClient.connect(clientId.c_str(), GATEWAY_USERNAME, GATEWAY_PASSWORD);

    if (ok) {
        Serial.println("Gateway broker connected (username/password hop le).");
        return true;
    }

    Serial.print("Gateway broker connect failed, rc=");
    Serial.println(gatewayMqttClient.state());
    return false;
}

static bool connectCoreIoTDirect() {
    if (coreiotMqttClient.connected()) {
        return true;
    }

    String clientId = "ESP32-Core-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.println("Connecting directly to CoreIoT...");

    bool ok = coreiotMqttClient.connect(clientId.c_str(), COREIOT_TOKEN, nullptr);

    if (ok) {
        Serial.println("Connected directly to CoreIoT.");
        coreiotMqttClient.subscribe("v1/devices/me/rpc/request/+");
        return true;
    }

    Serial.print("Direct CoreIoT connect failed, rc=");
    Serial.println(coreiotMqttClient.state());
    return false;
}

static void updateUplinkMode() {
    if (connectGatewayBroker()) {
        g_uplinkMode = UplinkMode::Gateway;
        return;
    }

    if (connectCoreIoTDirect()) {
        g_uplinkMode = UplinkMode::DirectCoreIoT;
        return;
    }

    g_uplinkMode = UplinkMode::DirectCoreIoT;
}

static bool publishTelemetry() {
    StaticJsonDocument<256> doc;
    doc["device_id"] = glob_device_id;
    doc["temperature"] = glob_temperature;
    doc["humidity"] = glob_humidity;
    doc["ts"] = millis();

    String payload;
    serializeJson(doc, payload);

    if (g_uplinkMode == UplinkMode::Gateway && gatewayMqttClient.connected()) {
        bool ok = gatewayMqttClient.publish(GATEWAY_TOPIC, payload.c_str());
        if (ok) {
            Serial.println("Published via Gateway: " + payload);
            return true;
        }

        Serial.println("Publish via Gateway failed -> fallback direct CoreIoT");
        gatewayMqttClient.disconnect();
        g_uplinkMode = UplinkMode::DirectCoreIoT;
    }

    if (!coreiotMqttClient.connected() && !connectCoreIoTDirect()) {
        return false;
    }

    bool ok = coreiotMqttClient.publish(COREIOT_TELEMETRY_TOPIC, payload.c_str());
    if (ok) {
        Serial.println("Published directly to CoreIoT: " + payload);
        return true;
    }

    Serial.println("Direct CoreIoT publish failed.");
    return false;
}

static void setup_coreiot() {
    gatewayMqttClient.setServer(GATEWAY_HOST, GATEWAY_PORT);
    coreiotMqttClient.setServer(COREIOT_SERVER, COREIOT_PORT);
    coreiotMqttClient.setCallback(rpcCallback);
}

void coreiot_task(void* pvParameters) {
    (void)pvParameters;

    setup_coreiot();

    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("coreiot_task: waiting WiFi connection...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            if (gatewayMqttClient.connected()) {
                gatewayMqttClient.disconnect();
            }
            if (coreiotMqttClient.connected()) {
                coreiotMqttClient.disconnect();
            }
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        updateUplinkMode();

        gatewayMqttClient.loop();
        coreiotMqttClient.loop();

        publishTelemetry();
        vTaskDelay(pdMS_TO_TICKS(glob_send_interval));
    }
}
