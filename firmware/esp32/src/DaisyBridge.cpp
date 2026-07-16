#include "DaisyBridge.h"

namespace {
constexpr uint32_t DaisyUartSettleMs = 10;
constexpr uint32_t DaisyUnavailableBackoffMs = 1500;

HardwareSerial daisySerial(2);
bool daisyUartStarted = false;
bool daisyRxBufferConfigured = false;
uint8_t daisyUartRxPin = DaisyBridge::DefaultRxPin;
uint8_t daisyUartTxPin = DaisyBridge::DefaultTxPin;
uint32_t daisyCommandCount = 0;
uint32_t daisyTxByteCount = 0;
uint32_t daisyReplyCount = 0;
uint32_t daisyTimeoutCount = 0;
uint32_t daisyRecoveryCount = 0;
uint32_t daisyUnavailableUntilMs = 0;

bool timeIsBefore(uint32_t lhs, uint32_t rhs) {
	return static_cast<int32_t>(lhs - rhs) < 0;
}

uint32_t unavailableBackoffRemainingMs() {
	const uint32_t nowMs = millis();
	if (daisyUnavailableUntilMs == 0 || !timeIsBefore(nowMs, daisyUnavailableUntilMs)) return 0;
	return daisyUnavailableUntilMs - nowMs;
}

void recoverDaisyUart() {
	DaisyBridge::clearInput();
	daisySerial.setPins(DaisyBridge::DefaultRxPin, DaisyBridge::DefaultTxPin);
	daisySerial.updateBaudRate(DaisyBridge::Baud);
	daisySerial.write('\n');
	daisySerial.flush();
	delay(DaisyBridge::RetryDelayMs);
	DaisyBridge::clearInput();
	daisyRecoveryCount++;
}

bool commandUsesLargeResponseWindow(const String& command) {
	return command.startsWith("status") || command == "info" || command.startsWith("effect ");
}

bool commandCanRetry(const String& command) {
	return command.startsWith("status")
		|| command == "info"
		|| command.startsWith("effect ")
		|| command == "cpu_usage"
		|| command == "ping"
		|| command == "uartdiag";
}

DaisyBridge::Reply sendDaisyCommand(const String& command) {
	DaisyBridge::clearInput();
	daisyCommandCount++;
	daisyTxByteCount += daisySerial.write(
		reinterpret_cast<const uint8_t*>(command.c_str()), command.length());
	daisyTxByteCount += daisySerial.write('\n');
	daisySerial.flush();
	DaisyBridge::Reply reply = commandUsesLargeResponseWindow(command)
		? DaisyBridge::readLine(
			DaisyBridge::LargeResponseFirstByteTimeoutMs,
			DaisyBridge::LargeResponseIdleTimeoutMs,
			DaisyBridge::LargeResponseMaxDurationMs)
		: DaisyBridge::readLine();
	if (reply.complete) {
		daisyUnavailableUntilMs = 0;
		daisyReplyCount++;
	} else {
		daisyUnavailableUntilMs = millis() + DaisyUnavailableBackoffMs;
		daisyTimeoutCount++;
		Serial.printf("Daisy timeout command=%s tx_bytes=%u recoveries=%u\n",
			command.c_str(),
			static_cast<unsigned>(daisyTxByteCount),
			static_cast<unsigned>(daisyRecoveryCount));
	}
	return reply;
}
}

namespace DaisyBridge {
void begin(uint8_t rxPin, uint8_t txPin) {
	configureUart(rxPin, txPin);
}

void configureUart(uint8_t rxPin, uint8_t txPin) {
	if (daisyUartStarted && rxPin == daisyUartRxPin && txPin == daisyUartTxPin) return;
	if (daisyUartStarted) {
		daisySerial.end();
		daisyUartStarted = false;
		delay(DaisyUartSettleMs);
	}
	if (!daisyRxBufferConfigured) {
		daisySerial.setRxBufferSize(ResponseMaxLen);
		daisyRxBufferConfigured = true;
	}
	daisySerial.begin(Baud, SERIAL_8N1, rxPin, txPin);
	daisyUartRxPin = rxPin;
	daisyUartTxPin = txPin;
	daisyUartStarted = true;
	delay(DaisyUartSettleMs);
}

void stopUart() {
	if (!daisyUartStarted) return;
	daisySerial.end();
	daisyUartStarted = false;
}

void clearInput() {
	while (daisySerial.available() > 0) {
		daisySerial.read();
	}
}

void sendReset() {
	clearInput();
	daisySerial.print("reset\n");
	daisySerial.flush();
}

Reply readLine(uint32_t firstByteTimeoutMs, uint32_t idleTimeoutMs, uint32_t maxDurationMs) {
	String response;
	response.reserve(256);
	const uint32_t startMs = millis();
	uint32_t lastByteMs = startMs;
	bool receivedByte = false;

	while ((millis() - startMs) < maxDurationMs) {
		while (daisySerial.available() > 0) {
			const char character = static_cast<char>(daisySerial.read());
			receivedByte = true;
			lastByteMs = millis();
			if (character == '\n') {
				return {response, true, false};
			}
			if (character != '\r' && response.length() < ResponseMaxLen - 1) {
				response += character;
			} else if (character != '\r') {
				return {response, false, true};
			}
		}
		const uint32_t nowMs = millis();
		if (!receivedByte && (nowMs - startMs) >= firstByteTimeoutMs) break;
		if (receivedByte && (nowMs - lastByteMs) >= idleTimeoutMs) break;
		delay(1);
	}

	return {response, false, response.length() >= ResponseMaxLen - 1};
}

Reply testLoopback(uint8_t rxPin, uint8_t txPin) {
	configureUart(rxPin, txPin);
	delay(20);
	clearInput();
	daisySerial.print("LOOPBACK_TEST\n");
	daisySerial.flush();
	Reply reply = readLine(LoopbackTimeoutMs, ResponseIdleTimeoutMs, LoopbackTimeoutMs);
	configureUart(DefaultRxPin, DefaultTxPin);
	return reply;
}

Reply transactCommand(const String& command) {
	if (unavailableBackoffRemainingMs() > 0) return {"", false, false};

	Reply reply = sendDaisyCommand(command);
	if (!reply.complete) {
		delay(RetryDelayMs);
		recoverDaisyUart();
		if (!reply.overflow && commandCanRetry(command)) {
			daisyUnavailableUntilMs = 0;
			reply = sendDaisyCommand(command);
			if (!reply.complete) {
				delay(RetryDelayMs);
				recoverDaisyUart();
			}
		}
	}
	return reply;
}

Stats stats() {
	const uint32_t backoffRemainingMs = unavailableBackoffRemainingMs();
	return {
		daisyUartStarted,
		backoffRemainingMs > 0,
		backoffRemainingMs,
		daisyCommandCount,
		daisyTxByteCount,
		daisyReplyCount,
		daisyTimeoutCount,
		daisyRecoveryCount,
	};
}
}