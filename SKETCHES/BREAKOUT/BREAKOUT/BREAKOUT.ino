// BreakDot - Smooth Breakout for ATtiny85 @ 8MHz
#include <Arduino.h>
#include <U8x8lib.h>

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 2, /* data=*/ 0, /* reset=*/ U8X8_PIN_NONE);

#define BTN_LEFT 3
#define BTN_RIGHT 4
#define BTN_START 1

#define PADDLE_WIDTH 4
#define BRICK_ROWS 3
#define BRICK_COLS 16

bool bricks[BRICK_ROWS][BRICK_COLS];
int paddleX = 6;
int ballX = 8, ballY = 5;
int ballDX = 1, ballDY = -1;
bool inGame = false;
bool gameOver = false;
bool gameWin = false;

int prevBallX = 8, prevBallY = 5;
unsigned long lastBallUpdate = 0;

void drawStartScreen() {
  u8x8.clear();
  u8x8.setCursor(3, 2); u8x8.print("BREAKDOT");
  u8x8.setCursor(2, 4); u8x8.print("PB1 TO START");
}

void drawGameOver() {
  u8x8.clear();
  u8x8.setCursor(3, 3); u8x8.print("GAME OVER");
  u8x8.setCursor(2, 5); u8x8.print("PB1 TO RESTART");
}

void drawWin() {
  u8x8.clear();
  u8x8.setCursor(4, 3); u8x8.print("YOU WIN!");
  u8x8.setCursor(2, 5); u8x8.print("PB1 TO RESTART");
}

void resetGame() {
  inGame = true;
  gameOver = false;
  gameWin = false;
  paddleX = 6;
  ballX = prevBallX = 8;
  ballY = prevBallY = 5;
  ballDX = 1;
  ballDY = -1;
  for (int r = 0; r < BRICK_ROWS; r++)
    for (int c = 0; c < BRICK_COLS; c++)
      bricks[r][c] = true;
  u8x8.clear();
}

void drawBricks() {
  for (int r = 0; r < BRICK_ROWS; r++)
    for (int c = 0; c < BRICK_COLS; c++)
      if (bricks[r][c]) {
        u8x8.setCursor(c, r);
        u8x8.print("#");
      }
}

void drawPaddle() {
  u8x8.clearLine(7);
  for (int i = 0; i < PADDLE_WIDTH; i++) {
    u8x8.setCursor(paddleX + i, 7);
    u8x8.print("=");
  }
}

void drawBall() {
  if (ballX != prevBallX || ballY != prevBallY) {
    u8x8.setCursor(prevBallX, prevBallY);
    u8x8.print(" ");
    prevBallX = ballX;
    prevBallY = ballY;
  }
  u8x8.setCursor(ballX, ballY);
  u8x8.print("o");
}

void updateBall() {
  ballX += ballDX;
  ballY += ballDY;

  if (ballX <= 0 || ballX >= 15) ballDX *= -1;
  if (ballY <= 0) ballDY *= -1;

  if (ballY == 6 && ballX >= paddleX && ballX < paddleX + PADDLE_WIDTH)
    ballDY = -1;

  if (ballY < BRICK_ROWS && ballY >= 0 && bricks[ballY][ballX]) {
    bricks[ballY][ballX] = false;
    ballDY *= -1;
  }

  if (ballY > 7) {
    inGame = false;
    gameOver = true;
  }

  bool anyLeft = false;
  for (int r = 0; r < BRICK_ROWS; r++)
    for (int c = 0; c < BRICK_COLS; c++)
      if (bricks[r][c]) anyLeft = true;

  if (!anyLeft) {
    inGame = false;
    gameWin = true;
  }
}

void setup() {
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  drawStartScreen();
}

void loop() {
  if (!inGame) {
    if (digitalRead(BTN_START) == LOW) {
      resetGame();
    } else {
      if (gameOver) drawGameOver();
      else if (gameWin) drawWin();
      delay(100);
      return;
    }
  }

  if (digitalRead(BTN_LEFT) == LOW && paddleX > 0) paddleX--;
  if (digitalRead(BTN_RIGHT) == LOW && paddleX < 16 - PADDLE_WIDTH) paddleX++;

  drawBricks();
  drawPaddle();

  unsigned long now = millis();
  if (now - lastBallUpdate >= 120) {
    updateBall();
    lastBallUpdate = now;
  }

  drawBall();
  delay(10);
}
