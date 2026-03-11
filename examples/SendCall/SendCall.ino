
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

  // We received a message and are calling back the incoming number
  if (modem.isIncomingSMS()) {

    const char* number = modem.getSMSNumber();

    modem.sendCall(number);

    modem.clearIncomingSMS();

    return;
  }
}


