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

bool includesMatrix(MatrixTarget target, uint8_t index) {
  if (target == MatrixTarget::All) return true;
  if (target == MatrixTarget::Eyes) return index < 2;
  if (target == MatrixTarget::Mouth) return index >= 2;
  return static_cast<uint8_t>(target) == index + 1;
}

bool compatibleWithMatrix(const MatrixAnimation &animation, uint8_t index) {
  return animation.width == 0 ||
         animation.width == (index < 2 ? MATRIX_WIDTH : MOUTH_WIDTH);
}

void showColorFrame(uint8_t index, const MatrixAnimation &animation, const ColorFrame &frame) {
  Adafruit_IS31FL3741_QT_buffered &matrix = matrices[index];
  if (!frame.pixels) {
    matrix.fillScreen(0);
  } else {
    const uint8_t offset = index == 3 ? MATRIX_WIDTH : 0;
    for (uint8_t y = 0; y < MATRIX_HEIGHT; ++y) {
      for (uint8_t x = 0; x < MATRIX_WIDTH; ++x) {
        const uint8_t pixel = pgm_read_byte(frame.pixels + y * animation.width + offset + x);
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
    matrices[i].setGlobalCurrent(0x05);
    matrices[i].enable(true);
  }
  i2c->setClock(800000);
}

bool matrixSetAnimation(MatrixTarget target, const char *animationName) {
  if (static_cast<uint8_t>(target) > static_cast<uint8_t>(MatrixTarget::Mouth)) return false;
  const MatrixAnimation *animation = findMatrixAnimation(animationName);
  if (!animation) return false;

  MatrixAnimation descriptor;
  memcpy_P(&descriptor, animation, sizeof(descriptor));
  if (!descriptor.frames || descriptor.frameCount == 0) return false;
  for (uint8_t f = 0; f < descriptor.frameCount; ++f) {
    const ColorFrame frame = readFrame(descriptor, f);
    if (frame.durationMs == 0 || (descriptor.width == 0 && frame.pixels)) return false;
  }
  // Validate every member before changing any playback state.
  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    if (includesMatrix(target, i) && !compatibleWithMatrix(descriptor, i)) return false;
  }
  const uint32_t startedAt = millis();
  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    if (!includesMatrix(target, i)) continue;
    playback[i].animation = animation;
    playback[i].frameStartedAt = startedAt;
    playback[i].frameIndex = 0;
    playback[i].dirty = true;
  }
  return true;
}

void matrixLoop() {
  // Use one timestamp for all displays, even though I2C writes are sequential.
  const uint32_t now = millis();
  for (uint8_t i = 0; i < MATRIX_COUNT; ++i) {
    MatrixPlayback &state = playback[i];
    if (!state.animation) continue;

    MatrixAnimation animation;
    memcpy_P(&animation, state.animation, sizeof(animation));
    ColorFrame frame = readFrame(animation, state.frameIndex);
    uint32_t elapsed = now - state.frameStartedAt;
    if (!state.dirty && (animation.frameCount == 1 || elapsed < frame.durationMs)) continue;

    if (animation.frameCount > 1 && elapsed >= frame.durationMs) {
      uint32_t cycleDuration = 0;
      for (uint8_t f = 0; f < animation.frameCount; ++f) {
        cycleDuration += readFrame(animation, f).durationMs;
      }
      // Skip complete cycles after a stall; keep both halves on the same timeline.
      const uint32_t skipped = (elapsed / cycleDuration) * cycleDuration;
      state.frameStartedAt += skipped;
      elapsed -= skipped;
      while (elapsed >= frame.durationMs) {
        elapsed -= frame.durationMs;
        state.frameStartedAt += frame.durationMs;
        state.frameIndex = (state.frameIndex + 1) % animation.frameCount;
        frame = readFrame(animation, state.frameIndex);
      }
    }

    // Render only the current frame, without sending missed frames over I2C.
    showColorFrame(i, animation, frame);
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
