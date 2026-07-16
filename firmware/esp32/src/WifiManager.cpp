#include "WifiManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include "wifi_credentials.h"

namespace {
constexpr uint32_t WifiConnectTimeoutMs = 20000;
constexpr uint32_t WifiReconnectIntervalMs = 5000;
constexpr uint32_t WifiRejoinIntervalMs = 15000;
uint32_t lastStatusMs = 0;
uint32_t disconnectedSinceMs = 0;

void startStation() {
	WiFi.persistent(false);
	WiFi.disconnect(true, true);
	WiFi.mode(WIFI_STA);
	WiFi.setSleep(false);
	WiFi.setAutoReconnect(true);
	WiFi.begin(CHIMERA_WIFI_SSID, CHIMERA_WIFI_PASSWORD);
}
}

namespace WifiManager {
void connect() {
	startStation();

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

void maintain() {
	if (millis() - lastStatusMs < WifiReconnectIntervalMs) return;
	lastStatusMs = millis();
	if (WiFi.status() == WL_CONNECTED) {
		disconnectedSinceMs = 0;
	} else if (disconnectedSinceMs == 0) {
		disconnectedSinceMs = millis();
		Serial.println("WiFi disconnected; requesting reconnect");
		WiFi.reconnect();
	} else if (millis() - disconnectedSinceMs >= WifiRejoinIntervalMs) {
		disconnectedSinceMs = millis();
		Serial.println("WiFi still disconnected; cycling station radio");
		startStation();
	} else {
		WiFi.reconnect();
	}
	Serial.printf("bridge alive wifi_status=%d ip=%s\n",
		static_cast<int>(WiFi.status()),
		WiFi.localIP().toString().c_str());
}
}