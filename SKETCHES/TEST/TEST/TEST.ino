#include <Arduino.h>
#include <U8x8lib.h>
#include <avr/pgmspace.h>

// Software I2C pins (adjust to your board)
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(2, 0, U8X8_PIN_NONE);

// Smallest font for minimal size
#define FONT u8x8_font_5x7_f

// Words stored in PROGMEM
const char w0[] PROGMEM = "B8003";
const char w1[] PROGMEM = "Handheld";
const char w2[] PROGMEM = "Games";
const char w3[] PROGMEM = "Console";

const char* const words[] PROGMEM = {w0, w1, w2, w3};

uint8_t animStep = 0;
int speedDelay = 200;
int speedDir = -10;
const int minDelay = 10;
const int maxDelay = 200;

// Read and print word from PROGMEM directly without buffer
void printWord(uint8_t idx, uint8_t x, uint8_t y) {
  u8x8.setCursor(x, y);
  // Print chars one by one from PROGMEM
  const char* ptr = (const char*)pgm_read_word(&words[idx]);
  char c;
  while ((c = pgm_read_byte(ptr++))) {
    u8x8.print(c);
  }
}

void drawTitle() {
  u8x8.setFont(FONT);
  u8x8.clearLine(2);
  u8x8.clearLine(3);
  u8x8.setCursor(4, 2);  // Center approx for "B8003"
  u8x8.print(F("B8003"));
  u8x8.setCursor(1, 3);  // Center approx for "Games Console"
  u8x8.print(F("Games Console"));
}

void setup() {
  u8x8.begin();
  u8x8.setFont(FONT);
  u8x8.clearDisplay();
  drawTitle();
}

void loop() {
  // Clear lines except title lines
  for (uint8_t l = 0; l < 8; l++) {
    if (l != 2 && l != 3) u8x8.clearLine(l);
  }

  uint8_t step = animStep % 16;

  switch(step) {
    case 0:  printWord(0, 5, 0); break;
    case 1:  printWord(1, 5, 7); break;
    case 2:  printWord(2, 0, 4); break;
    case 3:  printWord(3, 9, 4); break;

    case 4:  printWord(1, 5, 0); break;
    case 5:  printWord(2, 5, 7); break;
    case 6:  printWord(3, 0, 4); break;
    case 7:  printWord(0, 9, 4); break;

    case 8:  printWord(2, 5, 0); break;
    case 9:  printWord(3, 5, 7); break;
    case 10: printWord(0, 0, 4); break;
    case 11: printWord(1, 9, 4); break;

    case 12: printWord(3, 5, 0); break;
    case 13: printWord(0, 5, 7); break;
    case 14: printWord(1, 0, 4); break;
    case 15: printWord(2, 9, 4); break;
  }

  animStep++;
  delay(speedDelay);

  speedDelay += speedDir;
  if (speedDelay <= minDelay || speedDelay >= maxDelay) {
    speedDir = -speedDir;
  }
}
