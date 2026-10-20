#ifndef FRAMES_H
#define FRAMES_H
#include <stdint.h>

struct MonoFrame {
    const uint8_t *bitmap;
    uint8_t colorR;
    uint8_t colorG;
    uint8_t colorB;
};

extern MonoFrame xFrame;
extern MonoFrame circleFrame;

#endif