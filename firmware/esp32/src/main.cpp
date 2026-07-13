#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
constexpr uint8_t ApChannel = 6;
constexpr uint8_t ApMaxClients = 4;
constexpr uint8_t DaisyUartRxPin = 16;
constexpr uint8_t DaisyUartTxPin = 17;
constexpr uint32_t DaisyUartBaud = 115200;
constexpr uint32_t DaisyResponseTimeoutMs = 2000;
constexpr uint32_t LoopbackTimeoutMs = 250;
constexpr size_t DaisyResponseMaxLen = 4096;

const char* ApSsid = "ChimeraMultiFX";
const char* ApPassword = "chimerafx";
IPAddress ApIp(192, 168, 4, 1);
IPAddress ApGateway(192, 168, 4, 1);
IPAddress ApSubnet(255, 255, 255, 0);

WebServer server(80);
HardwareSerial daisySerial(2);

void sendCorsHeaders() {
	server.sendHeader("Access-Control-Allow-Origin", "*");
	server.sendHeader("Access-Control-Allow-Methods", "GET");
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

String readDaisyLine(uint32_t timeoutMs) {
	String response;
	response.reserve(256);
	const uint32_t startMs = millis();

	while ((millis() - startMs) < timeoutMs) {
		while (daisySerial.available() > 0) {
			const char character = static_cast<char>(daisySerial.read());
			if (character == '\n') {
				return response;
			}
			if (character != '\r' && response.length() < DaisyResponseMaxLen - 1) {
				response += character;
			}
		}
		delay(1);
	}

	return response;
}

String transactDaisyCommand(const String& command) {
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

void handleRoot() {
	sendCorsHeaders();
	server.send(200, "text/plain", "ChimeraMultiFX ESP32 control surface\n");
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
	const String response = readDaisyLine(LoopbackTimeoutMs);
	configureDaisyUart(DaisyUartRxPin, DaisyUartTxPin);

	if (response == "LOOPBACK_TEST") {
		server.send(200, "text/plain", "OK uart2 loopback");
	} else {
		server.send(500, "text/plain", "FAIL loopback expected 'LOOPBACK_TEST' but got '" + response + "'");
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
	const String response = transactDaisyCommand("ping");
	configureDaisyUart(DaisyUartRxPin, DaisyUartTxPin);

	if (response.startsWith("PONG")) {
		server.send(200, "text/plain", response);
	} else if (response.isEmpty()) {
		server.send(504, "text/plain", "daisy_timeout");
	} else {
		server.send(502, "text/plain", "unexpected: " + response);
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

	const String response = transactDaisyCommand(command);
	if (response.isEmpty()) {
		server.send(504, "application/json", "{\"error\":\"daisy_timeout\"}");
		return;
	}

	const char* contentType = response.startsWith("{") || response.startsWith("[")
		? "application/json"
		: "text/plain";
	server.send(200, contentType, response);
}

void handleNotFound() {
	sendCorsHeaders();
	server.send(404, "application/json", "{\"error\":\"not_found\"}");
}

void setupRoutes() {
	server.on("/", HTTP_GET, handleRoot);
	server.on("/health", HTTP_GET, handleHealth);
	server.on("/api/bridge/pins", HTTP_GET, handleBridgePins);
	server.on("/api/uart2/loopback", HTTP_GET, handleUartLoopback);
	server.on("/api/bridge/selftest", HTTP_GET, handleBridgeSelfTest);
	server.on("/api/daisy/command", HTTP_GET, handleDaisyCommand);
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
}

void setup() {
	setupAccessPoint();
	configureDaisyUart(DaisyUartRxPin, DaisyUartTxPin);
	setupRoutes();
	server.begin();
}

void loop() {
	server.handleClient();
	delay(1);
}
