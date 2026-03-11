/*
 * In some cases, when a more reliable method of initializing 
 * the modem is required, you can use the `M590Func` library, 
 * which contains additional functions for working with the 
 * modem using the `M590Driver` driver.
 * 
 */

#include <Arduino.h>
#include <M590Driver.h>
#include <M590Func.h>

M590 modem(Serial);

M590FUNC modemFunc(&modem); 

void blinkLed() {   // blinking the LED asynchronously

  static uint32_t timer_blink = 0; 
  static bool level = true;
  if (!timer_blink) pinMode(LED_BUILTIN, OUTPUT);
  if (millis() - timer_blink < 500) return;
  timer_blink = millis();
  digitalWrite(LED_BUILTIN, level);
  level = !level;  
    
}

void setup() {

  modemFunc.fullInit(); 
  
}

void loop() {

  modem.process();

  blinkLed();
  
}

