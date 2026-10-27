#pragma once

void ledSetup();
void ledLoop();

// Apply a mode and #RRGGBB color together. Invalid input leaves state unchanged.
bool ledSetState(const char *mode, const char *color);
