#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "BridgeServer.h"
#include "DaisyBridge.h"
#include "WifiManager.h"

namespace {
WebServer server(80);
WebSocketsServer webSocket(81);
}

void setup() {
	Serial.begin(115200);
	delay(200);
	Serial.println("ChimeraMultiFX bridge boot");
	WifiManager::connect();
	DaisyBridge::begin();
	if (!LittleFS.begin(true)) {
		Serial.println("LittleFS mount failed");
	}
	BridgeServer::setupRoutes(server, webSocket);
	server.begin();
	webSocket.begin();
	Serial.println("HTTP server started on port 80; WebSocket on port 81");
}

void loop() {
	webSocket.loop();
	server.handleClient();
	WifiManager::maintain();
	delay(1);
}
