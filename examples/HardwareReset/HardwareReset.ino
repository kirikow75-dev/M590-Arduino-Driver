
#include <Arduino.h>
#include "M590Driver.h"

M590 modem(Serial);

void hungup() {

    modem.sendCmd("ATH");
    while(modem.isBusy()) modem.process();
}

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

  // Modem is not ready - waiting...
  if (!modem.isReady()) return;
  
  // Modem is processing command - waiting...
  if (modem.isBusy()) return;

  /// Call received - run hard reset
  if (modem.isIncomingCall()) {

    modem.clearIncomingCall();

    hungup(); // <= We are terminating the current connection -
              // this action may not occur during a reboot
              
    modem.hardReset();  // <= Not mandatory, but preferred if the reboot is unsuccessful

    return;   // <= This ensures that only one command is executed in each cycle
  }
  
}


