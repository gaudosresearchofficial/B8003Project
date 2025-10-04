#include <Arduino.h>
#include <U8x8lib.h>

// --- OLED (I2C software) ---
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/2, /* data=*/0, /* reset=*/U8X8_PIN_NONE); 
// PB2=SCL, PB0=SDA

// --- Buttons ---
#define BTN_PASS 3   // PB3
#define BTN_SHOOT 4  // PB4

// --- Game constants ---
const uint8_t W = 16;
const uint8_t H = 8;
const uint16_t GAME_TICKS = 1800; // fast 90 minutes

// --- Player struct ---
struct P { int8_t x,y; bool hasBall; };

P myTeam[3], enemyTeam[3];
int8_t ballX, ballY;
bool ballFree = true;
bool myRight = true;
uint16_t tickCount = 0;
uint8_t scoreA=0, scoreB=0;

// penalties
bool inPenalties=false;
uint8_t penA=0, penB=0, penCount=0; // each side 5

// --- Helpers ---
inline uint8_t r8(){ return rand() & 0xFF; }
inline void clamp(int8_t &x,int8_t &y){ if(x<0)x=0; if(x>=W)x=W-1; if(y<0)y=0; if(y>=H)y=H-1; }

static inline void mvToward(P& p,int8_t tx,int8_t ty){
  if(p.x<tx) p.x++; else if(p.x>tx) p.x--;
  if(p.y<ty) p.y++; else if(p.y>ty) p.y--;
  clamp(p.x,p.y);
}
static inline void chaseBall(P& p){ mvToward(p, ballX, ballY); }
static inline void attackGoal(P& p,bool right){ mvToward(p, right? (W-1):0, H/2 ); }

void giveBall(P& p){ ballFree=false; p.hasBall=true; ballX=p.x; ballY=p.y; }
void dropBall(P& p){ ballFree=true; p.hasBall=false; ballX=p.x; ballY=p.y; }

void checkSteal(P& attacker, P& defender){
  if(attacker.x==defender.x && attacker.y==defender.y && defender.hasBall){
    defender.hasBall=false; giveBall(attacker);
  }
}

void initGame(){
  for(uint8_t i=0;i<3;i++){
    myTeam[i].x=2; myTeam[i].y=2*i+2; myTeam[i].hasBall=false;
    enemyTeam[i].x=W-3; enemyTeam[i].y=2*i+1; enemyTeam[i].hasBall=false;
  }
  ballX=W/2; ballY=H/2; ballFree=true;
  tickCount=0; myRight=true;
}

void drawField(){
  u8x8.clearDisplay();
  for(uint8_t y=0;y<H;y++){
    for(uint8_t x=0;x<W;x++){
      char c=' ';
      if(ballX==x && ballY==y) c='o';
      for(uint8_t i=0;i<3;i++){
        if(myTeam[i].x==x && myTeam[i].y==y) c=myTeam[i].hasBall?'Q':'M';
        if(enemyTeam[i].x==x && enemyTeam[i].y==y) c=enemyTeam[i].hasBall?'q':'E';
      }
      u8x8.setCursor(x,y);
      u8x8.write(c);
    }
  }
  u8x8.setCursor(0,H);
  u8x8.print(scoreA); u8x8.print('-'); u8x8.print(scoreB);
}

void updateTeams(){
  for(uint8_t i=0;i<3;i++){
    P& p=myTeam[i];
    if(p.hasBall){
      if(digitalRead(BTN_SHOOT)==LOW){
        ballX = myRight? (W-1):0; ballY=p.y; dropBall(p);
        if((myRight && ballX==W-1)||(!myRight && ballX==0)){ scoreA++; initGame(); return; }
      } else if(digitalRead(BTN_PASS)==LOW){
        uint8_t mate=(i+1)%3; giveBall(myTeam[mate]); dropBall(p);
      } else { attackGoal(p,myRight); ballX=p.x; ballY=p.y; }
    } else {
      if(ballFree) chaseBall(p); else attackGoal(p,myRight);
    }
  }
  for(uint8_t i=0;i<3;i++){
    P& e=enemyTeam[i];
    if(e.hasBall){
      attackGoal(e,!myRight); ballX=e.x; ballY=e.y;
      if((!myRight && e.x==0)||(myRight && e.x==W-1)){ scoreB++; initGame(); return; }
    } else { if(ballFree) chaseBall(e); else attackGoal(e,!myRight); }
  }
  for(uint8_t i=0;i<3;i++) for(uint8_t j=0;j<3;j++){ checkSteal(myTeam[i],enemyTeam[j]); checkSteal(enemyTeam[j],myTeam[i]); }
}

// --- Penalties ---
void doPenaltyRound(){
  u8x8.clear();
  u8x8.setCursor(0,0); u8x8.print("PENS ");
  u8x8.print(penA); u8x8.print("-"); u8x8.print(penB);

  // my shot
  u8x8.setCursor(0,2); u8x8.print("Your shot!");
  while(digitalRead(BTN_SHOOT)==HIGH); // wait press
  delay(50);
  if((r8()&1)==0){ penA++; u8x8.setCursor(0,3); u8x8.print("GOAL!"); }
  else { u8x8.setCursor(0,3); u8x8.print("MISS!"); }
  delay(800);

  // enemy shot
  u8x8.clear(); u8x8.setCursor(0,0); u8x8.print("Enemy shot!");
  if((r8()&1)==0){ penB++; u8x8.setCursor(0,2); u8x8.print("GOAL!"); }
  else { u8x8.setCursor(0,2); u8x8.print("MISS!"); }
  delay(800);

  penCount++;
}

void setup(){
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  pinMode(BTN_PASS,INPUT_PULLUP);
  pinMode(BTN_SHOOT,INPUT_PULLUP);
  initGame();
}

void loop(){
  if(inPenalties){
    if((penCount<5)||(penA==penB && penCount>=5)){ doPenaltyRound(); }
    else {
      u8x8.clear();
      u8x8.setCursor(0,2);
      u8x8.print("PENS END ");
      u8x8.print(penA); u8x8.print("-"); u8x8.print(penB);
      while(1);
    }
    return;
  }

  updateTeams();
  drawField();
  delay(100);
  tickCount++;
  if(tickCount==GAME_TICKS/2) myRight=!myRight;
  if(tickCount>=GAME_TICKS){
    if(scoreA==scoreB){ inPenalties=true; penA=penB=penCount=0; }
    else {
      u8x8.clear(); u8x8.setCursor(0,2); u8x8.print("FT ");
      u8x8.print(scoreA); u8x8.print("-"); u8x8.print(scoreB);
      while(1);
    }
  }
}
