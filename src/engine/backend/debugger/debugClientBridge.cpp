#include "debugClientBridge.h"
#include "debugClient.h"

void IDebuggerClientBridge::SetDebuggerClientBridge(IDebuggerClientBridge* bridge) {
	if (debugClient != nullptr) {
		debugClient->SetBridge(bridge);
	}
}
