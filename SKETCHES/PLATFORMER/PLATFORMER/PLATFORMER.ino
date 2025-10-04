#include <Arduino.h>
#include <U8x8lib.h>

#define F_CPU 8000000UL

// Pins
#define BTN_LEFT PB3
#define BTN_RIGHT PB4
#define BTN_JUMP PB1
#define OLED_SCL PB2
#define OLED_SDA PB0

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

const uint8_t SCREEN_WIDTH = 16;
const uint8_t SCREEN_HEIGHT = 8;

char terrain[SCREEN_WIDTH + 1]; // current level terrain, null-terminated

const char levelData[][17] PROGMEM = {
  "___$______$___*",  //0
  "__X__V__$__V__*",  //1
  "_$___V___X___$*",  //2
  "_V_X_$__V___X_*",  //3
  "__X_V___B___$_*",  //4 boss
  "___$X___$___X_*",  //5
  "_V__$_V___$__X*",  //6
  "__X___V__$__X_*",  //7
  "___V___$___V__*",  //8
  "__B___X___$_X_*",  //9 boss
  "_$____$__X_V__*",  //10
  "__X__X___V__$_*",  //11
  "_V__$_X___$_X_*",  //12
  "_X___$_V__$X__*",  //13
  "__$__B__X___$_*",  //14 boss
  "__X__$_V___$_*",  //15
  "__V__X____V__*",  //16
  "_$_X___$__X__*",  //17
  "___V___X___V_*",  //18
  "__$_B___$_V__*",  //19 boss
  "_$_X___$__X__*",  //20
  "__V__$V___$__*",  //21
  "_X___$_X__V__*",  //22
  "__V___$___X__*",  //23
  "___B___X__$__*",  //24 boss
};

uint8_t currentLevel = 0;

uint8_t playerX = 0;
int8_t playerY = 5; // vertical row (0 top, 7 bottom), ground approx row 6
int8_t playerVY = 0;

const int8_t GRAVITY = 1;
const int8_t JUMP_VELOCITY = -3;

uint16_t coinsCollected = 0;

void loadLevel(uint8_t level) {
  if (level >= sizeof(levelData) / sizeof(levelData[0])) level = 0;
  strcpy_P(terrain, levelData[level]);
  playerX = 0;
  playerY = 5;
  playerVY = 0;
}

void drawScreen() {
  u8x8.clear();

  // Draw terrain on row 6 (ground)
  u8x8.setCursor(0, 6);
  u8x8.print(terrain);

  // Draw player '@' at playerX, playerY
  u8x8.setCursor(playerX, playerY);
  u8x8.print('@');

  // HUD row 0: level and coins
  u8x8.setCursor(0, 0);
  u8x8.printf("Lvl:%d $:%d", currentLevel + 1, coinsCollected);
}

void killPlayer(const char* reason) {
  u8x8.clear();
  u8x8.setCursor(0, 3);
  u8x8.print("You died!");
  u8x8.setCursor(0, 4);
  u8x8.print(reason);
  delay(1200);
  loadLevel(currentLevel);
}

void collectCoin() {
  coinsCollected++;
  terrain[playerX] = '_';
}

void nextLevel() {
  currentLevel++;
  if (currentLevel >= sizeof(levelData) / sizeof(levelData[0])) {
    u8x8.clear();
    u8x8.setCursor(2, 3);
    u8x8.print("You win!");
    while (true) delay(1000);
  }
  loadLevel(currentLevel);
}

void bossDefeated() {
  u8x8.clear();
  u8x8.setCursor(1, 3);
  u8x8.print("Boss Defeated!");
  delay(900);
}

bool isOnGround() {
  return playerY >= 5; // ground is row 6, player stands one above
}

// Attack boss if jump pressed and adjacent on same row
void tryAttackBoss() {
  bool jumpPressed = !digitalRead(BTN_JUMP);
  if (!jumpPressed) return;

  // Left adjacent
  if (playerX > 0 && terrain[playerX - 1] == 'B' && playerY == 5) {
    terrain[playerX - 1] = '_';
    bossDefeated();
  }
  // Right adjacent
  else if (playerX < SCREEN_WIDTH - 1 && terrain[playerX + 1] == 'B' && playerY == 5) {
    terrain[playerX + 1] = '_';
    bossDefeated();
  }
}

void updatePlayer() {
  // Jump input and only if on ground
  bool jumpPressed = !digitalRead(BTN_JUMP);
  if (jumpPressed && isOnGround() && playerVY == 0) {
    playerVY = JUMP_VELOCITY;
  }

  // Apply gravity
  playerVY += GRAVITY;

  // Calculate tentative newY
  int8_t newY = playerY + playerVY;

  // Floor collision - ground at row 5 (player stands at 5, ground at 6)
  if (newY >= 5) {
    newY = 5;
    playerVY = 0;
  }

  // Ceiling collision
  if (newY < 0) {
    newY = 0;
    playerVY = 0;
  }

  playerY = newY;
}

void setup() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_JUMP, INPUT_PULLUP);

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  loadLevel(currentLevel);
  drawScreen();
}

void loop() {
  tryAttackBoss();

  bool btnLeft = !digitalRead(BTN_LEFT);
  bool btnRight = !digitalRead(BTN_RIGHT);

  // Horizontal movement
  if (btnLeft && playerX > 0) {
    playerX--;
  } else if (btnRight && playerX < SCREEN_WIDTH - 1) {
    playerX++;
  }

  updatePlayer();

  // Check tile player stands on or just below, depends on playerY:
  // Since playerY is vertical position, tile is always on ground row (6)
  // So collisions with spikes/enemies are checked on terrain[playerX]
  char tile = terrain[playerX];

  if (tile == 'V') {
    killPlayer("Spiked!");
    return;
  } else if (tile == 'X') {
    killPlayer("Enemy!");
    return;
  } else if (tile == 'B') {
    killPlayer("Boss!");
    return;
  } else if (tile == '$') {
    collectCoin();
  } else if (tile == '*') {
    nextLevel();
  }

  drawScreen();
  delay(150);
}
