#include "taskWebServer.h"

#include <LittleFS.h>
#include <Preferences.h>

#include "global.h"
// Định nghĩa các biến toàn cục
String wifi_ssid = "";
String wifi_password = "";
bool wifi_configured = false;
String glob_device_id = DEFAULT_DEVICE_ID;
unsigned long glob_send_interval = DEFAULT_SEND_INTERVAL;
bool glob_led_enabled = true;

WebServer server(80);

String testWiFiConnection(String ssid, String password) {
    Serial.println("Testing WiFi connection...");
    Serial.print("SSID: ");
    Serial.println(ssid);
    WiFi.disconnect();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (password.length() > 0) {
        WiFi.begin(ssid.c_str(), password.c_str());
    } else {
        WiFi.begin(ssid.c_str());
    }
    int attempts = 0;
    const int maxAttempts = 20;

    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        attempts++;
    }
    wl_status_t status = WiFi.status();
    WiFi.disconnect();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    switch (status) {
        case WL_CONNECTED:
            return "SUCCESS";
        case WL_NO_SSID_AVAIL:
            return "SSID_NOT_FOUND";
        case WL_CONNECT_FAILED:
            return "WRONG_PASSWORD";
        case WL_DISCONNECTED:
        case WL_CONNECTION_LOST:
            return "CONNECTION_FAILED";
        default:
            return "TIMEOUT";
    }
}

// Helper functions for Preferences (local to this file, no Firebase)
bool loadDeviceConfigFromPreferences(String& device_id, unsigned long& send_interval) {
    Preferences prefs;
    prefs.begin("device_config", true);
    device_id = prefs.getString("device_id", DEFAULT_DEVICE_ID);
    send_interval = prefs.getULong("send_interval", DEFAULT_SEND_INTERVAL);
    prefs.end();

    if (device_id.length() > 0 && send_interval > 0) {
        return true;
    }
    return false;
}

bool saveDeviceConfigToPreferences(const String& device_id, unsigned long send_interval) {
    Preferences prefs;
    prefs.begin("device_config", false);
    bool success = prefs.putString("device_id", device_id) &&
                   prefs.putULong("send_interval", send_interval);
    prefs.end();
    return success;
}

bool loadAPConfigFromPreferences(String& ap_ssid, String& ap_password) {
    Preferences prefs;
    prefs.begin("ap_config", true);
    ap_ssid = prefs.getString("ssid", DEFAULT_AP_SSID);
    ap_password = prefs.getString("password", DEFAULT_AP_PASSWORD);
    prefs.end();

    if (ap_ssid.length() > 0) {
        return true;
    }
    return false;
}

bool saveAPConfigToPreferences(const String& ap_ssid, const String& ap_password) {
    Preferences prefs;
    prefs.begin("ap_config", false);
    bool success = prefs.putString("ssid", ap_ssid) && prefs.putString("password", ap_password);
    prefs.end();
    return success;
}

String escapeJsonString(String str) {
    String escaped = "";
    for (unsigned int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (c == '"') {
            escaped += "\\\"";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else if (c == '\n') {
            escaped += "\\n";
        } else if (c == '\r') {
            escaped += "\\r";
        } else if (c == '\t') {
            escaped += "\\t";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

String getContentType(String filename) {
    if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".png"))
        return "image/png";
    else if (filename.endsWith(".jpg"))
        return "image/jpeg";
    else if (filename.endsWith(".gif"))
        return "image/gif";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else if (filename.endsWith(".json"))
        return "application/json";
    return "text/plain";
}

bool handleFileRead(String path) {
    Serial.println("handleFileRead: " + path);
    if (path.endsWith("/")) {
        path += "index.html";
    }
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) {
        if (LittleFS.exists(pathWithGz)) {
            path += ".gz";
        }
        File file = LittleFS.open(path, "r");
        if (server.streamFile(file, contentType) != file.size()) {
            Serial.println("Sent less data than expected!");
        }
        file.close();
        return true;
    }
    Serial.println("\tFile Not Found");
    return false;
}

void handleRoot() {
    if (handleFileRead("/index.html")) {
        return;
    }

    server.send(404, "text/plain", "FileNotFound");
}

void handleConfig() {
    String device_id;
    unsigned long send_interval;
    loadDeviceConfigFromPreferences(device_id, send_interval);

    String ap_ssid, ap_password;
    loadAPConfigFromPreferences(ap_ssid, ap_password);

    String json = "{";
    json += "\"device_id\":\"" + device_id + "\",";
    json += "\"send_interval\":" + String(send_interval) + ",";
    json += "\"ap_ssid\":\"" + ap_ssid + "\",";
    json += "\"ap_password\":\"" + ap_password + "\",";
    json += "\"led_state\":" + String(glob_led_enabled ? 1 : 0);
    json += "}";

    server.send(200, "application/json", json);
}

void handleTest() {
    if (!server.hasArg("ssid")) {
        server.send(400, "text/plain", "Error: SSID is required");
        return;
    }

    String ssid = server.arg("ssid");
    ssid.trim();

    if (ssid.length() == 0) {
        server.send(400, "text/plain", "Error: SSID cannot be empty");
        return;
    }

    String password = server.arg("password");

    Serial.println("====================");
    Serial.println("Testing WiFi connection from web form:");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password.length() > 0 ? "***" : "(empty)");
    String result = testWiFiConnection(ssid, password);
    Serial.print("Test result: ");
    Serial.println(result);
    Serial.println("====================");

    server.send(200, "text/plain", result);
}

void handleConnect() {
    if (!server.hasArg("ssid")) {
        server.send(400, "text/plain", "Error: SSID is required");
        return;
    }
    String ssid = server.arg("ssid");
    ssid.trim();
    if (ssid.length() == 0) {
        server.send(400, "text/plain", "Error: SSID cannot be empty");
        return;
    }
    if (ssid.length() > 32) {
        server.send(400, "text/plain", "Error: SSID must be 32 characters or less");
        return;
    }
    String password = server.arg("password");
    if (password.length() > 64) {
        server.send(400, "text/plain", "Error: Password must be 64 characters or less");
        return;
    }
    wifi_ssid = ssid;
    wifi_password = password;
    wifi_configured = true;
    Serial.println("====================");
    Serial.println("WiFi Configuration saved:");
    Serial.print("SSID: ");
    Serial.println(wifi_ssid);
    Serial.print("Password: ");
    Serial.println(wifi_password.length() > 0 ? "***" : "(empty - open network)");
    Serial.println("====================");

    server.send(200, "text/plain", "WiFi configured successfully! SSID: " + wifi_ssid);
}

void handleDevice() {
    if (!server.hasArg("device_id") || !server.hasArg("send_interval")) {
        server.send(400, "text/plain", "Error: device_id and send_interval are required");
        return;
    }

    String device_id = server.arg("device_id");
    device_id.trim();
    if (device_id.length() == 0 || device_id.length() > 32) {
        server.send(400, "text/plain", "Error: device_id must be 1-32 characters");
        return;
    }

    unsigned long send_interval = server.arg("send_interval").toInt();
    if (send_interval < 1000 || send_interval > 600000) {
        server.send(400, "text/plain", "Error: send_interval must be 1000-600000 ms");
        return;
    }

    saveDeviceConfigToPreferences(device_id, send_interval);
    glob_device_id = device_id;
    glob_send_interval = send_interval;  // Cập nhật biến toàn cục

    Serial.println("====================");
    Serial.println("Device Configuration saved:");
    Serial.print("Device ID: ");
    Serial.println(device_id);
    Serial.print("Send Interval: ");
    Serial.print(send_interval);
    Serial.println(" ms");
    Serial.println("====================");

    server.send(200, "text/plain", "Device config saved successfully!");
}

void handleScan() {
    Serial.println("WiFi scan requested...");
    
    // Disconnect from current WiFi if connected to allow scanning
    WiFi.disconnect();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // Start scan (blocking call)
    int n = WiFi.scanNetworks();
    
    if (n < 0) {
        server.send(500, "application/json", "{\"error\":\"Scan failed\"}");
        Serial.println("WiFi scan failed");
        return;
    }
    
    if (n == 0) {
        server.send(200, "application/json", "[]");
        Serial.println("No networks found");
        return;
    }
    
    // Build JSON array
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + escapeJsonString(WiFi.SSID(i)) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? 0 : 1);
        json += "}";
    }
    json += "]";
    
    Serial.print("Found ");
    Serial.print(n);
    Serial.println(" networks");
    
    server.send(200, "application/json", json);
}

void handleAP() {
    if (!server.hasArg("ap_ssid")) {
        server.send(400, "text/plain", "Error: ap_ssid is required");
        return;
    }

    String ap_ssid = server.arg("ap_ssid");
    ap_ssid.trim();
    if (ap_ssid.length() == 0 || ap_ssid.length() > 32) {
        server.send(400, "text/plain", "Error: ap_ssid must be 1-32 characters");
        return;
    }

    String ap_password = server.arg("ap_password");
    if (ap_password.length() > 64) {
        server.send(400, "text/plain", "Error: ap_password must be 64 characters or less");
        return;
    }

    saveAPConfigToPreferences(ap_ssid, ap_password);

    Serial.println("====================");
    Serial.println("AP Configuration saved:");
    Serial.print("AP SSID: ");
    Serial.println(ap_ssid);
    Serial.print("AP Password: ");
    Serial.println(ap_password.length() > 0 ? "***" : "(empty)");
    Serial.println("Note: ESP32 needs to restart to apply new AP config");
    Serial.println("====================");

    String response = "AP config saved successfully! Restart required for AP changes.";
    server.send(200, "text/plain", response);
}

void handleLedControl() {
    if (!server.hasArg("led_state")) {
        server.send(400, "text/plain", "Error: led_state is required");
        return;
    }

    String ledState = server.arg("led_state");
    ledState.trim();

    if (ledState != "0" && ledState != "1") {
        server.send(400, "text/plain", "Error: led_state must be 0 or 1");
        return;
    }

    glob_led_enabled = (ledState == "1");

    Serial.println("====================");
    Serial.println("LED Control updated:");
    Serial.print("LED Enabled: ");
    Serial.println(glob_led_enabled ? "ON" : "OFF");
    Serial.println("====================");

    server.send(200, "text/plain", "LED control updated successfully!");
}

void taskWebServer(void* pvParameters) {
    Serial.println("Task Web Server: Starting...");

    // Load device config from Preferences (bao gồm send_interval)
    String device_id;
    unsigned long send_interval;
    if (loadDeviceConfigFromPreferences(device_id, send_interval)) {
        glob_device_id = device_id;
        glob_send_interval = send_interval;
        Serial.println("Device config loaded from Preferences:");
        Serial.print("  Device ID: ");
        Serial.println(device_id);
    } else {
        Serial.println("Using default device config");
        glob_device_id = DEFAULT_DEVICE_ID;
        glob_send_interval = DEFAULT_SEND_INTERVAL;
    }

    // Initialize LittleFS
    bool fsMounted = LittleFS.begin(true);
    if (fsMounted) {
        Serial.println("LittleFS mounted successfully");

        // List files in LittleFS
        File root = LittleFS.open("/");
        Serial.println("Files in LittleFS:");
        File file = root.openNextFile();
        bool hasFiles = false;
        while (file) {
            hasFiles = true;
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
            file = root.openNextFile();
        }
        if (!hasFiles) {
            Serial.println("  (No files found)");
        }
    } else {
        Serial.println("LittleFS Mount Failed - will use inline HTML fallback");
        Serial.println("To use files from LittleFS, upload filesystem image:");
        Serial.println("  pio run --target uploadfs");
    }

    // Load AP config from Preferences
    String ap_ssid, ap_password;
    Preferences prefs;
    prefs.begin("ap_config", true);
    ap_ssid = prefs.getString("ssid", DEFAULT_AP_SSID);
    ap_password = prefs.getString("password", DEFAULT_AP_PASSWORD);
    prefs.end();

    Serial.println("Creating WiFi Access Point...");
    Serial.print("AP SSID: ");
    Serial.println(ap_ssid);
    Serial.print("AP Password: ");
    Serial.println(ap_password);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.println("Connect to this WiFi network and open http://192.168.4.1");

    // Serve static files from LittleFS
    server.onNotFound([]() {
        if (!handleFileRead(server.uri())) {
            server.send(404, "text/plain", "FileNotFound");
        }
    });

    // API endpoints
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/scan", handleScan);
    server.on("/test", HTTP_POST, handleTest);
    server.on("/connect", HTTP_POST, handleConnect);
    server.on("/device", HTTP_POST, handleDevice);
    server.on("/ap", HTTP_POST, handleAP);
    server.on("/led", HTTP_POST, handleLedControl);

    server.begin();
    Serial.println("Web server started!");

    while (1) {
        server.handleClient();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
