#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
constexpr uint8_t ApChannel = 6;
constexpr uint8_t ApMaxClients = 4;
constexpr uint32_t ClientLogIntervalMs = 2000;

const char* ApSsid = "ChimeraMultiFX";
const char* ApPassword = "chimerafx";
IPAddress ApIp(192, 168, 4, 1);
IPAddress ApGateway(192, 168, 4, 1);
IPAddress ApSubnet(255, 255, 255, 0);

WebServer server(80);
uint8_t lastClientCount = 0;
uint32_t lastClientLogMs = 0;

void sendCorsHeaders() {
	server.sendHeader("Access-Control-Allow-Origin", "*");
	server.sendHeader("Access-Control-Allow-Methods", "GET");
	server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() {
	sendCorsHeaders();
	server.send(200, "text/plain", "ChimeraMultiFX ESP32 control surface\n");
}

void handleHealth() {
	sendCorsHeaders();
	server.send(200, "application/json", "{\"ok\":true}");
}

void handleNotFound() {
	sendCorsHeaders();
	server.send(404, "application/json", "{\"error\":\"not_found\"}");
}

void setupRoutes() {
	server.on("/", HTTP_GET, handleRoot);
	server.on("/health", HTTP_GET, handleHealth);
	server.onNotFound(handleNotFound);
}

void setupAccessPoint() {
	WiFi.persistent(false);
	WiFi.disconnect(true, true);
	WiFi.mode(WIFI_AP);
	WiFi.setSleep(false);

	WiFi.softAPConfig(ApIp, ApGateway, ApSubnet);
	WiFi.softAP(ApSsid, ApPassword, ApChannel, false, ApMaxClients);
}

void logClientCount() {
	const uint32_t now = millis();
	const uint8_t clientCount = WiFi.softAPgetStationNum();
	if (clientCount != lastClientCount || (now - lastClientLogMs) >= ClientLogIntervalMs) {
		lastClientCount = clientCount;
		lastClientLogMs = now;
	}
}
}

void setup() {
	setupAccessPoint();

	setupRoutes();
	server.begin();
}

void loop() {
	server.handleClient();
	logClientCount();
}
