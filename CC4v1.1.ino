#include "Melody.h"
#include "TM1637.h"
#include <TM1637Display.h>
int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};

////////////////////////////////////////////
//to do
//modify reset timeer from 6-10 sec to something else. this is confusing
//now unable to access time setting menu?
///////////////////////////////////////////

#define CLK_P1 5
#define DIO_P1 6
#define CLK_P2 7
#define DIO_P2 8

const int buzzer = 9; //buzzer
//inputs
const int BtnMenu = 2;       //pause/reset/menu/sound off_on
const int BtnP1 = 3;      //player1 pin4
const int BtnP2 = 4;      //player2 pin5

//LED
int ledPinMenu = 10;  //middle
int ledPinP1 = 11; //p1
int ledPinP2 = 12; //p2

int BtnState = HIGH;
//(LOW is pressed b/c i'm using the pullup resistors)

unsigned long previousMillis = 0;// store last time led was updated

long interval = 500;// interv to blink
int ledState = LOW;

long millis_held_btnMenu;   
long secs_held_btnMenu;       
long prev_secs_held_btnMenu;  
byte previous_btnMenu = LOW;
unsigned long firstTime_btnMenu; // how long since the button was first pressed

int prevActSecRem;  // timer switching

bool isMenu = false;  // within in the time-set menu
bool isGameOver = false;
bool isSound = true;
int setPlayerTimer = 1;

class Player
{
  public:
    int minutes;
    String menuText; 
    String playIndicator; //indicates player being active
    long secondsRun; //how long player was active

    void IncrementMinutes(){
      minutes = min(minutes, 97);
      minutes = minutes + 2;
    }
    void DecrementMinutes(){
      minutes = max(minutes, 2);
      minutes--;
    }

    int SecondsRemaining()
    {
      // sec remaining on player clock
      int minsAllowed = minutes;
      int secondsRunSoFar = secondsRun;
      return minsAllowed * 60 - secondsRunSoFar;
    }

    void UpdateCounter(Player p0, Player p1, Player p2){
      secondsRun += millis() / 1000 - (p0.secondsRun + p1.secondsRun + p2.secondsRun);
    }
};

// init players
Player p0 = {0, "P0", "0", 0}; // dummy player, active when player clocks are not counting down
Player p1 = {15, "P1", "1", 0};
Player p2 = {15, "P2", "2", 0};

Player* activePlayer = &p0;

void PlayerIncDecMin(const Player& pp, bool isInc);
void displayOn7Seg(Player player,TM1637 ShowOn);

TM1637 P1Display(CLK_P1, DIO_P1);
TM1637 P2Display(CLK_P2, DIO_P2);


void setup()
{
  pinMode(ledPinMenu, OUTPUT);
  pinMode(ledPinP1, OUTPUT);
  pinMode(ledPinP2, OUTPUT);


  // Turn on 20k pullup resistors to simplify switch input
  pinMode(BtnMenu, INPUT_PULLUP);
  pinMode(BtnP1, INPUT_PULLUP);
  pinMode(BtnP2, INPUT_PULLUP);
   // Segments 

    P1Display.set(BRIGHT_TYPICAL);
    P1Display.init();
  P1Display.point(POINT_ON);
  displayOn7Seg(p1,P1Display);
  
  P2Display.set(BRIGHT_TYPICAL);
  P2Display.init();
  P2Display.point(POINT_ON);
  displayOn7Seg(p2,P2Display);
  
  //setup done
  LEDsOnOff(HIGH);
  tone(buzzer, 1000); // Send 1KHz sound signal...
  delay(500);        // ...for 0.5 sec
  noTone(buzzer);     // Stop sound...
  LEDsOnOff(LOW);

  
  
}


void loop()
{
  prevActSecRem = activePlayer->SecondsRemaining(); // prev active player seconds
  activePlayer->UpdateCounter(p0, p1, p2);;  

  unsigned long currentMillis = millis();
  if (activePlayer->menuText=="P0") Blink(currentMillis, ledPinMenu);

  //menu button
  BtnState   = digitalRead(BtnMenu);
  // if button == pressed, remember the start time
  if ((BtnState == LOW && previous_btnMenu == HIGH && (millis() - firstTime_btnMenu) > 100)){
    firstTime_btnMenu = millis();
  }
  millis_held_btnMenu = (millis() - firstTime_btnMenu);
  secs_held_btnMenu = millis_held_btnMenu / 1000;

  

  if (millis_held_btnMenu > 50)
  // valid push must be > 50ms
  {
    if (BtnState == LOW && secs_held_btnMenu > prev_secs_held_btnMenu)
    {
      ledblink(1, 50, ledPinMenu); // Each second the button is held blink the indicator led
    }

    if (BtnState == HIGH && previous_btnMenu == LOW)
  // check btn was released
    {

      if (secs_held_btnMenu <= 0)
    //pause
      {
    if (isSound) Beep(1000, 50);
    activePlayer = &p0;
      }
      
      if (secs_held_btnMenu >= 2 && secs_held_btnMenu < 4)

      {
        if (isMenu == true) {
          isMenu = false;
        }
        else {
          isMenu = true;
        }
      }
     
      if (secs_held_btnMenu >= 4 && secs_held_btnMenu < 6 )
     //disable sound
      {
        if (isSound == true) {
          isSound = false;
        }
        else {
          isSound = true;
        }
      }
       
      if (secs_held_btnMenu >= 6 && secs_held_btnMenu < 10)
    // reset if held between 6-10 sec
      {
        ledblink(4, 300, ledPinMenu);
        resetGame();
      }
      
      if (secs_held_btnMenu >= 10)
    // reset
      {
        ledblink(10, 200, ledPinMenu);
        MasterResetGame();
        Beep(920, 200);
        Beep(940, 200);
        Beep(960, 200);
      }
      // ===============================================================================
    }
    previous_btnMenu = BtnState;
    prev_secs_held_btnMenu = secs_held_btnMenu;
  }
  
  if (activePlayer->menuText =="P0")

  {
    Blink(currentMillis, ledPinMenu);
  }
  
  if (isMenu == true)
  // menu timer setting
  {
    resetGame();
    LEDsOnOff(LOW);
    while (setPlayerTimer <= 2) //until p2 finishes setting timer
    {
      delay(100);
      switch (setPlayerTimer)
      {
        case 1:
    //p1timer set
          {
            if (!digitalRead(BtnP1)) PlayerIncDecMin(p1, true); //true=increment, false=decrement
            if (!digitalRead(BtnP2)) PlayerIncDecMin(p1, false);
        displayOn7Seg(p1,P1Display);
        displayOn7Seg(p2,P2Display);
          } break;
        
        case 2:
    //p2timer set
          {
            if (!digitalRead(BtnP1)) PlayerIncDecMin(p2, true);
            if (!digitalRead(BtnP2)) PlayerIncDecMin(p2, false);
        displayOn7Seg(p1,P1Display);
        displayOn7Seg(p2,P2Display);
          } break;
      }
    
      if (!digitalRead(BtnMenu))

      {
        setPlayerTimer++;
        //Beep(1000, 50);
        if (setPlayerTimer > 2){
          isMenu = false;
          setPlayerTimer = 1;
          break;
        }
      }
    }
  }
  
  else
  //check loser else switch players
  {
    int Loser;
    Loser = getLoserNumber();
    if (Loser > 0){
      if (isGameOver == false)
    //only when game is over print 1x
      {
    displayOn7Seg(p1,P1Display);
    displayOn7Seg(p2,P2Display);
    
        if (isSound){
          if (Loser == 1) sing(3);
          if (Loser == 2) sing(4);
        }
      }
    
      isGameOver = true;
      Blink(currentMillis, ledPinMenu);

      if (!digitalRead(BtnMenu)) 
    //show result till btn1 pressed --> reset game
      {
        Serial.println("Reset Game");
        LEDsOnOff(LOW);
        resetGame();
        Blink(currentMillis, ledPinMenu);
    displayOn7Seg(p1,P1Display);
    displayOn7Seg(p2,P2Display);
      }
    }
    else 
  //game not finnished
    {
      //p1 pressed
      if (!digitalRead(BtnP1)){
      if (activePlayer->menuText !="P2"){
      digitalWrite(ledPinMenu, LOW);
      digitalWrite(ledPinP1, LOW);
      digitalWrite(ledPinP2, HIGH);
       Beep(100, 100);
      activePlayer = &p2;
      }
      }
      //p2 pressed
      if (!digitalRead(BtnP2)){
       if (activePlayer->menuText !="P1"){
      digitalWrite(ledPinMenu, LOW);
      digitalWrite(ledPinP2, LOW);
      digitalWrite(ledPinP1, HIGH);
     Beep(940, 100);
      activePlayer = &p1;
       }
      }

      if (activePlayer->menuText != "P0")
    //stop refreshing if p0 active
    {
      //only print on p1/p2 change
      if (activePlayer->SecondsRemaining() != prevActSecRem){
      displayOn7Seg(p1,P1Display);
      displayOn7Seg(p2,P2Display);

      }
    }
    }
  }
}

void ledblink(int times, int lengthms, int pinnum)
// blink led test
{
  for (int x = 0; x < times; x++){
    digitalWrite(pinnum, HIGH);
    delay (lengthms);
    digitalWrite(pinnum, LOW);
    delay(lengthms);
  }
}

void Beep(int khz, int duration)
//beep test
{
  if (isSound){
    tone(buzzer, khz); 
    delay(duration);  
    noTone(buzzer);  
  }
}

void resetGame()
// reset game
{
  p0.secondsRun += (p1.secondsRun + p2.secondsRun);
  p1.secondsRun = 0;
  p2.secondsRun = 0;
  activePlayer = &p0;
  isGameOver = false;
  setPlayerTimer = 1;
  LEDsOnOff(LOW);
  displayOn7Seg(p1,P1Display);
  displayOn7Seg(p2,P2Display);
}

void MasterResetGame()
// reset game 5m
{
  p0.secondsRun += (p1.secondsRun + p2.secondsRun);
  p1.secondsRun = 0;
  p2.secondsRun = 0;
  p1.minutes = 15;
  p2.minutes = 15;
  activePlayer = &p0;
  isGameOver = false;
  setPlayerTimer = 1;
  LEDsOnOff(LOW);
  displayOn7Seg(p1,P1Display);
  displayOn7Seg(p2,P2Display);
}

int getLoserNumber()
// return the number of the losing player, or 0 if both have time remaining
{
  if (p1.SecondsRemaining() <= 0)
  {
    return 1;
  } else if (p2.SecondsRemaining() <= 0)
  {
    return 2;
  }
  return 0;
}

void LEDsOnOff(int OnOff)
{
  digitalWrite(ledPinMenu, OnOff);
  digitalWrite(ledPinP1, OnOff);
  digitalWrite(ledPinP2, OnOff);
}

void PrintTimers(int P1s, int P2s)
{ 
  Serial.print("P1  :  P2");
  Serial.println("  ");
  Serial.print(P1s);
  Serial.print("   :  ");
  Serial.print(P2s);
  Serial.println(" ");
  Serial.println("===========");
}

void Blink(unsigned long currM, int ledPin)
{ 
  if (currM - previousMillis >= interval) {
    previousMillis = currM;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(ledPin, ledState);
  }
}

void PlayerIncDecMin(const Player& pp, bool isInc)
{
  if(isInc == true){
    if (isSound)Beep(440, 100);
    pp.IncrementMinutes();
    ledblink(1, 50, ledPinMenu);
  }
  else{
      if (isSound)Beep(392, 100);
    pp.DecrementMinutes();
    ledblink(1, 50, ledPinMenu);
  }
  
  
}

void displayOn7Seg(Player player,TM1637 ShowOn)
{
  int seconds=(player.minutes*60)-player.secondsRun;
  
  int minutes=seconds/60;
  if(seconds%60==0) seconds=0;
    
  TimeDisp[0] = minutes / 10; 
  TimeDisp[1] = minutes % 10;
  TimeDisp[2] = (seconds % 60) / 10;
  TimeDisp[3] = (seconds % 60) % 10;;
  
    ShowOn.display(TimeDisp);
}
