
#include "LedControl.h"
#include <EEPROM.h>

// DataIn=11, CLK=13, LOAD=12, #MAX7219 devices
LedControl lc=LedControl(11,13,12,1);
//Wait a bit between updates of the display
unsigned long delaytime=250;

//Encoder Stuff
static int pinA = 2; // Our first hardware interrupt pin is digital pin 2
static int pinB = 3; // Our second hardware interrupt pin is digital pin 3
static int encoderButton = 4; // Encoder button pin
volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent
volatile bool lastButtonState = false; //Last value of the encoder button
volatile long lastTickTime = 0; //Used to calculate encoder acceleration

//Control data
static int button1 = A2;
static int button2 = A1;
static int button3 = A0;
static int led1 = A5;
static int led2 = A4;
static int led3 = A3;
static long button1Time = 0;
static long button2Time = 0;
static long button3Time = 0;
static int longPressTime = 2000;
static int triggerPin = 7;
static int ssrPin = 8;
static int buzzerPin = 9;

static int blinkTime = 200;
volatile long blinkTimer = 0;
volatile bool blinking = false;

volatile uint32_t incrementValue = 1;
volatile uint32_t incrementPos = 0;

static int eepromPreset1Start = 5; //Each uint32_t occupies 4 bytes
static int eepromPreset2Start = 9;
static int eepromPreset3Start = 13;
volatile uint32_t preset1 = 0;
volatile uint32_t preset2 = 0;
volatile uint32_t preset3 = 0;

volatile uint32_t weldTime = 0;
volatile long lastWeldTime = 0; //Time in millis that the last weld occured at
static int weldTimeout = 3000;  //Minimum time before next weld

volatile bool refresh = false;


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
  pinMode(triggerPin, INPUT_PULLUP);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ssrPin, OUTPUT);

  preset1 = recallPreset(eepromPreset1Start);
  preset2 = recallPreset(eepromPreset2Start);
  preset3 = recallPreset(eepromPreset3Start);

  Serial.println(preset1);
  Serial.println(preset2);
  Serial.println(preset3);
  
  // The MAX7219 is in power-saving mode on startup, we have to do a wakeup call
  lc.shutdown(0,false);
  // Set the brightness (0-15) to full
  lc.setIntensity(0,15);
  // Clear the display
  lc.clearDisplay(0);
}

void loop() {

  if( !digitalRead(triggerPin) && millis() > (lastWeldTime + weldTimeout) ){
    //Timer setup
    int milliseconds = weldTime / 1000;
    int microseconds = weldTime % 1000;

    Serial.println(weldTime);
    
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(ssrPin, HIGH);

    if(milliseconds){
      delay(milliseconds);
    }
    if(microseconds){
      delayMicroseconds(microseconds);
    }
    
    digitalWrite(ssrPin, LOW);
    digitalWrite(buzzerPin, LOW);
    lastWeldTime = millis();
  }

  //Timer for blinking decimal point selection indicator
  if(millis() > blinkTimer){
    blinkTimer = millis() + blinkTime;
    blinking = !blinking;
    refresh = true;
  }

  if(!digitalRead(encoderButton)){
    if(incrementValue > 1000000 || incrementValue == 0){
      incrementValue = 1;
      incrementPos = 0;
    }else{
      incrementValue = incrementValue * 10;
      incrementPos++;
    }
    refresh = true;
    delay(250);
  }

  if(!digitalRead(button1) && button1Time == 0){
    button1Time = millis();
    refresh = true;
  }
  if(!digitalRead(button2) && button2Time == 0){
    button2Time = millis();
    refresh = true;
  }
  if(!digitalRead(button3) && button3Time == 0){
    button3Time = millis();
    refresh = true;
  }

  if( digitalRead(button1) ){
    if( button1Time == 0){
      //Do nothing
    }else if(millis() - button1Time > longPressTime){ //save
      button1Time = 0;
      savePreset(eepromPreset1Start, weldTime);
      preset1 = weldTime;
      for(int i = 0; i < 3; i++){
        digitalWrite(led1, HIGH);
        delay(blinkTime);
        digitalWrite(led1, LOW);
        delay(blinkTime);
      }
      refresh = true;
    }else{  //recall
      weldTime = preset1;
      button1Time = 0;
      digitalWrite(led1, HIGH);
      digitalWrite(led2, LOW);
      digitalWrite(led3, LOW);
      refresh = true;
    }
  }

  if( digitalRead(button2) ){
    if( button2Time == 0){
      //Do nothing
    }else if(millis() - button2Time > longPressTime){ //save
      button2Time = 0;
      savePreset(eepromPreset2Start, weldTime);
      preset2 = weldTime;
      for(int i = 0; i < 3; i++){
        digitalWrite(led2, HIGH);
        delay(blinkTime);
        digitalWrite(led2, LOW);
        delay(blinkTime);
      }
      refresh = true;
    }else{  //recall
      weldTime = preset2;
      button2Time = 0;
      digitalWrite(led1, LOW);
      digitalWrite(led2, HIGH);
      digitalWrite(led3, LOW);
      refresh = true;
    }
  }

  if( digitalRead(button3) ){
    if( button3Time == 0){
      //Do nothing
    }else if(millis() - button3Time > longPressTime){ //save
      button3Time = 0;
      savePreset(eepromPreset3Start, weldTime);
      preset3 = weldTime;
      for(int i = 0; i < 3; i++){
        digitalWrite(led3, HIGH);
        delay(blinkTime);
        digitalWrite(led3, LOW);
        delay(blinkTime);
      }
      refresh = true;
    }else{  //recall
       weldTime = preset3;
       button3Time = 0;
       digitalWrite(led1, LOW);
       digitalWrite(led2, LOW);
       digitalWrite(led3, HIGH);
       refresh = true;
    }
  }

  if(refresh) {
    //Serial.println(weldTime);
    updateDisplay(weldTime, blinking, incrementPos);
    refresh = false;
  }
  
  
  //delay(delaytime);

}

void PinA(){
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; // read all eight pin values then strip away all but pinA and pinB's values
  if(reading == B00001100 && aFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    weldTime += incrementValue; //increment the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
  }
  else if (reading == B00000100) bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  refresh = true;
  sei(); //restart interrupts
}
void PinB(){
  cli(); //stop interrupts happening before we read pin values
  reading = PIND & 0xC; //read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge

    if(weldTime - incrementValue < weldTime){ //Detect integer underflow
      weldTime = weldTime - incrementValue;  //decrement the encoder's position count
    }else{
      weldTime = 0;
    }
    
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    digitalWrite(led3, LOW);
  }
  else if (reading == B00001000) aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
  refresh = true;
  sei(); //restart interrupts
}

//Device#, digit#, value, decimal
//lc.setDigit(0,0,5,false)
void updateDisplay(uint32_t value, bool blinking, int blinkingPos){
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

  //Device#, row#, col#, decimal
  lc.setLed(0, blinkingPos, 0, blinking);
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
  
  Serial.print("Saved value: ");
  Serial.print(value);
  Serial.print("\n");
}

uint32_t recallPreset(int presetStartAddr){
  byte upper = EEPROM.read(presetStartAddr);
  byte upMid = EEPROM.read(presetStartAddr + 1);
  byte lowMid = EEPROM.read(presetStartAddr + 2);
  byte lower = EEPROM.read(presetStartAddr + 3);

  uint32_t value = ((uint32_t) upper << 24) | ((uint32_t) upMid << 16) | ((uint32_t) lowMid << 8) | ((uint32_t) lower);

  Serial.print("Read value: ");
  Serial.print(value);
  Serial.print("\n");
  
  return value;
}
