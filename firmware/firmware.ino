//We always have to include the library
#include "LedControl.h"

 //DataIn=11, CLK=13, LOAD=12, #MAX7219 devices
LedControl lc=LedControl(11,13,12,1);

/* we always wait a bit between updates of the display */
unsigned long delaytime=250;

void setup() {
  // The MAX7219 is in power-saving mode on startup, we have to do a wakeup call
  lc.shutdown(0,false);
  // Set the brightness to a medium values
  lc.setIntensity(0,8);
  // Clear the display
  lc.clearDisplay(0);
}
void loop() {
  //Device#, digit#, value, decimal
  lc.setDigit(0,0,5,false);
  delay(1000);
  //delay(delaytime);
  
  //lc.clearDisplay(0);
  
  lc.setChar(0,0,'a',true);
  delay(1000);
  //delay(delaytime);
}
