#include <Arduino.h>
#include <U8x8lib.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

// --- OLED ---
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(2,0,U8X8_PIN_NONE);

// --- Pins ---
#define BTN_LEFT 4
#define BTN_RIGHT 3
#define BTN_JUMP 1

// --- Constants ---
const uint8_t W=16;
const uint8_t H=8;
const int8_t GRAVITY=1;
const int8_t JUMP_V=-3;
const uint8_t MAX_PLATFORMS=4;
const uint8_t MAX_ENEMIES=3;
const uint8_t MAX_COINS=3;
const uint8_t MAX_POWERBLOCKS=2;
const uint16_t FRAME_DELAY=75;

// --- Player ---
int8_t playerX, playerY;
int8_t velocityY=0;
const char PLAYER_HEAD='@';
const char PLAYER_BODY='&';

// --- Structs ---
struct Platform { int8_t x,y,w; };
struct Enemy { int8_t x,y,dir; bool alive; };
struct Coin { int8_t x,y; bool collected; };
struct PowerBlock { int8_t x,y; bool used; };

// --- RAM arrays ---
Platform platforms[MAX_PLATFORMS];
Enemy enemies[MAX_ENEMIES];
Coin coins[MAX_COINS];
PowerBlock powerblocks[MAX_POWERBLOCKS];

// --- Game state ---
bool inMenu=true;
bool gameOver=false;
uint16_t score=0;
uint16_t highScore;
uint8_t currentLevel=0;

// --- Flag animation ---
bool flagAnimation=false;
uint8_t flagY=0;
uint32_t flagTimer=0;

// --- Level Data in PROGMEM (31 bytes per level) ---
// Format per level:
// platforms: 4 * (x,y,w) = 12 bytes
// enemies:   3 * (x,y,dir) = 9 bytes
// coins:     3 * (x,y) = 6 bytes
// powerblocks: 2 * (x,y) = 4 bytes
// total = 31 bytes
const uint8_t levelData[16][31] PROGMEM = {
  {0,6,6,5,4,5,2,2,4,10,1,5,  1,5,1,6,3,1,0,0,1,  2,5,7,3,11,1,  3,4,10,2}, // L1
  {0,6,5,6,5,4,3,3,6,9,1,5,   2,5,1,7,4,1,0,0,1,  1,5,6,4,10,1,  4,3,12,1}, // L2
  {0,5,6,7,4,5,2,2,5,10,1,5,  1,4,1,8,3,1,0,0,1,  2,4,9,3,11,1,  3,3,12,2}, // L3
  {0,6,5,6,4,6,2,3,4,11,1,5,  1,5,1,7,3,1,0,0,1,  3,5,8,3,12,1,  4,4,10,2}, // L4
  {0,6,4,5,5,5,2,3,6,10,2,5,  1,5,1,6,4,1,3,2,1,  2,5,7,4,11,2,  3,4,9,3},   // L5
  {0,6,5,6,5,5,3,3,4,11,1,5,  1,5,1,7,4,1,4,2,1,  2,5,8,4,12,1,  3,3,10,2}, // L6
  {0,6,6,5,5,4,2,3,5,9,1,6,   1,5,1,6,4,1,3,2,1,  2,5,7,4,10,1,  4,3,11,2}, // L7
  {0,6,5,6,4,5,3,3,6,10,2,5,  2,5,1,7,4,1,4,2,1,  1,5,6,4,11,2,  3,3,12,1}, // L8
  {0,6,5,5,5,5,2,4,6,10,2,5,  1,5,1,6,4,1,3,3,1,  2,5,7,4,11,2,  3,4,10,3}, // L9
  {0,6,6,6,5,4,2,3,5,9,1,6,   1,5,1,7,4,1,4,2,1,  2,5,8,4,10,1,  3,3,11,2}, // L10
  {0,6,5,5,4,5,3,3,6,10,2,5,  2,5,1,7,4,1,4,2,1,  1,5,6,4,11,2,  3,3,12,1}, // L11
  {0,6,6,6,5,5,2,4,5,9,1,5,   1,5,1,7,4,1,3,3,1,  2,5,8,4,10,1,  4,3,11,2}, // L12
  {0,6,5,5,5,4,3,3,6,10,2,5,  2,5,1,7,4,1,4,3,1,  1,5,6,4,11,2,  3,3,12,1}, // L13
  {0,6,6,6,5,5,2,4,5,9,2,6,   1,5,1,7,4,1,3,3,1,  2,5,8,4,10,2,  4,3,11,2}, // L14
  {0,6,5,5,5,5,3,3,6,10,2,5,  2,5,1,7,4,1,4,3,1,  1,5,6,4,11,2,  3,3,12,1}, // L15
  {0,6,6,6,5,4,2,4,5,9,1,6,   1,5,1,7,4,1,2,5,1,  8,4,10,1,  4,3,11,2}  // L16 (final) - note: last level structured to fit 31 bytes
};

// --- Helpers ---
bool onPlatform() {
  for(uint8_t i=0;i<MAX_PLATFORMS;i++)
    if(playerY==platforms[i].y-1 && playerX>=platforms[i].x && playerX<platforms[i].x+platforms[i].w) return true;
  return false;
}

// --- Load level ---
void loadLevel(uint8_t lvl) {
  uint8_t buf[31];
  memcpy_P(buf, levelData[lvl], 31);

  // platforms
  for(uint8_t i=0;i<MAX_PLATFORMS;i++){
    platforms[i].x = buf[i*3 + 0];
    platforms[i].y = buf[i*3 + 1];
    platforms[i].w = buf[i*3 + 2];
  }

  // enemies
  for(uint8_t i=0;i<MAX_ENEMIES;i++){
    enemies[i].x = buf[12 + i*3 + 0];
    enemies[i].y = buf[12 + i*3 + 1];
    enemies[i].dir = buf[12 + i*3 + 2];
    enemies[i].alive = true;
  }

  // coins
  for(uint8_t i=0;i<MAX_COINS;i++){
    coins[i].x = buf[21 + i*2 + 0];
    coins[i].y = buf[21 + i*2 + 1];
    coins[i].collected = false;
  }

  // power blocks
  for(uint8_t i=0;i<MAX_POWERBLOCKS;i++){
    powerblocks[i].x = buf[27 + i*2 + 0];
    powerblocks[i].y = buf[27 + i*2 + 1];
    powerblocks[i].used = false;
  }

  // --- FIX: spawn on first platform ---
  playerX = platforms[0].x + 1;
  playerY = platforms[0].y - 1;
  velocityY = 0;
}


// --- Draw ---
void draw() {
  u8x8.clearDisplay();
  for(uint8_t i=0;i<MAX_PLATFORMS;i++)
    for(uint8_t j=0;j<platforms[i].w;j++){
      u8x8.setCursor(platforms[i].x + j, platforms[i].y);
      u8x8.write('-');
    }

  u8x8.setCursor(playerX, playerY-1); u8x8.write(PLAYER_HEAD);
  u8x8.setCursor(playerX, playerY);   u8x8.write(PLAYER_BODY);

  for(uint8_t i=0;i<MAX_ENEMIES;i++)
    if(enemies[i].alive){
      u8x8.setCursor(enemies[i].x, enemies[i].y);
      u8x8.write('E');
    }

  for(uint8_t i=0;i<MAX_COINS;i++)
    if(!coins[i].collected){
      u8x8.setCursor(coins[i].x, coins[i].y);
      u8x8.write('o');
    }

  for(uint8_t i=0;i<MAX_POWERBLOCKS;i++){
    u8x8.setCursor(powerblocks[i].x, powerblocks[i].y);
    u8x8.write(powerblocks[i].used ? '-' : '*');
  }

  u8x8.setCursor(0,0); u8x8.print(score);
  u8x8.setCursor(W-5,0); u8x8.print("HS:"); u8x8.print(highScore);
  u8x8.setCursor(W/2-2,0); u8x8.print("LV"); u8x8.print(currentLevel+1);
}

// --- Update ---
void update() {
  if(digitalRead(BTN_LEFT)==LOW && playerX>0) playerX--;
  if(digitalRead(BTN_RIGHT)==LOW && playerX<W-1) playerX++;
  if(digitalRead(BTN_JUMP)==LOW && onPlatform()) velocityY = JUMP_V;

  velocityY += GRAVITY;
  playerY += velocityY;

  if(velocityY>0 && onPlatform()){ velocityY=0; playerY--; }
  if(playerY<0) playerY=0;
  if(playerY>=H){
    gameOver=true;
    if(score>highScore){ highScore=score; EEPROM.update(0,highScore&0xFF); EEPROM.update(1,highScore>>8); }
  }

  for(uint8_t i=0;i<MAX_ENEMIES;i++){
    Enemy &e = enemies[i];
    if(!e.alive) continue;
    Platform &p = platforms[i]; // enemy tied to matching platform index
    e.x += e.dir;
    if(e.x <= p.x || e.x >= p.x + p.w - 1) e.dir = -e.dir;

    if(playerY == e.y && playerX == e.x){
      if(velocityY > 0){
        e.alive = false; score += 50; velocityY = JUMP_V/2;
      } else {
        gameOver = true;
        if(score>highScore){ highScore=score; EEPROM.update(0,highScore&0xFF); EEPROM.update(1,highScore>>8); }
      }
    }
  }

  for(uint8_t i=0;i<MAX_COINS;i++)
    if(!coins[i].collected && playerX==coins[i].x && playerY==coins[i].y){ coins[i].collected=true; score+=10; }

  for(uint8_t i=0;i<MAX_POWERBLOCKS;i++){
    PowerBlock &b = powerblocks[i];
    if(!b.used && playerX==b.x && playerY==b.y+1 && velocityY<0){
      b.used = true; score += 20; velocityY = 1;
    }
  }

  if(!flagAnimation && playerX >= W-1){
    flagAnimation = true; flagY = 0; flagTimer = millis();
  }
}

// --- Menu ---
void menuLoop(){
  u8x8.clear();
  u8x8.setCursor(0,1); u8x8.print("ULTIMATE");
  u8x8.setCursor(0,2); u8x8.print("KANNA");
  u8x8.setCursor(0,3); u8x8.print("BROTHERS");
  u8x8.setCursor(2,5); u8x8.print("PB1=START");
  while(digitalRead(BTN_JUMP)==HIGH); delay(50);
  inMenu=false; currentLevel=0; score=0; loadLevel(currentLevel);
}

// --- Setup ---
void setup(){
  u8x8.begin(); u8x8.setFont(u8x8_font_chroma48medium8_r);
  pinMode(BTN_LEFT,INPUT_PULLUP); pinMode(BTN_RIGHT,INPUT_PULLUP); pinMode(BTN_JUMP,INPUT_PULLUP);
  highScore = EEPROM.read(0) | (EEPROM.read(1)<<8);
  inMenu = true;
}

// --- Loop ---
uint32_t lastFrame = 0;
void loop(){
  if(inMenu) { menuLoop(); }

  // Flag animation (slide down)
  if(flagAnimation){
    if(millis() - flagTimer >= 100){
      flagTimer = millis();
      if(flagY < H-1) flagY++;
      else {
        flagAnimation = false;
        currentLevel++;
        if(currentLevel >= 16) gameOver = true;
        else loadLevel(currentLevel);
      }
    }

    u8x8.clearDisplay();
    // draw platforms (current level)
    for(uint8_t i=0;i<MAX_PLATFORMS;i++)
      for(uint8_t j=0;j<platforms[i].w;j++){
        u8x8.setCursor(platforms[i].x + j, platforms[i].y);
        u8x8.write('-');
      }
    // player sliding on flag (rightmost column)
    if(flagY > 0) u8x8.setCursor(W-1, flagY-1), u8x8.write(PLAYER_HEAD);
    u8x8.setCursor(W-1, flagY); u8x8.write(PLAYER_BODY);
    // flagpole (one column left of rightmost)
    for(uint8_t y=0;y<H;y++){ u8x8.setCursor(W-2, y); u8x8.write('|'); }

    u8x8.setCursor(0,0); u8x8.print(score);
    u8x8.setCursor(W-5,0); u8x8.print("HS:"); u8x8.print(highScore);
    u8x8.setCursor(W/2-2,0); u8x8.print("LV"); u8x8.print(currentLevel+1);
    return;
  }

  if(gameOver){
    u8x8.clear();
    if(currentLevel >= 16) u8x8.setCursor(3,3), u8x8.print("YOU WIN!");
    else u8x8.setCursor(1,2), u8x8.print("GAME OVER");
    u8x8.setCursor(3,5); u8x8.print("Score:"); u8x8.print(score);
    u8x8.setCursor(3,6); u8x8.print("High:"); u8x8.print(highScore);
    delay(1000);
    // reset to menu
    gameOver = false;
    inMenu = true;
    return;
  }

  if(millis() - lastFrame >= FRAME_DELAY){
    lastFrame = millis();
    update();
    draw();
  }
}
