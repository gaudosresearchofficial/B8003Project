#include <Arduino.h>
#include <U8x8lib.h>
#include <EEPROM.h>

// --- OLED ---
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(2, 0, U8X8_PIN_NONE);

// --- Pins ---
#define BTN_LEFT 4
#define BTN_RIGHT 3
#define BTN_SHOOT 1

// --- Game constants ---
const uint8_t W = 16;
const uint8_t H = 8;
const uint8_t NUM_ALIENS = 18;
const uint8_t MAX_BULLETS = 3;
const uint8_t MAX_LIVES = 3;
const uint8_t MAX_LEVELS = 4;

// --- Aliens ---
struct Alien { int8_t x,y; bool alive; char icon; };
Alien aliens[NUM_ALIENS];
int8_t alienDirection = 1; // 1=right, -1=left
unsigned long lastAlienMove = 0;
unsigned long alienInterval = 200; // initial speed (ms)
uint8_t level = 1;

// --- Player ---
int8_t playerX;
struct Bullet { bool active; int8_t x,y; };
Bullet bullets[MAX_BULLETS];

// --- Game state ---
bool inMenu = true;
bool gameOver = false;
uint8_t lives = MAX_LIVES;
uint16_t score = 0;
uint16_t highScore;

// --- Helpers ---
inline uint8_t r8(){ return rand() & 0xFF; }
uint8_t aliveCount(){ uint8_t c=0; for(uint8_t i=0;i<NUM_ALIENS;i++) if(aliens[i].alive) c++; return c; }

// --- Initialize aliens ---
void initAliens(){
  for(uint8_t r=0;r<3;r++){
    for(uint8_t c=0;c<6;c++){
      aliens[r*6+c].x = 2 + c*2;
      aliens[r*6+c].y = 1 + r;
      aliens[r*6+c].alive = true;
      aliens[r*6+c].icon = (r==0)?'B':'A';
    }
  }
  alienDirection = 1;
  lastAlienMove = millis();
}

// --- Reset game ---
void resetGame(){
  initAliens();
  playerX = W/2;
  for(uint8_t i=0;i<MAX_BULLETS;i++) bullets[i].active=false;
  lives = MAX_LIVES;
  score = 0;
  level = 1;
  alienInterval = 200;
  gameOver = false;
}

// --- Draw game ---
void draw(){
  u8x8.clearDisplay();
  // Aliens
  for(uint8_t i=0;i<NUM_ALIENS;i++){
    if(aliens[i].alive){
      u8x8.setCursor(aliens[i].x,aliens[i].y);
      u8x8.write(aliens[i].icon);
    }
  }
  // Player
  if(!gameOver){
    u8x8.setCursor(playerX,H-1);
    u8x8.write('^');
  }
  // Bullets
  for(uint8_t i=0;i<MAX_BULLETS;i++){
    if(bullets[i].active){
      u8x8.setCursor(bullets[i].x, bullets[i].y);
      u8x8.write('|');
    }
  }
  // Score and lives
  u8x8.setCursor(0,0);
  u8x8.print(score);
  u8x8.setCursor(W-5,0);
  u8x8.print("L:");
  u8x8.print(lives);
  u8x8.setCursor(W/2-3,0);
  u8x8.print("HS:");
  u8x8.print(highScore);
}

// --- Move aliens ---
void moveAliens(){
  bool edgeHit=false;
  for(uint8_t i=0;i<NUM_ALIENS;i++){
    if(aliens[i].alive){
      if((aliens[i].x <= 0 && alienDirection==-1) || (aliens[i].x >= W-1 && alienDirection==1)){
        edgeHit = true;
        break;
      }
    }
  }
  if(edgeHit){
    for(uint8_t i=0;i<NUM_ALIENS;i++){
      if(aliens[i].alive) aliens[i].y++;
      if(aliens[i].y>=H-1){ // player row reached
        if(lives>0){ lives--; initAliens(); }
        else gameOver=true;
      }
    }
    alienDirection = -alienDirection;
  } else {
    for(uint8_t i=0;i<NUM_ALIENS;i++){
      if(aliens[i].alive) aliens[i].x += alienDirection;
    }
  }
}

// --- Update bullets ---
void updateBullets(){
  for(uint8_t i=0;i<MAX_BULLETS;i++){
    if(!bullets[i].active) continue;
    bullets[i].y--;
    if(bullets[i].y<0){ bullets[i].active=false; continue; }
    for(uint8_t j=0;j<NUM_ALIENS;j++){
      if(aliens[j].alive && aliens[j].x==bullets[i].x && aliens[j].y==bullets[i].y){
        u8x8.setCursor(aliens[j].x,aliens[j].y); u8x8.write('X');
        delay(20); // tiny explosion flash
        aliens[j].alive=false;
        bullets[i].active=false;
        score += (aliens[j].icon=='B')?20:10;
        if(score>highScore){
          highScore=score;
          EEPROM.write(0,highScore&0xFF);
          EEPROM.write(1,highScore>>8);
        }
        break;
      }
    }
  }
}

// --- Handle input ---
void handleInput(){
  if(digitalRead(BTN_LEFT)==LOW && playerX>0) playerX--;
  if(digitalRead(BTN_RIGHT)==LOW && playerX<W-1) playerX++;
  if(digitalRead(BTN_SHOOT)==LOW){
    for(uint8_t i=0;i<MAX_BULLETS;i++){
      if(!bullets[i].active){
        bullets[i].x=playerX;
        bullets[i].y=H-2;
        bullets[i].active=true;
        break;
      }
    }
  }
}

// --- Check level/wave completion ---
void checkLevel(){
  if(aliveCount()==0){
    level++;
    if(level>MAX_LEVELS) level=MAX_LEVELS;
    alienInterval = max(40, 200 - (level*30)); // faster each level
    initAliens();
  } else {
    // speed up as aliens decrease
    alienInterval = max(40, 200 - ((NUM_ALIENS - aliveCount())*5) - (level-1)*20);
  }
}

// --- Menu ---
void menuLoop(){
  u8x8.clear();
  u8x8.setCursor(1,2); u8x8.print("SPACE INVADERS");
  u8x8.setCursor(3,4); u8x8.print("PB1=START");
  while(digitalRead(BTN_SHOOT)==HIGH);
  delay(50);
  inMenu=false;
  resetGame();
}

// --- Setup ---
void setup(){
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  pinMode(BTN_LEFT,INPUT_PULLUP);
  pinMode(BTN_RIGHT,INPUT_PULLUP);
  pinMode(BTN_SHOOT,INPUT_PULLUP);

  // Read high score from EEPROM
  highScore = EEPROM.read(0) | (EEPROM.read(1)<<8);
  if(highScore==0xFFFF){ highScore=0; EEPROM.write(0,0); EEPROM.write(1,0); }

  inMenu=true;
}

// --- Loop ---
void loop(){
  if(inMenu) menuLoop();
  if(gameOver){
    u8x8.clear();
    u8x8.setCursor(3,2); u8x8.print("GAME OVER");
    u8x8.setCursor(1,4); u8x8.print("Score:");
    u8x8.setCursor(8,4); u8x8.print(score);
    u8x8.setCursor(1,5); u8x8.print("High:");
    u8x8.setCursor(7,5); u8x8.print(highScore);
    delay(500);
    inMenu=true;
    return;
  }

  handleInput();
  updateBullets();

  // Alien movement based on interval
  if(millis() - lastAlienMove >= alienInterval){
    moveAliens();
    lastAlienMove = millis();
  }

  checkLevel();
  draw();
}
