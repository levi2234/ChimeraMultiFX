#pragma once

#include <Arduino.h>

namespace DaisyBridge {
static constexpr uint8_t DefaultRxPin = 16;
static constexpr uint8_t DefaultTxPin = 17;
static constexpr uint32_t Baud = 115200;
static constexpr uint32_t FirstByteTimeoutMs = 250;
static constexpr uint32_t ResponseIdleTimeoutMs = 80;
static constexpr uint32_t ResponseMaxDurationMs = 2500;
static constexpr uint32_t LargeResponseFirstByteTimeoutMs = 2000;
static constexpr uint32_t LargeResponseIdleTimeoutMs = 750;
static constexpr uint32_t LargeResponseMaxDurationMs = 8000;
static constexpr uint32_t RetryDelayMs = 2;
static constexpr uint32_t LoopbackTimeoutMs = 250;
static constexpr size_t ResponseMaxLen = 32768;
static constexpr size_t CommandMaxLen = 127;
static constexpr size_t WebSocketReplyMaxLen = 1024;

struct Reply {
	String body;
	bool complete;
	bool overflow;
};

struct Stats {
	bool uartStarted;
	bool uartBackoffActive;
	uint32_t uartBackoffRemainingMs;
	uint32_t commandCount;
	uint32_t txByteCount;
	uint32_t replyCount;
	uint32_t timeoutCount;
	uint32_t recoveryCount;
};

void begin(uint8_t rxPin = DefaultRxPin, uint8_t txPin = DefaultTxPin);
void configureUart(uint8_t rxPin, uint8_t txPin);
void stopUart();
void clearInput();
void sendReset();
Reply readLine(uint32_t firstByteTimeoutMs = FirstByteTimeoutMs,
	uint32_t idleTimeoutMs = ResponseIdleTimeoutMs,
	uint32_t maxDurationMs = ResponseMaxDurationMs);
Reply testLoopback(uint8_t rxPin, uint8_t txPin);
Reply transactCommand(const String& command);
Stats stats();
}