#include <Adafruit_IS31FL3741.h>
#include <pgmspace.h>

#include "matrix.h"

namespace {
// Retain the driver's existing buffers; no additional framebuffer is needed.
Adafruit_IS31FL3741_QT_buffered matrices[MATRIX_COUNT];
TwoWire *i2c = &Wire;

struct MatrixPlayback {
  const MatrixAnimation *animation; // Flash address, shared by all players.
  uint32_t frameStartedAt;
  uint8_t frameIndex;
  bool dirty;
};

MatrixPlayback playback[MATRIX_COUNT] = {};

ColorFrame readFrame(const MatrixAnimation &animation, uint8_t index) {
  ColorFrame frame;
  memcpy_P(&frame, animation.frames + index, sizeof(frame));
  return frame;
}

void showColorFrame(Adafruit_IS31FL3741_QT_buffered &matrix, const ColorFrame &frame) {
  if (!frame.pixels) {
    matrix.fillScreen(0);
  } else {
    for (uint8_t y = 0; y < MATRIX_HEIGHT; ++y) {
      for (uint8_t x = 0; x < MATRIX_WIDTH; ++x) {
        const uint8_t pixel = pgm_read_byte(frame.pixels + y * MATRIX_WIDTH + x);
        const uint8_t red = (pixel >> 5) * 255 / 7;
        const uint8_t green = ((pixel >> 2) & 0x07) * 255 / 7;
        const uint8_t blue = (pixel & 0x03) * 255 / 3;
        matrix.drawPixel(x, y, matrix.color565(red, green, blue));
      }
    }
  }
  matrix.show();
}
} // namespace

void matrixSetup() {
  Serial.begin(9600);
  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    const uint8_t address = IS3741_ADDR_DEFAULT + i;
    if (!matrices[i].begin(address, i2c)) {
      Serial.print("IS41 not found at 0x");
      Serial.println(address, HEX);
      while (1) yield();
    }
    matrices[i].setLEDscaling(0xFF);
    matrices[i].setGlobalCurrent(0x10);
    matrices[i].enable(true);
  }
  i2c->setClock(800000);
}

bool matrixSetAnimation(uint8_t matrixNumber, const char *animationName) {
  if (matrixNumber > MATRIX_COUNT) return false;
  const MatrixAnimation *animation = findMatrixAnimation(animationName);
  if (!animation) return false;

  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    if (matrixNumber != 0 && matrixNumber != i + 1) continue;
    playback[i].animation = animation;
    playback[i].frameIndex = 0;
    playback[i].dirty = true;
  }
  return true;
}

void matrixLoop() {
  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    MatrixPlayback &state = playback[i];
    if (!state.animation) continue;

    MatrixAnimation animation;
    memcpy_P(&animation, state.animation, sizeof(animation));
    ColorFrame frame = readFrame(animation, state.frameIndex);
    if (!state.dirty) {
      if (animation.frameCount == 1 || millis() - state.frameStartedAt < frame.durationMs) continue;
      state.frameIndex = (state.frameIndex + 1) % animation.frameCount;
      frame = readFrame(animation, state.frameIndex);
    }

    // Advance at most one frame per pass; no delay or catch-up burst after a stall.
    showColorFrame(matrices[i], frame);
    state.frameStartedAt = millis();
    state.dirty = false;
  }
}

void setMonoFrameAll(MonoFrame *frame) {
  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    matrices[i].fillScreen(0);
    matrices[i].drawBitmap(0, 0, frame->bitmap, MATRIX_WIDTH, MATRIX_HEIGHT,
                          matrices[i].color565(frame->colorR, frame->colorG, frame->colorB));
    matrices[i].show();
    // Connectivity diagnostics temporarily cover, rather than erase, selections.
    if (playback[i].animation) playback[i].dirty = true;
  }
}

void writeLetterAll(const char *letter) {
  if (!letter) return;
  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    playback[i].animation = nullptr;
    matrices[i].fillScreen(0);
    matrices[i].setCursor(0, 0);
    matrices[i].setTextColor(matrices[i].color565(0, 0, 255));
    matrices[i].print(letter);
    matrices[i].show();
  }
}
