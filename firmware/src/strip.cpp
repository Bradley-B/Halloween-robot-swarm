#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <string.h>

#include "strip.h"

namespace {
constexpr uint8_t LED_PIN = 14;
constexpr uint16_t LED_COUNT = 60;
constexpr uint16_t CHASE_LENGTH = 5;
constexpr float PULSE_TWO_PI = 6.28318530718f;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

enum class StripMode { Rainbow, PulseSine, PulseSquare, Chase, ChaseBounce, Solid, Off };

StripMode mode = StripMode::Rainbow;
uint32_t color = 0xff9f43;
uint32_t frameIntervalMs = STRIP_DEFAULT_FRAME_INTERVAL_MS;
uint32_t pulsePeriodMs = STRIP_DEFAULT_PULSE_PERIOD_MS;
uint32_t animationStartedAt = 0;
uint32_t lastFrameAt = 0;
bool dirty = true;

bool parseMode(const char *value, StripMode &result) {
  if (!value) return false;
  if (strcmp(value, "rainbow") == 0) result = StripMode::Rainbow;
  else if (strcmp(value, "pulse_sine") == 0) result = StripMode::PulseSine;
  else if (strcmp(value, "pulse_square") == 0) result = StripMode::PulseSquare;
  else if (strcmp(value, "chase") == 0) result = StripMode::Chase;
  else if (strcmp(value, "chase_bounce") == 0) result = StripMode::ChaseBounce;
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

bool ledSetState(const char *newMode, const char *newColor,
                 uint32_t newFrameIntervalMs, uint32_t newPulsePeriodMs) {
  StripMode parsedMode;
  uint32_t parsedColor;
  if (!parseMode(newMode, parsedMode) || !parseColor(newColor, parsedColor) ||
      newFrameIntervalMs < 1 || newFrameIntervalMs > 60000 ||
      newPulsePeriodMs < 2 || newPulsePeriodMs > 60000) {
    return false;
  }

  mode = parsedMode;
  color = parsedColor;
  frameIntervalMs = newFrameIntervalMs;
  pulsePeriodMs = newPulsePeriodMs;
  animationStartedAt = millis();
  dirty = true;
  return true;
}

void ledLoop() {
  const uint32_t now = millis();
  const bool animated = mode == StripMode::Rainbow ||
                        mode == StripMode::PulseSine ||
                        mode == StripMode::PulseSquare ||
                        mode == StripMode::Chase ||
                        mode == StripMode::ChaseBounce;
  if (!dirty && (!animated || now - lastFrameAt < frameIntervalMs)) return;

  lastFrameAt = now;
  const uint32_t elapsed = now - animationStartedAt;
  switch (mode) {
    case StripMode::Rainbow:
      strip.rainbow(static_cast<uint16_t>((elapsed / frameIntervalMs) * 256));
      break;
    case StripMode::PulseSine: {
      const float phase = PULSE_TWO_PI * (elapsed % pulsePeriodMs) / pulsePeriodMs;
      const uint8_t level = static_cast<uint8_t>(255.0f * (0.5f + 0.5f * cosf(phase)));
      strip.fill(scaledColor(level));
      break;
    }
    case StripMode::PulseSquare:
      strip.fill(elapsed % pulsePeriodMs < pulsePeriodMs / 2 ? color : 0);
      break;
    case StripMode::Chase: {
      const uint16_t position = (elapsed / frameIntervalMs) % LED_COUNT;
      strip.clear();
      for (uint16_t i = 0; i < CHASE_LENGTH; ++i) {
        strip.setPixelColor((position + i) % LED_COUNT, color);
      }
      break;
    }
    case StripMode::ChaseBounce: {
      // Keep the entire lit group on the strip and reflect at each end.
      static_assert(CHASE_LENGTH < LED_COUNT, "Bouncing chase needs room to move");
      constexpr uint16_t travel = LED_COUNT - CHASE_LENGTH;
      const uint16_t step = (elapsed / frameIntervalMs) % (2 * travel);
      const uint16_t position = step <= travel ? step : 2 * travel - step;
      strip.clear();
      for (uint16_t i = 0; i < CHASE_LENGTH; ++i) {
        strip.setPixelColor(position + i, color);
      }
      break;
    }
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
