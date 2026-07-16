#include "BridgeServer.h"

#include <LittleFS.h>
#include "DaisyBridge.h"

namespace {
WebServer* server = nullptr;
WebSocketsServer* webSocket = nullptr;

void sendCorsHeaders() {
	server->sendHeader("Access-Control-Allow-Origin", "*");
	server->sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
	server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

bool requestedPins(uint8_t& rxPin, uint8_t& txPin) {
	rxPin = DaisyBridge::DefaultRxPin;
	txPin = DaisyBridge::DefaultTxPin;

	if (server->hasArg("rx")) {
		const int value = server->arg("rx").toInt();
		if (value < 0 || value > 39) {
			server->send(400, "application/json", "{\"error\":\"invalid_rx_pin\"}");
			return false;
		}
		rxPin = static_cast<uint8_t>(value);
	}
	if (server->hasArg("tx")) {
		const int value = server->arg("tx").toInt();
		if (value < 0 || value > 39) {
			server->send(400, "application/json", "{\"error\":\"invalid_tx_pin\"}");
			return false;
		}
		txPin = static_cast<uint8_t>(value);
	}
	return true;
}

bool requestedGpio(uint8_t& pin) {
	if (!server->hasArg("pin")) {
		server->send(400, "application/json", "{\"error\":\"missing_pin\"}");
		return false;
	}
	const int value = server->arg("pin").toInt();
	if (value < 0 || value > 39) {
		server->send(400, "application/json", "{\"error\":\"invalid_pin\"}");
		return false;
	}
	pin = static_cast<uint8_t>(value);
	return true;
}

void handleRoot() {
	sendCorsHeaders();
	File index = LittleFS.open("/index.html", "r");
	if (!index) {
		server->send(503, "text/plain", "UI not installed. Run: pio run --target uploadfs\n");
		return;
	}
	server->streamFile(index, "text/html");
	index.close();
}

void handleHealth() {
	sendCorsHeaders();
	const DaisyBridge::Stats stats = DaisyBridge::stats();
	const String response = "{\"ok\":true,\"uart_started\":"
		+ String(stats.uartStarted ? "true" : "false")
		+ ",\"uart_backoff_active\":" + String(stats.uartBackoffActive ? "true" : "false")
		+ ",\"uart_backoff_remaining_ms\":" + String(stats.uartBackoffRemainingMs)
		+ ",\"commands\":" + String(stats.commandCount)
		+ ",\"tx_bytes\":" + String(stats.txByteCount)
		+ ",\"replies\":" + String(stats.replyCount)
		+ ",\"timeouts\":" + String(stats.timeoutCount)
		+ ",\"recoveries\":" + String(stats.recoveryCount) + "}";
	server->send(200, "application/json", response);
}

void handleBridgePins() {
	sendCorsHeaders();
	const String response = "{\"uart\":2,\"rx_pin\":" + String(DaisyBridge::DefaultRxPin)
		+ ",\"tx_pin\":" + String(DaisyBridge::DefaultTxPin)
		+ ",\"baud\":" + String(DaisyBridge::Baud) + "}";
	server->send(200, "application/json", response);
}

void handleGpioRead() {
	sendCorsHeaders();
	uint8_t pin;
	if (!requestedGpio(pin)) return;

	DaisyBridge::stopUart();
	pinMode(pin, INPUT_PULLDOWN);
	delay(1);
	const int value = digitalRead(pin);
	DaisyBridge::configureUart(DaisyBridge::DefaultRxPin, DaisyBridge::DefaultTxPin);
	server->send(200, "application/json",
		"{\"pin\":" + String(pin) + ",\"value\":" + String(value) + "}");
}

void handleGpioWrite() {
	sendCorsHeaders();
	uint8_t pin;
	if (!requestedGpio(pin)) return;
	if (!server->hasArg("value")) {
		server->send(400, "application/json", "{\"error\":\"missing_value\"}");
		return;
	}

	const bool value = server->arg("value").toInt() != 0;
	DaisyBridge::stopUart();
	pinMode(pin, OUTPUT);
	digitalWrite(pin, value ? HIGH : LOW);
	DaisyBridge::configureUart(DaisyBridge::DefaultRxPin, DaisyBridge::DefaultTxPin);
	server->send(200, "application/json",
		"{\"pin\":" + String(pin) + ",\"value\":" + String(value ? 1 : 0) + "}");
}

void handleUartLoopback() {
	sendCorsHeaders();
	uint8_t rxPin;
	uint8_t txPin;
	if (!requestedPins(rxPin, txPin)) {
		return;
	}

	const DaisyBridge::Reply reply = DaisyBridge::testLoopback(rxPin, txPin);

	if (reply.complete && reply.body == "LOOPBACK_TEST") {
		server->send(200, "text/plain", "OK uart2 loopback");
	} else {
		server->send(500, "text/plain", "FAIL loopback expected 'LOOPBACK_TEST' but got '" + reply.body + "'");
	}
}

void handleBridgeSelfTest() {
	sendCorsHeaders();
	uint8_t rxPin;
	uint8_t txPin;
	if (!requestedPins(rxPin, txPin)) {
		return;
	}

	DaisyBridge::configureUart(rxPin, txPin);
	const DaisyBridge::Reply reply = DaisyBridge::transactCommand("ping");
	DaisyBridge::configureUart(DaisyBridge::DefaultRxPin, DaisyBridge::DefaultTxPin);

	if (reply.complete && reply.body.startsWith("PONG")) {
		server->send(200, "text/plain", reply.body);
	} else if (reply.body.isEmpty()) {
		server->send(504, "text/plain", "daisy_timeout");
	} else if (!reply.complete) {
		server->send(504, "text/plain", "daisy_incomplete_response");
	} else {
		server->send(502, "text/plain", "unexpected: " + reply.body);
	}
}

void handleDaisyReset() {
	sendCorsHeaders();
	DaisyBridge::sendReset();
	server->send(202, "text/plain", "daisy reset requested");
}

void handleDaisyCommand() {
	sendCorsHeaders();
	if (!server->hasArg("cmd")) {
		server->send(400, "application/json", "{\"error\":\"missing or empty 'cmd' parameter\"}");
		return;
	}

	String command = server->arg("cmd");
	command.trim();
	if (command.isEmpty()) {
		server->send(400, "application/json", "{\"error\":\"missing or empty 'cmd' parameter\"}");
		return;
	}

	const DaisyBridge::Reply reply = DaisyBridge::transactCommand(command);
	if (reply.body.isEmpty()) {
		server->send(504, "application/json", "{\"error\":\"daisy_timeout\"}");
		return;
	}
	if (!reply.complete) {
		server->send(504, "application/json", "{\"error\":\"daisy_incomplete_response\"}");
		return;
	}

	const char* contentType = reply.body.startsWith("{") || reply.body.startsWith("[")
		? "application/json"
		: "text/plain";
	server->send(200, contentType, reply.body);
}

void handleNotFound() {
	sendCorsHeaders();
	server->send(404, "application/json", "{\"error\":\"not_found\"}");
}

void handleOptions() {
	sendCorsHeaders();
	server->send(204);
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

	if (command.isEmpty() || command.length() > DaisyBridge::CommandMaxLen
		|| command.indexOf('\n') >= 0 || command.indexOf('\r') >= 0) {
		webSocket->sendTXT(client, "ERR invalid command");
		return;
	}

	const DaisyBridge::Reply reply = DaisyBridge::transactCommand(command);
	if (reply.body.isEmpty()) {
		webSocket->sendTXT(client, "ERR daisy_timeout");
	} else if (!reply.complete) {
		webSocket->sendTXT(client, "ERR daisy_incomplete_response");
	} else if (reply.body.length() > DaisyBridge::WebSocketReplyMaxLen) {
		webSocket->sendTXT(client, "ERR response_too_large_use_http");
	} else {
		String body = reply.body;
		webSocket->sendTXT(client, body);
	}
}
}

namespace BridgeServer {
void setupRoutes(WebServer& routeServer, WebSocketsServer& socketServer) {
	server = &routeServer;
	webSocket = &socketServer;

	server->on("/", HTTP_GET, handleRoot);
	server->on("/", HTTP_OPTIONS, handleOptions);
	server->on("/health", HTTP_GET, handleHealth);
	server->on("/api/bridge/pins", HTTP_GET, handleBridgePins);
	server->on("/api/gpio/read", HTTP_GET, handleGpioRead);
	server->on("/api/gpio/write", HTTP_GET, handleGpioWrite);
	server->on("/api/uart2/loopback", HTTP_GET, handleUartLoopback);
	server->on("/api/bridge/selftest", HTTP_GET, handleBridgeSelfTest);
	server->on("/api/bridge/reset", HTTP_POST, handleDaisyReset);
	server->on("/api/bridge/reset", HTTP_GET, handleDaisyReset);
	server->on("/api/daisy/command", HTTP_GET, handleDaisyCommand);
	server->serveStatic("/app/", LittleFS, "/app/", "max-age=3600");
	server->serveStatic("/assets/", LittleFS, "/assets/", "max-age=3600");
	server->onNotFound(handleNotFound);
	webSocket->onEvent(handleWebSocketEvent);
}
}