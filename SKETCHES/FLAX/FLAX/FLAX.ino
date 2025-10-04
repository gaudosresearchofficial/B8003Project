#include <Arduino.h>
#include <U8g2lib.h>

// Page-buffer mode software I2C (minimal RAM)
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u(PB2, PB0, U8X8_PIN_NONE);

#define BTN PB1

// Player and obstacle variables
int py=30, vel=0, ox=127, gap=28, score=0, lastScore=0;
bool running=false;

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  u.begin();
  randomSeed(analogRead(0));
}

void resetGame() {
  py=30; vel=0; ox=127; gap=random(10,40); score=0;
  running=true;
}

void drawGame() {
  u.firstPage();
  do {
    // Player
    u.drawBox(10, py, 4, 4);
    // Obstacle
    u.drawBox(ox, 0, 5, gap);
    u.drawBox(ox, gap+20, 5, 64-(gap+20));
    // Score
    u.setCursor(0,10); u.print(score);
  } while(u.nextPage());
}

bool checkCollision() {
  return (10+4>ox && 10<ox+5 && (py<gap || py+4>gap+20)) || py<0 || py+4>64;
}

void loop() {
  // Menu
  while(!running){
    u.firstPage();
    do {
      u.setCursor(30,20); u.print("Flax");
      u.setCursor(20,35); u.print("PB1=Start");
      u.setCursor(20,50); u.print("Score:"); u.print(lastScore);
    } while(u.nextPage());

    if(!digitalRead(BTN)) { resetGame(); delay(150); }
  }

  static unsigned long lastFrame=0;
  while(running){
    if(millis()-lastFrame<40) continue;
    lastFrame=millis();

    // Jump
    if(!digitalRead(BTN)) vel=-2;

    // Physics
    vel+=1; py+=vel; if(py<0) py=0; if(py>60) py=60;

    // Move obstacle
    ox-=2;
    if(ox<-5){ ox=127; gap=random(10,40); score++; }

    drawGame();

    if(checkCollision()){
      running=false;
      lastScore=score;
      // Minimal Game Over display
      u.firstPage();
      do {
        u.setCursor(30,20); u.print("Game Over!");
        u.setCursor(30,35); u.print("Score:"); u.print(score);
      } while(u.nextPage());
      delay(1000);
    }
  }
}
