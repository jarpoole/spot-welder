//We always have to include the library
#include "LedControl.h"
#include <EEPROM.h>

// DataIn=11, CLK=13, LOAD=12, #MAX7219 devices
LedControl lc=LedControl(11,13,12,1);
// We always wait a bit between updates of the display
unsigned long delaytime=250;

//Encoder Stuff
static int pinA = 2; // Our first hardware interrupt pin is digital pin 2
static int pinB = 3; // Our second hardware interrupt pin is digital pin 3
static int encoderButton = 4; // Encoder button pin
volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile uint32_t encoderPos = 0; //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile uint32_t oldEncPos = 0; //stores the last encoder position value so we can compare to the current reading and see if it has changed (so we know when to print to the serial monitor)
volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent
volatile bool lastButtonState = false; //Last value of the encoder button
volatile long lastTickTime = 0; //Used to calculate encoder acceleration

//Control data
static int button1 = A0;
static int button2 = A1;
static int button3 = A2;
static int led1 = A3;
static int led2 = A4;
static int led3 = A5;
static long button1Time = 0;
static long button2Time = 0;
static long button3Time = 0;
static int longPressTime = 2000;
static int blinkTime = 200;

static int eepromPreset1Start = 0; //Each uint32_t occupies 4 bytes
static int eepromPreset2Start = 5;
static int eepromPreset3Start = 9;
volatile uint32_t preset1 = 0;
volatile uint32_t preset2 = 0;
volatile uint32_t preset3 = 0;

volatile uint32_t weldTime = 0;


void setup() {
  pinMode(pinA, INPUT_PULLUP); // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  pinMode(pinB, INPUT_PULLUP); // set pinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  attachInterrupt(0,PinA,RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
  attachInterrupt(1,PinB,RISING); // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)
  Serial.begin(9600); // start the serial monitor link

  pinMode(encoderButton, INPUT_PULLUP);

  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  
  // The MAX7219 is in power-saving mode on startup, we have to do a wakeup call
  lc.shutdown(0,false);
  // Set the brightness to a medium values
  lc.setIntensity(0,8);
  // Clear the display
  lc.clearDisplay(0);
}
void loop() {

  if(!digitalRead(button1) && button1Time == 0){
    button1Time = millis();
  }
  if(!digitalRead(button2) && button2Time == 0){
    button2Time = millis();
  }
  if(!digitalRead(button3) && button3Time == 0){
    button3Time = millis();
  }

  if( digitalRead(button1) ){
    if( button1Time == 0){
      //Do nothing
    }else if(millis() - button1Time > longPressTime){ //save
      for(int i = 0; i < 3; i++){
        digitalWrite(led1, HIGH);
        delay(blinkTime);
        digitalWrite(led1, LOW);
        delay(blinkTime);
      }
    }else{  //recall
        digitalWrite(led1, HIGH);
        digitalWrite(led2, LOW);
        digitalWrite(led3, LOW);
    }
  }
 /*
  //Device#, digit#, value, decimal
  //lc.setDigit(0,0,5,false)

  if(oldEncPos != encoderPos) {
    Serial.println(encoderPos);
    oldEncPos = encoderPos;
  }
  printNum(encoderPos);
  delay(delaytime);

  */
}

void PinA(){
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; // read all eight pin values then strip away all but pinA and pinB's values
  if(reading == B00001100 && aFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos --; //decrement the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00000100) bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  sei(); //restart interrupts
}
void PinB(){
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; //read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    long tickTime = millis() - lastTickTime;
    if(tickTime < 10){
      encoderPos += 1000;
    }else if(tickTime < 100){
      encoderPos += 100;
    }else if(tickTime < 500){
      encoderPos += 10;
    }else{
      encoderPos ++; //increment the encoder's position count
    } 
    lastTickTime = millis();
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
  }
  else if (reading == B00001000) aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
  sei(); //restart interrupts
}

void printNum(uint32_t value){
  lc.clearDisplay(0);
  if(value == 0){
    lc.setDigit(0, 0, 0, false);
  }else{
    int c = 0; 
    while (value != 0){
      lc.setDigit(0, c, value % 10, false);
      value /= 10;
      c++;
    }
  }
}

void savePreset(int presetStartAddr, uint32_t value){
  byte upper = value >> 24;
  byte upMid = (value << 8) >> 24;
  byte lowMid = (value << 16) >> 24;
  byte lower = (value << 24) >> 24;
  
  EEPROM.write(presetStartAddr, upper);
  EEPROM.write(presetStartAddr + 1, upMid);
  EEPROM.write(presetStartAddr + 2, lowMid);
  EEPROM.write(presetStartAddr + 3, lower);
}

uint32_t recallPreset(int presetStartAddr){
  byte upper = EEPROM.read(presetStartAddr);
  byte upMid = EEPROM.read(presetStartAddr + 1);
  byte lowMid = EEPROM.read(presetStartAddr + 2);
  byte lower = EEPROM.read(presetStartAddr + 3);

  uint32_t value = (upper << 24) | (upMid << 16) | (lowMid << 8) | lower;
  return value;
}
