#include "debugClientBridge.h"
#include "debugClient.h"

void ibDebuggerClientBridge::SetDebuggerClientBridge(ibDebuggerClientBridge* bridge) {
	if (debugClient != nullptr) {
		debugClient->SetBridge(bridge);
	}
}
