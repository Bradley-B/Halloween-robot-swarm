#pragma once

#include "frames.h"

void matrixSetup();
void matrixLoop();

// Left/right are from the perspective of looking at the face.
enum class MatrixTarget : uint8_t {
    All = 0, LeftEye = 1, RightEye = 2, LeftMouth = 3, RightMouth = 4,
    Eyes = 5, Mouth = 6
};

// Group selections share a clock. Invalid or incompatible selections change nothing.
bool matrixSetAnimation(MatrixTarget target, const char *animationName);

// Temporary diagnostic display; selected animations resume on matrixLoop().
void setMonoFrameAll(MonoFrame *frame);
// Legacy letter display stops all selected animations until another command.
void writeLetterAll(const char *letter);
