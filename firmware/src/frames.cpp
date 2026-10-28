#include "frames.h"
#include <pgmspace.h>
#include <string.h>

// frames are defined here and exported in frames.h

// Legacy diagnostic bitmaps: the 3 least significant bits in each row are ignored.

const uint8_t xBitmap[] PROGMEM = {
  0x80, 0x08,  // #...........#
  0x20, 0x20,  // ..#.......#..
  0x10, 0x40,  // ...#.....#...
  0x05, 0x00,  // .....#.#.....
  0x02, 0x00,  // ......#......
  0x08, 0x80,  // ....#...#....
  0x10, 0x40,  // ...#.....#...
  0x40, 0x10,  // .#.........#.
  0x80, 0x08   // #...........#
};
MonoFrame xFrame = { .bitmap = xBitmap, .colorR = 255, .colorG = 0, .colorB = 0 };

const uint8_t circleBitmap[] PROGMEM = {
  0x07, 0x00,  // .....###.....
  0x10, 0x40,  // ...#.....#...
  0x40, 0x10,  // .#.........#.
  0x80, 0x08,  // #...........#
  0x80, 0x08,  // #...........#
  0x80, 0x08,  // #...........#
  0x40, 0x10,  // .#.........#.
  0x10, 0x40,  // ...#.....#...
  0x07, 0x00   // .....###.....
};
MonoFrame circleFrame = { .bitmap = circleBitmap, .colorR = 0, .colorG = 255, .colorB = 0 };

namespace {
// One byte per pixel. These symbols are RGB332 colors, not palette indexes.
constexpr uint8_t K = 0;
constexpr uint8_t P = rgb332(182, 36, 255);
constexpr uint8_t W = rgb332(255, 255, 255);
constexpr uint8_t B = rgb332(0, 73, 255);
constexpr uint8_t C = rgb332(0, 255, 255);
constexpr uint8_t O = rgb332(255, 146, 0);

const uint8_t eyeCenter[] PROGMEM = {
  K, K, K, K, P, P, P, P, P, K, K, K, K,
  K, K, P, P, W, W, W, W, W, P, P, K, K,
  K, P, W, W, W, W, B, W, W, W, W, P, K,
  P, W, W, W, W, B, B, B, W, W, W, W, P,
  P, W, W, W, W, B, C, B, W, W, W, W, P,
  P, W, W, W, W, B, B, B, W, W, W, W, P,
  K, P, W, W, W, W, B, W, W, W, W, P, K,
  K, K, P, P, W, W, W, W, W, P, P, K, K,
  K, K, K, K, P, P, P, P, P, K, K, K, K,
};
static_assert(sizeof(eyeCenter) == MATRIX_WIDTH * MATRIX_HEIGHT, "Frame dimensions must be 13x9");

const uint8_t eyeLeft[] PROGMEM = {
  K, K, K, K, P, P, P, P, P, K, K, K, K,
  K, K, P, P, W, W, W, W, W, P, P, K, K,
  K, P, W, B, W, W, W, W, W, W, W, P, K,
  P, W, B, B, B, W, W, W, W, W, W, W, P,
  P, W, B, C, B, W, W, W, W, W, W, W, P,
  P, W, B, B, B, W, W, W, W, W, W, W, P,
  K, P, W, B, W, W, W, W, W, W, W, P, K,
  K, K, P, P, W, W, W, W, W, P, P, K, K,
  K, K, K, K, P, P, P, P, P, K, K, K, K,
};
static_assert(sizeof(eyeLeft) == MATRIX_WIDTH * MATRIX_HEIGHT, "Frame dimensions must be 13x9");

const uint8_t eyeRight[] PROGMEM = {
  K, K, K, K, P, P, P, P, P, K, K, K, K,
  K, K, P, P, W, W, W, W, W, P, P, K, K,
  K, P, W, W, W, W, W, W, W, B, W, P, K,
  P, W, W, W, W, W, W, W, B, B, B, W, P,
  P, W, W, W, W, W, W, W, B, C, B, W, P,
  P, W, W, W, W, W, W, W, B, B, B, W, P,
  K, P, W, W, W, W, W, W, W, B, W, P, K,
  K, K, P, P, W, W, W, W, W, P, P, K, K,
  K, K, K, K, P, P, P, P, P, K, K, K, K,
};
static_assert(sizeof(eyeRight) == MATRIX_WIDTH * MATRIX_HEIGHT, "Frame dimensions must be 13x9");

const uint8_t eyeHalf[] PROGMEM = {
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, P, P, P, P, P, P, P, P, P, K, K,
  K, P, W, W, W, W, B, W, W, W, W, P, K,
  P, W, W, W, W, B, C, B, W, W, W, W, P,
  K, P, W, W, W, W, B, W, W, W, W, P, K,
  K, K, P, P, P, P, P, P, P, P, P, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
};
static_assert(sizeof(eyeHalf) == MATRIX_WIDTH * MATRIX_HEIGHT, "Frame dimensions must be 13x9");

const uint8_t eyeClosed[] PROGMEM = {
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, C, C, P, P, P, P, P, P, P, C, C, K,
  K, K, P, P, P, P, P, P, P, P, P, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K,
};
static_assert(sizeof(eyeClosed) == MATRIX_WIDTH * MATRIX_HEIGHT, "Frame dimensions must be 13x9");

// One continuous 26x9 mouth. Matrix 3 reads columns 0–12, matrix 4 reads 13–25.
const uint8_t mouthSmile[] PROGMEM = {
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, O, O, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, O, O, K, K,
  K, K, K, K, O, O, W, W, W, W, W, W, W, W, W, W, W, W, W, W, O, O, K, K, K, K,
  K, K, K, K, K, K, O, O, O, O, O, O, O, O, O, O, O, O, O, O, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
};
static_assert(sizeof(mouthSmile) == MOUTH_WIDTH * MATRIX_HEIGHT, "Mouth frame dimensions must be 26x9");

const uint8_t mouthOpen[] PROGMEM = {
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
  K, K, K, K, K, K, O, O, O, O, O, O, O, O, O, O, O, O, O, O, K, K, K, K, K, K,
  K, K, K, K, O, O, W, W, W, W, W, W, W, W, W, W, W, W, W, W, O, O, K, K, K, K,
  K, K, O, O, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, O, O, K, K,
  K, K, O, O, K, K, K, K, P, P, P, P, P, P, P, P, P, P, K, K, K, K, O, O, K, K,
  K, K, K, K, O, O, P, P, P, P, P, P, P, P, P, P, P, P, P, P, O, O, K, K, K, K,
  K, K, K, K, K, K, O, O, O, O, O, O, O, O, O, O, O, O, O, O, K, K, K, K, K, K,
  K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K, K,
};
static_assert(sizeof(mouthOpen) == MOUTH_WIDTH * MATRIX_HEIGHT, "Mouth frame dimensions must be 26x9");

// Each entry sets its own hold time. Pixel arrays can be reused in sequences.
const ColorFrame blinkFrames[] PROGMEM = {
  {eyeCenter, 2400}, {eyeHalf, 100}, {eyeClosed, 180}, {eyeHalf, 100}
};
const ColorFrame lookFrames[] PROGMEM = {
  {eyeLeft, 900}, {eyeCenter, 400}, {eyeRight, 900}, {eyeCenter, 400}
};
const ColorFrame talkFrames[] PROGMEM = {
  {mouthSmile, 300}, {mouthOpen, 450}, {mouthSmile, 200}, {mouthOpen, 250}
};
const ColorFrame smileFrames[] PROGMEM = {{mouthSmile, 1000}};
const ColorFrame offFrames[] PROGMEM = {{nullptr, 1000}};

const char blinkName[] PROGMEM = "blink";
const char lookName[] PROGMEM = "look";
const char talkName[] PROGMEM = "talk";
const char smileName[] PROGMEM = "smile";
const char offName[] PROGMEM = "off";

#include "custom_animations.h"

const MatrixAnimation animations[] PROGMEM = {
  {blinkName, blinkFrames, sizeof(blinkFrames) / sizeof(ColorFrame), MATRIX_WIDTH},
  {lookName, lookFrames, sizeof(lookFrames) / sizeof(ColorFrame), MATRIX_WIDTH},
  {talkName, talkFrames, sizeof(talkFrames) / sizeof(ColorFrame), MOUTH_WIDTH},
  {smileName, smileFrames, sizeof(smileFrames) / sizeof(ColorFrame), MOUTH_WIDTH},
  {offName, offFrames, sizeof(offFrames) / sizeof(ColorFrame), 0},
  CUSTOM_MATRIX_ANIMATIONS
};
} // namespace

const MatrixAnimation *findMatrixAnimation(const char *name) {
  if (!name) return nullptr;
  for (size_t i = 0; i < sizeof(animations) / sizeof(MatrixAnimation); ++i) {
    MatrixAnimation animation;
    memcpy_P(&animation, &animations[i], sizeof(animation));
    if (strcmp_P(name, animation.name) == 0) return &animations[i];
  }
  return nullptr;
}
