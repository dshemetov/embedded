#include "html.h"
#include "secrets.h" // WIFI_SSID and WIFI_PASSWORD
#include <ESPmDNS.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>

WebServer server(80);
uint8_t otaDone = 0;

void handleUpdate() {
    Serial.println("handleUpdate");
    size_t fsize = UPDATE_SIZE_UNKNOWN;
    if (server.hasArg("size")) {
        fsize = server.arg("size").toInt();
    }
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Receiving Update: %s, Size: %d\n",
                      upload.filename.c_str(), fsize);
        if (!Update.begin(fsize)) {
            otaDone = 0;
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) !=
            upload.currentSize) {
            Update.printError(Serial);
        } else {
            otaDone = 100 * Update.progress() / Update.size();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("Update Success: %u bytes\nRebooting...\n",
                          upload.totalSize);
        } else {
            Serial.printf("%s\n", Update.errorString());
            otaDone = 0;
        }
    }
}

void handleUpdateEnd() {
    server.sendHeader("Connection", "close");
    if (Update.hasError()) {
        server.send(502, "text/plain", Update.errorString());
    } else {
        server.sendHeader("Refresh", "10");
        server.sendHeader("Location", "/");
        server.send(307);
        ESP.restart();
    }
}

void wifiInit() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait for connection for 5 seconds, then move on.
    unsigned long startTime = millis(); // Get the current time
    while (!(WiFi.status() == WL_CONNECTED) &&
           ((millis() - startTime) < 5000)) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        MDNS.begin("esp32");
    } else {
        Serial.println("Failed to connect to WiFi");
    }
}

void webServerInit() {
    server.on(
        "/update", HTTP_POST, []() { handleUpdateEnd(); },
        []() { handleUpdate(); });
    server.on("/favicon.ico", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip");
        server.send_P(200, "image/x-icon", favicon_ico_gz, favicon_ico_gz_len);
    });
    server.onNotFound([]() { server.send(200, "text/html", indexHtml); });
    server.begin();
    Serial.printf("Web Server ready at http://esp32.local or http://%s\n",
                  WiFi.softAPIP().toString().c_str());
}