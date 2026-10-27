#pragma once

#include <stdint.h>

constexpr uint32_t STRIP_DEFAULT_FRAME_INTERVAL_MS = 10;
constexpr uint32_t STRIP_DEFAULT_PULSE_PERIOD_MS = 2000;

void ledSetup();
void ledLoop();

// Apply settings together. Invalid input leaves state unchanged.
// Frame interval: 1–60000 ms. Pulse period: 2–60000 ms.
bool ledSetState(const char *mode, const char *color,
                 uint32_t frameIntervalMs = STRIP_DEFAULT_FRAME_INTERVAL_MS,
                 uint32_t pulsePeriodMs = STRIP_DEFAULT_PULSE_PERIOD_MS);
