
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

  static bool flagSend = false;
  static char* number = nullptr;
  static String info = "";

  // The driver's asynchronous process is being run in a loop
  modem.process();

  // Concatenate multiple modem response lines into one.
  if (flagSend && modem.isUartBuf()) {
    String add = String(modem.getUartBuf());
    add.trim();
    if (add != "OK") info += add + " ";
  }

  // Modem is processing command - waiting...
  if (modem.isBusy()) return;

  // A new call has arrived and the modem is free.
  // We are requesting the device name and setting 
  // the SMS sending flag.
  if (!flagSend && modem.isIncomingCall()) {

    number = modem.getCallerNumber();

    flagSend = true;

    info = "";

    modem.sendCmd("ATI");

    modem.clearIncomingCall();

    return;   // <= This ensures that only one command is executed in each cycle

  }

  // Transmitting the data and resetting the logic.
  if (flagSend) {
    
    modem.sendSMS(number,info.c_str());
    
    modem.clearIncomingCall();

    flagSend = false;

    return;
  }

}

//===========  END ============
