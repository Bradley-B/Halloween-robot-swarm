#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <string.h>

#include "strip.h"

namespace {
constexpr uint8_t LED_PIN = 14;
constexpr uint16_t LED_COUNT = 60;
constexpr uint32_t FRAME_INTERVAL_MS = 10;
constexpr uint32_t PULSE_PERIOD_MS = 2000;
constexpr float PULSE_TWO_PI = 6.28318530718f;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGBW + NEO_KHZ800);

enum class StripMode { Rainbow, PulseSine, PulseSquare, Solid, Off };

StripMode mode = StripMode::Rainbow;
uint32_t color = 0xff9f43;
uint32_t animationStartedAt = 0;
uint32_t lastFrameAt = 0;
bool dirty = true;

bool parseMode(const char *value, StripMode &result) {
  if (!value) return false;
  if (strcmp(value, "rainbow") == 0) result = StripMode::Rainbow;
  else if (strcmp(value, "pulse_sine") == 0) result = StripMode::PulseSine;
  else if (strcmp(value, "pulse_square") == 0) result = StripMode::PulseSquare;
  else if (strcmp(value, "solid") == 0) result = StripMode::Solid;
  else if (strcmp(value, "off") == 0) result = StripMode::Off;
  else return false;
  return true;
}

bool parseColor(const char *value, uint32_t &result) {
  if (!value || strlen(value) != 7 || value[0] != '#') return false;
  result = 0;
  for (uint8_t i = 1; i < 7; ++i) {
    const char digit = value[i];
    uint8_t nibble;
    if (digit >= '0' && digit <= '9') nibble = digit - '0';
    else if (digit >= 'a' && digit <= 'f') nibble = digit - 'a' + 10;
    else if (digit >= 'A' && digit <= 'F') nibble = digit - 'A' + 10;
    else return false;
    result = (result << 4) | nibble;
  }
  return true;
}

uint32_t scaledColor(uint8_t level) {
  const uint8_t red = ((color >> 16) & 0xff) * level / 255;
  const uint8_t green = ((color >> 8) & 0xff) * level / 255;
  const uint8_t blue = (color & 0xff) * level / 255;
  return strip.Color(red, green, blue);
}
} // namespace

void ledSetup() {
  strip.begin();
  strip.setBrightness(50);
  strip.show();
  animationStartedAt = millis();
  dirty = true;
}

bool ledSetState(const char *newMode, const char *newColor) {
  StripMode parsedMode;
  uint32_t parsedColor;
  if (!parseMode(newMode, parsedMode) || !parseColor(newColor, parsedColor)) {
    return false;
  }

  mode = parsedMode;
  color = parsedColor;
  animationStartedAt = millis();
  dirty = true;
  return true;
}

void ledLoop() {
  const uint32_t now = millis();
  const bool animated = mode == StripMode::Rainbow ||
                        mode == StripMode::PulseSine ||
                        mode == StripMode::PulseSquare;
  if (!dirty && (!animated || now - lastFrameAt < FRAME_INTERVAL_MS)) return;

  lastFrameAt = now;
  const uint32_t elapsed = now - animationStartedAt;
  switch (mode) {
    case StripMode::Rainbow:
      strip.rainbow(static_cast<uint16_t>((elapsed / FRAME_INTERVAL_MS) * 256));
      break;
    case StripMode::PulseSine: {
      const float phase = PULSE_TWO_PI * (elapsed % PULSE_PERIOD_MS) / PULSE_PERIOD_MS;
      const uint8_t level = static_cast<uint8_t>(255.0f * (0.5f + 0.5f * cosf(phase)));
      strip.fill(scaledColor(level));
      break;
    }
    case StripMode::PulseSquare:
      strip.fill(elapsed % PULSE_PERIOD_MS < PULSE_PERIOD_MS / 2 ? color : 0);
      break;
    case StripMode::Solid:
      strip.fill(color);
      break;
    case StripMode::Off:
      strip.clear();
      break;
  }
  strip.show();
  dirty = false;
}
