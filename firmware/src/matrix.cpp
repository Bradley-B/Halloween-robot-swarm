// Rainbow swirl example for the Adafruit IS31FL3741 13x9 PWM RGB LED Matrix
// Driver w/STEMMA QT / Qwiic connector. This is nearly the same as the
// prior qtmatrix example, but animation is generally smoother as it
// buffers the LED states in RAM and updates everything at once via the
// show() function (a la NeoPixels). This requires a little more memory but
// should still fit on small microcontrollers like Arduino Uno, unless
// there's a lot of other stuff going on.

#include <Adafruit_IS31FL3741.h>
#include "matrix.h"

Adafruit_IS31FL3741_QT_buffered matrix;
Adafruit_IS31FL3741_QT_buffered matrix2;
Adafruit_IS31FL3741_QT_buffered matrix3;
Adafruit_IS31FL3741_QT_buffered matrix4;
// If colors appear wrong on matrix, try invoking constructor like so:
// Adafruit_IS31FL3741_QT_buffered ledmatrix(IS3741_RBG);

// Some boards have just one I2C interface, but some have more...
TwoWire *i2c = &Wire; // e.g. change this to &Wire1 for QT Py RP2040

void matrixInit(Adafruit_IS31FL3741_QT_buffered *matrix) {
  matrix->setLEDscaling(0xFF);
  matrix->setGlobalCurrent(0x10); // 0xFF
  matrix->enable(true); // bring out of shutdown
}

void matrixSetup() {
  Serial.begin(9600); //115200
  Serial.println("Adafruit QT RGB Matrix Simple RGB Swirl Test");

  if (! matrix.begin(IS3741_ADDR_DEFAULT, i2c)) {
    Serial.println("IS41 0x30 not found");
    while (1);
  }

  if (! matrix2.begin(IS3741_ADDR_DEFAULT + 1, i2c)) {
    Serial.println("IS41 0x31 not found");
    while (1);
  }

  if (! matrix3.begin(IS3741_ADDR_DEFAULT + 2, i2c)) {
    Serial.println("IS41 0x32 not found");
    while (1);
  }

  if (! matrix4.begin(IS3741_ADDR_DEFAULT + 3, i2c)) {
    Serial.println("IS41 0x33 not found");
    while (1);
  }

  Serial.println("IS41s found!");

  // By default the LED controller communicates over I2C at 400 KHz.
  // Arduino Uno can usually do 800 KHz, and 32-bit microcontrollers 1 MHz.
  i2c->setClock(800000);

  // Set brightness to max and bring controller out of shutdown state
  matrixInit(&matrix);
  matrixInit(&matrix2);
  matrixInit(&matrix3);
  matrixInit(&matrix4);
}

uint16_t hue_offset = 0;

void matrixLoop() {
  uint32_t i = 0;
  for (int y=0; y<matrix.height(); y++) {
    for (int x=0; x<matrix.width(); x++) {
      uint32_t color888 = matrix.ColorHSV(i * 65536 / 117 + hue_offset);
      uint16_t color565 = matrix.color565(color888);
      matrix.drawPixel(x, y, color565);
      i++;
    }
  }

  matrix.show(); // Buffered matrix MUST use show() to update!

  hue_offset += 256;

  // ledmatrix.setGlobalCurrent(hue_offset / 256); // Demonstrate global current
}

void setMonoFrame(Adafruit_IS31FL3741_QT_buffered *matrix, MonoFrame *frame) {
  uint32_t packedColor = matrix->color565(frame->colorR, frame->colorG, frame->colorB);

  matrix->fillScreen(0);
  matrix->drawBitmap(0, 0, frame->bitmap, 13, 9, packedColor);
  matrix->show();
}

void setMonoFrameAll(MonoFrame *frame) {
  setMonoFrame(&matrix, frame);
  setMonoFrame(&matrix2, frame);
  setMonoFrame(&matrix3, frame);
  setMonoFrame(&matrix4, frame);
}

void writeLetter(Adafruit_IS31FL3741_QT_buffered *matrix, const char *letter) {
  matrix->fill(0);
  matrix->setCursor(0, 0);
  matrix->setTextColor(matrix->color565(0, 0, 255));
  matrix->print(letter);
  matrix->show();
}

void writeLetterAll(const char *letter) {
  writeLetter(&matrix, letter);
  writeLetter(&matrix2, letter);
  writeLetter(&matrix3, letter);
  writeLetter(&matrix4, letter);
}