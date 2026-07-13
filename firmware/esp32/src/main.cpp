#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include "wifi_credentials.h"

namespace {
constexpr uint8_t DaisyUartRxPin = 16;
constexpr uint8_t DaisyUartTxPin = 17;
constexpr uint32_t DaisyUartBaud = 115200;
constexpr uint32_t DaisyResponseTimeoutMs = 2000;
constexpr uint32_t LoopbackTimeoutMs = 250;
constexpr uint32_t WifiConnectTimeoutMs = 20000;
constexpr uint32_t WifiReconnectIntervalMs = 5000;
constexpr size_t DaisyResponseMaxLen = 4096;
constexpr size_t DaisyCommandMaxLen = 127;

WebServer server(80);
WebSocketsServer webSocket(81);
HardwareSerial daisySerial(2);

struct DaisyReply {
	String body;
	bool complete;
};

void sendCorsHeaders() {
	server.sendHeader("Access-Control-Allow-Origin", "*");
	server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
	server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void configureDaisyUart(uint8_t rxPin, uint8_t txPin) {
	daisySerial.end();
	daisySerial.setRxBufferSize(DaisyResponseMaxLen);
	daisySerial.begin(DaisyUartBaud, SERIAL_8N1, rxPin, txPin);
}

void clearDaisyInput() {
	while (daisySerial.available() > 0) {
		daisySerial.read();
	}
}

DaisyReply readDaisyLine(uint32_t timeoutMs) {
	String response;
	response.reserve(256);
	const uint32_t startMs = millis();

	while ((millis() - startMs) < timeoutMs) {
		while (daisySerial.available() > 0) {
			const char character = static_cast<char>(daisySerial.read());
			if (character == '\n') {
				return {response, true};
			}
			if (character != '\r' && response.length() < DaisyResponseMaxLen - 1) {
				response += character;
			}
		}
		delay(1);
	}

	return {response, false};
}

DaisyReply transactDaisyCommand(const String& command) {
	clearDaisyInput();
	daisySerial.print(command);
	daisySerial.print('\n');
	daisySerial.flush();
	return readDaisyLine(DaisyResponseTimeoutMs);
}

bool requestedPins(uint8_t& rxPin, uint8_t& txPin) {
	rxPin = DaisyUartRxPin;
	txPin = DaisyUartTxPin;

	if (server.hasArg("rx")) {
		const int value = server.arg("rx").toInt();
		if (value < 0 || value > 39) {
			server.send(400, "application/json", "{\"error\":\"invalid_rx_pin\"}");
			return false;
		}
		rxPin = static_cast<uint8_t>(value);
	}
	if (server.hasArg("tx")) {
		const int value = server.arg("tx").toInt();
		if (value < 0 || value > 39) {
			server.send(400, "application/json", "{\"error\":\"invalid_tx_pin\"}");
			return false;
		}
		txPin = static_cast<uint8_t>(value);
	}
	return true;
}

bool requestedGpio(uint8_t& pin) {
	if (!server.hasArg("pin")) {
		server.send(400, "application/json", "{\"error\":\"missing_pin\"}");
		return false;
	}
	const int value = server.arg("pin").toInt();
	if (value < 0 || value > 39) {
		server.send(400, "application/json", "{\"error\":\"invalid_pin\"}");
		return false;
	}
	pin = static_cast<uint8_t>(value);
	return true;
}

void handleRoot() {
	sendCorsHeaders();
	File index = LittleFS.open("/index.html", "r");
	if (!index) {
		server.send(503, "text/plain", "UI not installed. Run: pio run --target uploadfs\n");
		return;
	}
	server.streamFile(index, "text/html");
	index.close();
}

void handleHealth() {
	sendCorsHeaders();
	server.send(200, "application/json", "{\"ok\":true}");
}

void handleBridgePins() {
	sendCorsHeaders();
	const String response = "{\"uart\":2,\"rx_pin\":" + String(DaisyUartRxPin)
		+ ",\"tx_pin\":" + String(DaisyUartTxPin)
		+ ",\"baud\":" + String(DaisyUartBaud) + "}";
	server.send(200, "application/json", response);
}

void handleGpioRead() {
	sendCorsHeaders();
	uint8_t pin;
	if (!requestedGpio(pin)) return;

	daisySerial.end();
	pinMode(pin, INPUT_PULLDOWN);
	delay(1);
	const int value = digitalRead(pin);
	configureDaisyUart(DaisyUartRxPin, DaisyUartTxPin);
	server.send(200, "application/json",
		"{\"pin\":" + String(pin) + ",\"value\":" + String(value) + "}");
}

void handleGpioWrite() {
	sendCorsHeaders();
	uint8_t pin;
	if (!requestedGpio(pin)) return;
	if (!server.hasArg("value")) {
		server.send(400, "application/json", "{\"error\":\"missing_value\"}");
		return;
	}

	const bool value = server.arg("value").toInt() != 0;
	daisySerial.end();
	pinMode(pin, OUTPUT);
	digitalWrite(pin, value ? HIGH : LOW);
	configureDaisyUart(DaisyUartRxPin, DaisyUartTxPin);
	server.send(200, "application/json",
		"{\"pin\":" + String(pin) + ",\"value\":" + String(value ? 1 : 0) + "}");
}

void handleUartLoopback() {
	sendCorsHeaders();
	uint8_t rxPin;
	uint8_t txPin;
	if (!requestedPins(rxPin, txPin)) {
		return;
	}

	configureDaisyUart(rxPin, txPin);
	delay(20);
	clearDaisyInput();
	daisySerial.print("LOOPBACK_TEST\n");
	daisySerial.flush();
	const DaisyReply reply = readDaisyLine(LoopbackTimeoutMs);
	configureDaisyUart(DaisyUartRxPin, DaisyUartTxPin);

	if (reply.complete && reply.body == "LOOPBACK_TEST") {
		server.send(200, "text/plain", "OK uart2 loopback");
	} else {
		server.send(500, "text/plain", "FAIL loopback expected 'LOOPBACK_TEST' but got '" + reply.body + "'");
	}
}

void handleBridgeSelfTest() {
	sendCorsHeaders();
	uint8_t rxPin;
	uint8_t txPin;
	if (!requestedPins(rxPin, txPin)) {
		return;
	}

	configureDaisyUart(rxPin, txPin);
	const DaisyReply reply = transactDaisyCommand("ping");
	configureDaisyUart(DaisyUartRxPin, DaisyUartTxPin);

	if (reply.complete && reply.body.startsWith("PONG")) {
		server.send(200, "text/plain", reply.body);
	} else if (reply.body.isEmpty()) {
		server.send(504, "text/plain", "daisy_timeout");
	} else if (!reply.complete) {
		server.send(504, "text/plain", "daisy_incomplete_response");
	} else {
		server.send(502, "text/plain", "unexpected: " + reply.body);
	}
}

void handleDaisyCommand() {
	sendCorsHeaders();
	if (!server.hasArg("cmd")) {
		server.send(400, "application/json", "{\"error\":\"missing or empty 'cmd' parameter\"}");
		return;
	}

	String command = server.arg("cmd");
	command.trim();
	if (command.isEmpty()) {
		server.send(400, "application/json", "{\"error\":\"missing or empty 'cmd' parameter\"}");
		return;
	}

	const DaisyReply reply = transactDaisyCommand(command);
	if (reply.body.isEmpty()) {
		server.send(504, "application/json", "{\"error\":\"daisy_timeout\"}");
		return;
	}
	if (!reply.complete) {
		server.send(504, "application/json", "{\"error\":\"daisy_incomplete_response\"}");
		return;
	}

	const char* contentType = reply.body.startsWith("{") || reply.body.startsWith("[")
		? "application/json"
		: "text/plain";
	server.send(200, contentType, reply.body);
}

void handleNotFound() {
	sendCorsHeaders();
	server.send(404, "application/json", "{\"error\":\"not_found\"}");
}

void handleOptions() {
	sendCorsHeaders();
	server.send(204);
}

void handleWebSocketEvent(uint8_t client,
	WStype_t type,
	uint8_t* payload,
	size_t length) {
	if (type != WStype_TEXT) return;

	String command;
	command.reserve(length);
	for (size_t i = 0; i < length; ++i) command += static_cast<char>(payload[i]);
	command.trim();

	if (command.isEmpty() || command.length() > DaisyCommandMaxLen
		|| command.indexOf('\n') >= 0 || command.indexOf('\r') >= 0) {
		webSocket.sendTXT(client, "ERR invalid command");
		return;
	}

	const DaisyReply reply = transactDaisyCommand(command);
	if (reply.body.isEmpty()) {
		webSocket.sendTXT(client, "ERR daisy_timeout");
	} else if (!reply.complete) {
		webSocket.sendTXT(client, "ERR daisy_incomplete_response");
	} else {
		String body = reply.body;
		webSocket.sendTXT(client, body);
	}
}

void setupRoutes() {
	server.on("/", HTTP_GET, handleRoot);
	server.on("/", HTTP_OPTIONS, handleOptions);
	server.on("/health", HTTP_GET, handleHealth);
	server.on("/api/bridge/pins", HTTP_GET, handleBridgePins);
	server.on("/api/gpio/read", HTTP_GET, handleGpioRead);
	server.on("/api/gpio/write", HTTP_GET, handleGpioWrite);
	server.on("/api/uart2/loopback", HTTP_GET, handleUartLoopback);
	server.on("/api/bridge/selftest", HTTP_GET, handleBridgeSelfTest);
	server.on("/api/daisy/command", HTTP_GET, handleDaisyCommand);
	server.serveStatic("/app/", LittleFS, "/app/", "max-age=3600");
	server.serveStatic("/assets/", LittleFS, "/assets/", "max-age=3600");
	server.onNotFound(handleNotFound);
}

void connectWifi() {
	WiFi.persistent(false);
	WiFi.disconnect(true, true);
	WiFi.mode(WIFI_STA);
	WiFi.setSleep(false);
	WiFi.setAutoReconnect(true);
	WiFi.begin(CHIMERA_WIFI_SSID, CHIMERA_WIFI_PASSWORD);

	Serial.printf("Connecting to WiFi %s", CHIMERA_WIFI_SSID);
	const uint32_t startMs = millis();
	while (WiFi.status() != WL_CONNECTED && millis() - startMs < WifiConnectTimeoutMs) {
		Serial.print('.');
		delay(250);
	}
	Serial.println();

	if (WiFi.status() == WL_CONNECTED) {
		Serial.printf("WiFi connected ip=%s rssi=%d dBm\n",
			WiFi.localIP().toString().c_str(),
			WiFi.RSSI());
	} else {
		Serial.printf("WiFi connection failed status=%d; reconnecting in background\n",
			static_cast<int>(WiFi.status()));
	}
}
}

void setup() {
	Serial.begin(115200);
	delay(200);
	Serial.println("ChimeraMultiFX bridge boot");
	connectWifi();
	configureDaisyUart(DaisyUartRxPin, DaisyUartTxPin);
	if (!LittleFS.begin(true)) {
		Serial.println("LittleFS mount failed");
	}
	setupRoutes();
	server.begin();
	webSocket.begin();
	webSocket.onEvent(handleWebSocketEvent);
	Serial.println("HTTP server started on port 80; WebSocket on port 81");
}

void loop() {
	webSocket.loop();
	server.handleClient();
	static uint32_t lastStatusMs = 0;
	if (millis() - lastStatusMs >= 5000) {
		lastStatusMs = millis();
		if (WiFi.status() != WL_CONNECTED) {
			WiFi.reconnect();
		}
		Serial.printf("bridge alive wifi_status=%d ip=%s\n",
			static_cast<int>(WiFi.status()),
			WiFi.localIP().toString().c_str());
	}
	delay(1);
}
