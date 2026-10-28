#pragma once

#include "frames.h"

void matrixSetup();
void matrixLoop();

// Matrix numbers 1–4 map to I2C addresses 0x30–0x33; 0 selects all matrices.
// Unknown animations or matrix numbers leave every selection unchanged.
bool matrixSetAnimation(uint8_t matrixNumber, const char *animationName);

// Temporary diagnostic display; selected animations resume on matrixLoop().
void setMonoFrameAll(MonoFrame *frame);
// Legacy letter display stops all selected animations until another command.
void writeLetterAll(const char *letter);
