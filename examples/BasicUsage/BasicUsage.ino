
#include <Arduino.h>
#include "M590Driver.h"

M590 modem(Serial);

void setup() {

  modem.begin();

  modem.powerOn();
  while (modem.isBusy()) modem.process();

  modem.waitReady();
  while (modem.isBusy()) modem.process();
}

void loop() {

  // The driver's asynchronous process is being run in a loop
  modem.process();

  // Modem is processing command - waiting...
  if (modem.isBusy()) return;

  //We received an SMS - we're sending a confirmation back 
  if (modem.isIncomingSMS()) {

    const char* number = modem.getSMSNumber();
    const char* text   = modem.getSMSText();

    modem.sendSMS_RAM_P(number,(PGM_P)F("Message received"));

    modem.clearIncomingSMS();

    return;   // <= This ensures that only one command is executed in each cycle
  }
  
}
