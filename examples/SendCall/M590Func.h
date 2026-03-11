/*
 * M590Func, version 1.00. 
 *
 * Library for M590Driver.
 *  
 *  use example: 
 *  
 *  M590 modem(Serial);
 *  M590FUNC modemFunc(&modem);
 *  
 *  void setup() {
 *    int init = modemFunc.fullInit();
 *    if (init < 1) { ... }   // error
 *  }
*/ 

class M590FUNC {

  public:
  M590 *modem;
  
  M590FUNC::M590FUNC(void *m) {
    modem = m;
  }

  // Send command to modem and compare return in string
  // cmd - send command, 
  // ret - string compare,
  // mode: 0 - strict comparison, 1 - search substring, ignore case
  // timeout: 0 - default value (15sec), else time in ms (1000 = 1sec)
  bool sendAndCompare(const __FlashStringHelper* cmd,
                           const __FlashStringHelper* ret,
                           uint8_t compare_mode = 0,
                           uint16_t timeout = 0);

  // Modem test registred function
  // return true if modem is registred in network, else return false
  bool isRegistred(); 

  // Modem full init function
  // return:  >0 = ok, 
  //           0 = modem init error, 
  //          -1 = SIM not ready (empty or protected), 
  //          -2 = SIM not registered in network
  int fullInit() ;
  
};

// --------------- Realisation ------------------

bool M590FUNC::sendAndCompare(const __FlashStringHelper* cmd,
                         const __FlashStringHelper* ret,
                         uint8_t compare_mode = 0,
                         uint16_t timeout = 0)
{   
  if (!timeout) {
    if (!modem->sendCmd_P((PGM_P)cmd)) return false;
  } else {
    if (!modem->sendCmd_P((PGM_P)cmd, timeout)) return false;
  }
   
  while (modem->isBusy()) { 
    modem->process();
    if (modem->getStatus() == M590_UNKNOWN) break;
  }

  M590Status status = modem->getStatus();

  modem->resetProcess();
    
  if (status != M590_OK && status != M590_UNKNOWN)
     return false;

  const char* buf = modem->getUartBuf();
  if (!buf) return false;

  if (compare_mode) {
    // search substring, no case mode
    return (strcasestr_P(buf, (PGM_P)ret) != nullptr);
  } else {
    // full compare
    return (strcmp_P(buf, (PGM_P)ret) == 0);
  }
}

bool M590FUNC::isRegistred() {

unsigned long tReg = millis();
bool registered = false;

while (millis() - tReg < 60000) {   // 60 sec timeout
  
  if (ModemSendAndCompare(F("AT+CREG?"), F("+CREG:"), 1, 10000)) { 
    
    const char* buf = modem->getUartBuf();
    if (buf[0]) {
      if (strstr(buf, "0,1") || strstr(buf, "0,5")) {
        registered = true;
        break;
      }
    }
  }
  delay(1000);   // pause
}

return registered;
}

int M590FUNC::fullInit() {

  if (!modem->begin()) return 0; 
  modem->setAutoInit(true);
  if (!modem->powerOn()) return 0;
  while (modem->isBusy()) modem->process();
  if (modem->getStatus() != M590_OK) return 0;
  if (!modem->waitReady()) return 0;
  while (modem->isBusy()) modem->process();
  if (modem->getStatus() != M590Status::M590_READY) return 0;

  if (!ModemSendAndCompare(F("AT+CPIN?"), F("READY"), 1))
    return -1;

  if (!ModemIsRegistred())
    return -2;

  return 1;
}


//========================= END =========================
