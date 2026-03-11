
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

  // Call received, sending back caller's number via message
  if (modem.isIncomingCall()) {

    const char* number = modem.getCallerNumber();

    const char text[36];
    sprintf(text, "Your phone number: %s", number);

    modem.sendSMS(number, text);

    modem.clearIncomingCall();

    return;
  }
}
