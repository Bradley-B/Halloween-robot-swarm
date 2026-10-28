#ifndef FRAMES_H
#define FRAMES_H
#include <stdint.h>

constexpr uint8_t MATRIX_WIDTH = 13;
constexpr uint8_t MATRIX_HEIGHT = 9;
constexpr uint8_t MATRIX_COUNT = 4;
constexpr uint8_t MOUTH_WIDTH = MATRIX_WIDTH * 2;

// RGB332: three red bits, three green bits, two blue bits per pixel.
constexpr uint8_t rgb332(uint8_t red, uint8_t green, uint8_t blue) {
    return (red & 0xe0) | ((green & 0xe0) >> 3) | (blue >> 6);
}

// Pixel arrays, frame sequences, names, and animation descriptors live in PROGMEM.
// A null pixels pointer represents an entirely black frame.
struct ColorFrame {
    const uint8_t *pixels;
    uint16_t durationMs; // 1–65535; a one-frame animation holds indefinitely.
};

struct MatrixAnimation {
    const char *name;
    const ColorFrame *frames;
    uint8_t frameCount;
    uint8_t width; // 13 for eyes, 26 for the shared mouth, 0 for universal off.
};

// Returns a flash address. Read descriptors with memcpy_P, not by dereferencing.
const MatrixAnimation *findMatrixAnimation(const char *name);

struct MonoFrame {
    const uint8_t *bitmap;
    uint8_t colorR;
    uint8_t colorG;
    uint8_t colorB;
};

extern MonoFrame xFrame;
extern MonoFrame circleFrame;

#endif
