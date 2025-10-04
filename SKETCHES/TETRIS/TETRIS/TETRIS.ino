#include <Arduino.h>
#include <U8x8lib.h>

#define F_CPU 8000000UL

// Button pins
#define BTN_LEFT PB4
#define BTN_RIGHT PB3
#define BTN_ROTATE PB1

// OLED pins
#define OLED_SCL PB2
#define OLED_SDA PB0

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

const uint8_t WIDTH = 14;   // inside play area
const uint8_t HEIGHT = 6;   // inside play area


char board[HEIGHT][WIDTH]; // ' ' empty, '#' filled

// All 7 tetrominoes (T, O, I, S, Z, J, L)
const int8_t shapes[7][4][2] PROGMEM = {
  {{0,0},{1,0},{-1,0},{0,1}},    // T
  {{0,0},{1,0},{0,1},{1,1}},     // O
  {{0,0},{1,0},{2,0},{3,0}},     // I
  {{0,0},{1,0},{0,1},{-1,1}},    // S
  {{0,0},{-1,0},{0,1},{1,1}},    // Z
  {{0,0},{-1,0},{-1,1},{1,0}},   // J
  {{0,0},{1,0},{1,1},{-1,0}},    // L
};

struct Piece {
  int8_t x,y;
  uint8_t shape;
  uint8_t rotation;
};

Piece currentPiece;

uint32_t lastFall = 0;
uint16_t baseFallDelay = 700;
uint16_t fallDelay;

uint16_t score = 0;
uint8_t level = 1;
uint16_t linesCleared = 0;

void clearBoard() {
  for (uint8_t y=0;y<HEIGHT;y++)
    for (uint8_t x=0;x<WIDTH;x++)
      board[y][x] = ' ';
}

void rotateCoords(int8_t *x, int8_t *y, uint8_t r) {
  int8_t tx = *x, ty = *y;
  switch(r & 3) {
    case 0: break;
    case 1: *x = -ty; *y = tx; break;
    case 2: *x = -tx; *y = -ty; break;
    case 3: *x = ty; *y = -tx; break;
  }
}

void getBlockPos(Piece p, uint8_t i, int8_t *outX, int8_t *outY) {
  int8_t bx = pgm_read_byte(&(shapes[p.shape][i][0]));
  int8_t by = pgm_read_byte(&(shapes[p.shape][i][1]));
  rotateCoords(&bx, &by, p.rotation);
  *outX = p.x + bx;
  *outY = p.y + by;
}

bool checkCollision(Piece p) {
  for (uint8_t i=0; i<4; i++) {
    int8_t x, y;
    getBlockPos(p, i, &x, &y);
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return true;
    if (board[y][x] != ' ') return true;
  }
  return false;
}

void placePiece(Piece p) {
  for (uint8_t i=0; i<4; i++) {
    int8_t x, y;
    getBlockPos(p, i, &x, &y);
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
      board[y][x] = '#';
  }
}

uint8_t removeFullLines() {
  uint8_t lines = 0;
  for (int8_t y = HEIGHT - 1; y >= 0; y--) {
    bool full = true;
    for (uint8_t x = 0; x < WIDTH; x++) {
      if (board[y][x] == ' ') {
        full = false;
        break;
      }
    }
    if (full) {
      lines++;
      for (int8_t ty = y; ty > 0; ty--)
        memcpy(board[ty], board[ty - 1], WIDTH);
      memset(board[0], ' ', WIDTH);
      y++; // recheck this row
    }
  }
  return lines;
}

void newPiece() {
  currentPiece.x = WIDTH / 2;
  currentPiece.y = 0;
  currentPiece.shape = random(0, 7);
  currentPiece.rotation = 0;
  if (checkCollision(currentPiece)) {
    // Game Over
    score = 0;
    level = 1;
    linesCleared = 0;
    clearBoard();
  }
}

void updateFallDelay() {
  fallDelay = baseFallDelay > (level-1)*50 ? baseFallDelay - (level-1)*50 : 200;
}

void drawBoard() {
  // Top border
  u8x8.setCursor(0, 0);
  for (uint8_t x = 0; x < WIDTH+2; x++) u8x8.print('#');

  // Playfield with side walls
  for (uint8_t y = 0; y < HEIGHT; y++) {
    u8x8.setCursor(0, y+1);
    u8x8.print('#'); // left
    for (uint8_t x = 0; x < WIDTH; x++) {
      char c = board[y][x];
      // Overlay current falling piece
      for (uint8_t i = 0; i < 4; i++) {
        int8_t px, py;
        getBlockPos(currentPiece, i, &px, &py);
        if (px == x && py == y) {
          c = '#';
          break;
        }
      }
      u8x8.print(c);
    }
    u8x8.print('#'); // right
  }

  // Bottom border
  u8x8.setCursor(0, HEIGHT+1);
  for (uint8_t x = 0; x < WIDTH+2; x++) u8x8.print('#');

  // Score & level on last line
  char info[17];
  snprintf(info, sizeof(info), "S:%4d L:%2d", score, level);
  u8x8.setCursor(0, HEIGHT+2);
  u8x8.print(info);
}

void setup() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_ROTATE, INPUT_PULLUP);

  randomSeed(analogRead(3));
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  clearBoard();
  newPiece();
  updateFallDelay();
}

void loop() {
  bool left   = !digitalRead(BTN_LEFT);
  bool right  = !digitalRead(BTN_RIGHT);
  bool rotate = !digitalRead(BTN_ROTATE);

  static uint32_t lastInput = 0;
  static bool lastRotateState = false;
  uint32_t now = millis();

  // left/right share 150ms debounce
  if (now - lastInput > 150) {
    Piece moved = currentPiece;
    if (left) {
      moved.x--;
      if (!checkCollision(moved)) currentPiece = moved;
      lastInput = now;
    } else if (right) {
      moved.x++;
      if (!checkCollision(moved)) currentPiece = moved;
      lastInput = now;
    }
  }

  // rotation = trigger once per button press, no long delay
  if (rotate && !lastRotateState) {
    Piece moved = currentPiece;
    moved.rotation = (moved.rotation + 1) & 3;
    if (!checkCollision(moved)) currentPiece = moved;
  }
  lastRotateState = rotate;

  // falling logic (unchanged)
  if (now - lastFall > fallDelay) {
    Piece moved = currentPiece;
    moved.y++;
    if (checkCollision(moved)) {
      placePiece(currentPiece);
      uint8_t cleared = removeFullLines();
      if (cleared) {
        score += cleared * 100;
        linesCleared += cleared;
        uint8_t newLevel = 1 + linesCleared / 5;
        if (newLevel != level) {
          level = newLevel;
          updateFallDelay();
        }
      }
      newPiece();
    } else {
      currentPiece = moved;
    }
    lastFall = now;
  }

  drawBoard();
  delay(20);
}

