#pragma once

// Registered in the pinned Meshtastic module registry by prepare.py.
void setupSentinelPresence();

#include <cstddef>
#include <cstdint>
bool sentinelLocalObservation(uint32_t to,uint32_t port,const uint8_t *bytes,size_t length);
