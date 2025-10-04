#include <Arduino.h>
#include <U8x8lib.h>

// Pins
#define BTN_UP    PB3
#define BTN_DOWN  PB4
#define BTN_START PB1
#define OLED_SCL  PB2
#define OLED_SDA  PB0

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

const uint8_t WIDTH  = 16;
const uint8_t HEIGHT = 8;

// Paddle & ball
uint8_t paddleL = HEIGHT/2 - 1, paddleR = HEIGHT/2 - 1;
uint8_t prevPaddleL = 0, prevPaddleR = 0;
uint8_t ballX = WIDTH/2, ballY = HEIGHT/2;
uint8_t prevBallX = WIDTH/2, prevBallY = HEIGHT/2;
int8_t ballDX = 1, ballDY = 1;

// Scores
uint8_t playerScore = 0;
uint8_t aiScore = 0;

// Timers
uint32_t lastBallMove = 0;
uint32_t lastAiMove   = 0;
uint32_t lastInput    = 0;

// Game state
bool running = false;

void drawObjects() {
  // Erase previous ball
  u8x8.setCursor(prevBallX, prevBallY); u8x8.print(' ');
  // Erase previous paddles
  for(uint8_t y=0;y<3;y++){
    u8x8.setCursor(0, prevPaddleL+y); u8x8.print(' ');
    u8x8.setCursor(WIDTH-1, prevPaddleR+y); u8x8.print(' ');
  }
  // Draw paddles
  for(uint8_t y=0;y<3;y++){
    u8x8.setCursor(0, paddleL+y); u8x8.print('|');
    u8x8.setCursor(WIDTH-1, paddleR+y); u8x8.print('|');
  }
  // Draw ball
  u8x8.setCursor(ballX, ballY); u8x8.print('O');

  // Save positions
  prevBallX = ballX; prevBallY = ballY;
  prevPaddleL = paddleL; prevPaddleR = paddleR;

  // Display score
  char info[17];
  snprintf(info, sizeof(info), "%d : %d", playerScore, aiScore);
  u8x8.setCursor(5,0);
  u8x8.print(info);
}

void resetGame() {
  paddleL = paddleR = HEIGHT/2 - 1;
  ballX = WIDTH/2; ballY = HEIGHT/2;
  ballDX = 1; ballDY = 1;
  aiScore = 0;
  playerScore = 0;
}

void setup() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);

  randomSeed(analogRead(3));
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  // Show menu
  u8x8.clear();
  u8x8.setCursor(3,3); u8x8.print("PONG");
  u8x8.setCursor(0,5); u8x8.print("'C' To Start");
}

void loop() {
  bool up    = !digitalRead(BTN_UP);
  bool down  = !digitalRead(BTN_DOWN);
  bool start = !digitalRead(BTN_START);
  uint32_t now = millis();

  if(!running) {
    if(start) {
      running = true;
      resetGame();
      lastBallMove = lastAiMove = now;
      u8x8.clear();
      drawObjects();
      delay(200); // debounce
    }
    return;
  }

  // Player input (PB3/4)
  if(now - lastInput > 100) {
    if(up && paddleL>0) paddleL--;
    if(down && paddleL<HEIGHT-3) paddleL++;
    lastInput = now;
  }

  // AI paddle movement (right side)
  uint8_t difficulty = map(playerScore, 0, 5, 170, 110); 
  // slower overall (170ms → 110ms)

  if(now - lastAiMove > difficulty) {
    int errorChance = random(0, 100);
    int targetY = ballY;

    if(errorChance < 25) targetY += random(-2, 3); 
    // 25% chance AI "aims" a bit wrong

    if(targetY > paddleR+1 && paddleR < HEIGHT-3) paddleR++;
    else if(targetY < paddleR+1 && paddleR > 0) paddleR--;

    lastAiMove = now;
}


 // Ball movement
if(now - lastBallMove > 200) {
  ballX += ballDX;
  ballY += ballDY;

  // Top/bottom collision
  if(ballY == 0 || ballY == HEIGHT-1) ballDY = -ballDY;

  // Left paddle collision
  if(ballX == 1 && ballY >= paddleL && ballY <= paddleL+2) {
    ballDX = 1; // bounce right
  }

  // Right paddle collision
  if(ballX == WIDTH-2 && ballY >= paddleR && ballY <= paddleR+2) {
    ballDX = -1; // bounce left
  }
    // Missed left side -> AI scores
    if(ballX == 0) {
    aiScore++;
    // Check for win/lose
    if(aiScore == 10) {
        running = false;
        u8x8.clear();
        u8x8.setCursor(3,3); u8x8.print("YOU LOSE!");
        u8x8.setCursor(1,5); u8x8.print("'C' TO RESTART");
        return; // stop further updates
    }
    // Reset ball
    ballX = WIDTH/2; ballY = HEIGHT/2;
    ballDX = 1; ballDY = (random(0,2)?1:-1);
}

    // Missed right side -> Player scores
    if(ballX == WIDTH-1) {
    playerScore++;
    // Check for win/lose
    if(playerScore == 10) {
        running = false;
        u8x8.clear();
        u8x8.setCursor(4,3); u8x8.print("YOU WIN!");
        u8x8.setCursor(1,5); u8x8.print("'C' TO RESTART");
        return; // stop further updates
    }
    // Reset ball
    ballX = WIDTH/2; ballY = HEIGHT/2;
    ballDX = -1; ballDY = (random(0,2)?1:-1);
}

  drawObjects();
  lastBallMove = now;
  }

  delay(10); // tiny delay to reduce CPU load
}
