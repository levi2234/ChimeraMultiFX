#pragma once

#include <WebServer.h>
#include <WebSocketsServer.h>

namespace BridgeServer {
void setupRoutes(WebServer& server, WebSocketsServer& webSocket);
}