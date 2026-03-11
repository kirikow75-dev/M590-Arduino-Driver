/*
 * M590 Arduino Driver, version 1.01, 03/03/2026
 *
 *  ------------------------------------------------------------
 * | Open-source driver for the Neoway M590 GSM module.         |
 * |                                                            |
 * | A professional version with extended reliability and       |
 * | diagnostics features is available for commercial projects. | 
 *  ------------------------------------------------------------
 *
 */

#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <fast_gpio.h>

/* ===== Modem control pins ===== */
#define M590_POWER_PIN 8             	// POWER ON/OFF. 
#define M590_POWER_PIN_ACTIVE LOW    	// Active level

#define M590_EMERGENCY_PIN 9            // EMERGENCY OFF pin (reset pin). 
#define M590_EMERGENCY_PIN_ACTIVE HIGH  // Comment if not used 

#define M590_SLEEP_PIN   12				// SLEEP pin. Comment if not used
#define M590_SLEEP_PIN_ACTIVE   LOW    	// SLEEP pin. Comment if not used

/* === Macros === */
#if defined(M590_EMERGENCY_PIN) && (M590_EMERGENCY_PIN >= 0)
  #define IS_M590_EMERGENCY_PIN 1
#else
  #define IS_M590_EMERGENCY_PIN 0
#endif

#if defined(M590_SLEEP_PIN) && (M590_SLEEP_PIN >= 0)
  #define IS_M590_SLEEP_PIN 1
#else
  #define IS_M590_SLEEP_PIN 0
#endif

/* ===== Configuration constants ===== */
#define M590_WAIT_READY_TIMEOUT       30000
#define M590_WAIT_CALL_TIMEOUT        60000
#define M590_SMS_TEXT_BUFFER_SIZE     128                
#define M590_UART_BUFFER_SIZE         128
#define M590_AUTO_SLEEP_MS            3000    // No set DTR pin only!
#define M590_CMD_TIMEOUT_DEFAULT      15000

/* ===== The result of the command execution ===== */
enum M590Status : uint8_t {
     M590_NONE = 0,
     M590_OK,
     M590_UNKNOWN,
     M590_ERROR,
     M590_TIMEOUT,
     M590_BUSY,
     M590_NO_CARRIER,
     M590_NO_ANSWER,
     M590_CONNECT,
     M590_READY
};

class M590 {
  
public:   // The public section of the class
  
    /* ===== FSM ===== */
    enum State : uint8_t {
        MS_OFF = 0,
        MS_IDLE,        
        MS_POWER,
        MS_HARDRESET,
        MS_INIT,
        MS_WAIT_READY,
        MS_SLEEP,
        MS_COMMAND,
        MS_SMS_CMGS,
        MS_CALL
    } state;

    /* ===== General vars ===== */   
    M590Status status;

    /* ===== Initialization ===== */
    M590(HardwareSerial& s);    
    bool begin(uint32_t baud = 9600);

    /* ===== Parametrs ===== */
    void setAutoInit(bool value) { autoInitFlag = value; }
    bool getAutoInit(void) { return autoInitFlag; }
        
    /* ===== Asynchronous command handler (FSM) ===== */
    void process(void);

    /* ===== Modem status check functions ===== */
    bool isBegin(void) const { return beginFlag; }
    bool isBusy(void) const { return state != MS_IDLE && state != MS_OFF; }
    bool isOn(void) const { return powerFlag; }
    bool isOff(void) const { return !powerFlag; }
    bool isInitProcess(void) const { return state == MS_INIT || (autoInitFlag && (state == MS_POWER || state == MS_HARDRESET)); }
    bool isInitOk(void) const { return initFlag; }
    bool isReady(void) { return readyFlag; }
    bool isSleepEnabled(void) const { return sleepEnabledFlag; }
    bool isSleep() const { return sleepFlag; }    // software flag !

    /* ===== Modem management functions ===== */
    bool powerOn();
    bool powerOff();
    bool init();
    bool waitReady();
    bool hardReset();
    bool softReset(); 			 // <= use it ONLY after successful initialization!
    bool sleepMode(bool on);
    bool resetProcess(void); 

    /* ===== Functions for sending messages, calls, and commands ===== */
    bool sendSMS(const char* number, const char* text);
    bool sendSMS_P(PGM_P number, PGM_P text);
    bool sendSMS_RAM_P(const char* number, PGM_P text);
    bool sendCall(const char* number);
    bool sendCall_P(PGM_P number);
    bool sendCmd(const char* cmd, uint16_t timeout);
    bool sendCmd_P(PGM_P cmd, uint16_t timeout);

    /* ===== Functions of incoming events SMS/Call ===== */
    bool isIncomingCall() const { return incomingCall && incomingCallNumberReady; }
    const char* getCallerNumber() const { return callerNumber; }
    void clearIncomingCall() { incomingCall = incomingCallNumberReady = false; }

    bool isIncomingSMS() const { return incomingSMSall; }
    const char* getSMSNumber() const { return smsNumber; }
    const char* getSMSText() const { return smsText; }
    void clearIncomingSMS() { incomingSMS = incomingSMSall = false; }   

    /* ===== Additional functions ===== */
    M590Status getStatus() { return status; }
    void clearStatus() { status = M590_NONE; }
    void setInitFlag(bool flag) { initFlag = flag; }

    bool isUartBuf() const { return uartBufFlag; }
    const char* getUartBuf() const { return uartBuf; } 
    // Returns a pointer to the last received UART frame. 
    // IMPORTANT: the buffer is valid until the next process() call.
    // to check the correctness of the content, use isUartBuf()
    // Can be used in conjunction with `if (modem.GetStatus() == M590_UNKNOWN)...`
    
private:    // Private classroom section

    HardwareSerial& serial;

    enum InitStep : uint8_t {
        INIT_START = 0,
        INIT_ATE0,
        INIT_CMGF,
        INIT_CLIP,
        INIT_CSCS,
        INIT_CNMI,
        INIT_END
    };       
    InitStep initStep;
    inline void initNextStep() { initStep = InitStep(initStep + 1); }

    enum HardresetStep : uint8_t {
        HARDRESET_START = 0,
        HARDRESET_WAIT,
        HARDRESET_END
    };
    HardresetStep hardresetStep;
    inline void hardresetNextStep() { hardresetStep = HardresetStep(hardresetStep + 1); }

    enum PowerStep : uint8_t {
        POWER_ON_START = 0,
        POWER_ON_STEP_1,
        POWER_ON_STEP_2,
        POWER_ON_STEP_3,
        POWER_ON_END,
        POWER_OFF_START,
        POWER_OFF_STEP_1,
        POWER_OFF_END
    };
    PowerStep powerStep;
    inline void powerNextStep() { powerStep = PowerStep(powerStep + 1); }
    
    enum SleepStep : uint8_t {
        SLEEP_ON = 0,
        SLEEP_ON_1,
        SLEEP_ON_END,
        SLEEP_OFF,
        SLEEP_OFF_1,
        SLEEP_OFF_END
    };
    SleepStep sleepStep;
    inline void sleepNextStep() { sleepStep = SleepStep(sleepStep + 1); }

    enum SendSmsStep : uint8_t {
        SENDSMS_START = 0,
        SENDSMS_SENDING,
        SENDSMS_WAIT,        
        SENDSMS_END
    };
    SendSmsStep sendSmsStep;
    inline void sendSmsNextStep() { sendSmsStep = SendSmsStep(sendSmsStep + 1); }

    /* ===== Status flags ===== */
    bool autoInitFlag;
    bool powerFlag;
    bool beginFlag;
    bool initFlag;
    bool readyFlag;
    bool sleepEnabledFlag;
    bool sleepFlag;
    bool wake_flag;
    bool hardresetPowerFlag;
    bool uartBufFlag;

    /* ===== Operating variables ===== */
    uint32_t sleepTimer;
    uint8_t powerAttempts;
    uint8_t initAttempts;
    uint16_t cmdTimeout;
    char uartBuf[M590_UART_BUFFER_SIZE];
    uint8_t idx;
    uint32_t cmdTimerMs;
    const char* ram_num;
    const char* ram_txt;
    PGM_P pgm_num;
    PGM_P pgm_txt;

    /* ===== Incoming SMS/Call ===== */
    bool incomingCall;      // событие: поступил входящий вызов
    bool incomingCallNumberReady; // событие: поступил номер входящего звонка
    bool incomingSMS;       // поступило входящее сообщение
    bool incomingSMSall;    // входящее сообщение обработано (получены номер, текст)
    
    char callerNumber[16];  // буфер номера входящего звонка
    char smsNumber[16];     // буфер номера входящего сообщения
    char smsText[M590_SMS_TEXT_BUFFER_SIZE];  // буфер текста входящего сообщения

    /* ===== internal working functions ===== */
    uint32_t getMillis();
    void insertPlus(char *str, size_t sizebuf);
    void initVars(uint8_t mode = 0);
    void clearRxBuf(uint8_t mode = 0x07);
    bool processReadUART(void);
    void processPower(void);
    void processHardReset(void);
    void processInit(void);
    void processCall(void);
    void processSendSMS(void);
    void processSleep(void);
    void processAutoSleep(void);
    void processAnswer(void);
    void processWaitReady(void);
    void parseNumber(const char *s, char *out, uint8_t outSize);
    void sendStr_RAM(const char* s);
    void sendStr_P(PGM_P s);
    void wake(void);
};

// =================== Realization ===================

M590::M590(HardwareSerial& s) : serial(s) 
{
    pinModeFast(M590_POWER_PIN, OUTPUT); 
    digitalWriteFast(M590_POWER_PIN, !M590_POWER_PIN_ACTIVE);

    #if IS_M590_EMERGENCY_PIN
      pinModeFast(M590_EMERGENCY_PIN, OUTPUT);
      digitalWriteFast(M590_EMERGENCY_PIN, !M590_EMERGENCY_PIN_ACTIVE);
    #endif
    
    #if IS_M590_SLEEP_PIN
      pinModeFast(M590_SLEEP_PIN, OUTPUT);
      digitalWriteFast(M590_SLEEP_PIN, !M590_SLEEP_PIN_ACTIVE); 
    #endif    
  
    autoInitFlag = true;
    powerFlag = false;    
    beginFlag = false;
    hardresetPowerFlag = false;
    
    initVars(0);
}

uint32_t M590::getMillis() {
  uint32_t val = millis();
  if (val == 0) val = 1;
  return val;
}

void M590::insertPlus(char* str, size_t sizebuf) {
  if (!str || str[0] == 0 || str[0] == '+') return;
  if (str[0] < '0' || str[0] > '9') return;  

  size_t len = strlen(str);

  if (len >= sizebuf) len = sizebuf - 1;  
  if (len >= sizebuf - 1) len = sizebuf - 2; 

  memmove(str + 1, str, len);
  str[0] = '+';
  str[len + 1] = 0;
}

void M590::initVars(uint8_t mode) { // mode: 0 - hardware reset, 1 - software reset

    if (mode == 0) {
      initFlag = false;
      readyFlag = false;
      sleepEnabledFlag = false;
      sleepFlag = false;
      wake_flag = false;
      state = MS_OFF;
    } else {
      state = MS_IDLE;
    }

    status = M590_NONE;

    initStep = 0;
    powerStep = 0;
    sleepStep = 0;
    hardresetStep = 0;

    sleepTimer = 0;
    initAttempts = 0;
    powerAttempts = 0;       

    cmdTimerMs = 0;  
    cmdTimeout = 0;
    ram_num = nullptr;
    ram_txt = nullptr; 
    pgm_num = nullptr;
    pgm_txt = nullptr; 
    callerNumber[0] = 0; 
    smsNumber[0] = 0; 
    smsText[0] = 0;
    incomingCall = false;
    incomingCallNumberReady = false;    
    incomingSMS = false;
    incomingSMSall = false;
    clearRxBuf(0x07);
}

void M590::clearRxBuf(uint8_t mode) {
    if (mode & 0x01) while (serial.available()) serial.read();
    if (mode & 0x02) idx = 0;
    if (mode & 0x04) { uartBuf[0] = 0; uartBufFlag = false; }
}

bool M590::begin(uint32_t baud) {
    if (beginFlag) return false;
    serial.begin(baud);    
    beginFlag = true;
    return true;
}

bool M590::powerOn() {
    if (!isBegin() || isBusy()) return false;

    initVars(0);
    state = MS_POWER;
    powerStep = POWER_ON_START;
    return true;
}

bool M590::powerOff() {
    if (!isBegin() || isBusy()) return false;

    initVars(0);
    state = MS_POWER;
    powerStep = POWER_OFF_START;
    return true;
}

bool M590::init(void) {
    if (!isBegin() || isOff() || isBusy()) return false;
    
    state = MS_INIT;
    initStep = INIT_START;
    return true;
}

bool M590::resetProcess() { 

    if (!isBegin() || isOff()) return false;
    state = MS_IDLE;
    status = M590_NONE;
    cmdTimerMs = 0;
    return true;
}

bool M590::softReset(void) {

    if (!isBegin() || isOff()) return false;

    initVars(1);

    return true;
}

bool M590::hardReset() {
    if (!isBegin() || isOff() || isBusy()) return false;

    initVars(0);
    
#if IS_M590_EMERGENCY_PIN
    state = MS_HARDRESET;
    hardresetStep = 0;
    cmdTimerMs = getMillis();
#else
    // If EMERGOFF is not set, just do POWER OFF → POWER ON (and INIT)
    hardresetPowerFlag = true;  
    powerOff();
#endif

    return true;
}

bool M590::sleepMode(bool on) {
    if (!isBegin() || isOff() || isBusy()) 
      return false;

    cmdTimerMs = 0;
    clearRxBuf();
    
    if (on) {
        state = MS_SLEEP;
        sleepStep = SLEEP_ON;
    } 
    else {
       wake();
       state = MS_SLEEP;
       sleepStep = SLEEP_OFF;
    }
    return true;
}

void M590::wake(void) {

#if IS_M590_SLEEP_PIN
    digitalWriteFast(M590_SLEEP_PIN, !M590_SLEEP_PIN_ACTIVE);
    delay(60);
#else
    // fallback (not guaranteed)
    serial.write('\r');
    delay(110);
#endif
    wake_flag = true;
    sleepFlag = false;
}

void M590::process(void) {

    if (!beginFlag) return;

    if (state == MS_OFF) return;   

    if (processReadUART()) processAnswer();

    #if IS_M590_EMERGENCY_PIN
      if (state == MS_HARDRESET) {
        processHardReset();
        return;
      }
    #endif

    if (state == MS_POWER) {
      processPower();
      return;
    }

    if (state == MS_INIT) { 
      processInit();
      return;
    }

    if (state == MS_WAIT_READY) {
      processWaitReady();
      return;
    }

    if (state == MS_SLEEP) {
      processSleep();
    }
    processAutoSleep();

    if (state == MS_SMS_CMGS) {
      processSendSMS();
      return;
    }
    
    if (state == MS_CALL) {
      processCall();
      return;
    }
   
    // Command timeout for sendCmd()
    if (state == MS_COMMAND && cmdTimeout) {
        if (millis() - cmdTimerMs > cmdTimeout) {
            status = M590_TIMEOUT;
            state = MS_IDLE; 
            return;
        }
    }
}

bool M590::processReadUART() {
  
    uartBufFlag = false;
    
    while (serial.available()) {
        if (sleepEnabledFlag) { 
          sleepTimer = getMillis(); 
          sleepFlag = false; 
        }
        char c = serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            if (idx) {
                uartBuf[idx] = 0;
                idx = 0;
                uartBufFlag = true;
                return true;
            }
            continue;
        }
        if (state == MS_SMS_CMGS && uartBuf[0] == '>' && c == ' ') {
          uartBuf[idx] = 0;
          idx = 0;
          return true;          
        }
        if (idx < sizeof(uartBuf) - 1) { 
          uartBuf[idx++] = c; 
          uartBuf[idx] = 0; 
        }
    }
    
    return false; 
}

void M590::processPower() {
 
    switch (powerStep) {

      case POWER_ON_START:
        digitalWriteFast(M590_POWER_PIN, M590_POWER_PIN_ACTIVE);
        cmdTimerMs = millis();
        powerNextStep();
        break;
        
      case POWER_ON_STEP_1:
        if (millis() - cmdTimerMs < 1000) break;
        digitalWriteFast(M590_POWER_PIN, !M590_POWER_PIN_ACTIVE);
        cmdTimerMs = millis();
        powerNextStep();
        break;

      case POWER_ON_STEP_2:
        if (millis() - cmdTimerMs < 3000) break;
        cmdTimerMs = 0;
        powerAttempts = 0;
        powerNextStep();
        break;

      case POWER_ON_STEP_3:
        if (cmdTimerMs == 0) {
            clearRxBuf(0x07);
            sendStr_P(PSTR("AT\r"));
            cmdTimerMs = millis();
            break;
        }
        if (millis() - cmdTimerMs < 1000) break;
        if (powerAttempts > 10) {
          powerFlag = false;
          initFlag = false;
          status = M590_ERROR;
          state = MS_OFF;
          return;
        } else {
          cmdTimerMs = 0;
          powerAttempts++;
        }
        break;

      case POWER_ON_END:
        powerFlag = true;
        if (hardresetPowerFlag) hardresetPowerFlag = false;
        if (autoInitFlag) { state = MS_INIT; initStep = INIT_START; }
        else { status = M590_OK; state = MS_IDLE; }
        break;


      case POWER_OFF_START:
        digitalWriteFast(M590_POWER_PIN, M590_POWER_PIN_ACTIVE);
        cmdTimerMs = millis();
        powerNextStep();
        break;
        
      case POWER_OFF_STEP_1:
        if (millis() - cmdTimerMs < 1300) break;
        digitalWriteFast(M590_POWER_PIN, !M590_POWER_PIN_ACTIVE);
        cmdTimerMs = millis();
        powerNextStep();
        break;

      case POWER_OFF_END:
        // shutdown pause ~5 sec
        if (millis() - cmdTimerMs < 5000) break;  // пауза завершения процессов
        powerFlag = false; 
        if (hardresetPowerFlag) { powerStep = POWER_ON_START; }
        else { status = M590_OK; state = MS_OFF; }
        break;
    }
}

void M590::processInit() {

    if (initStep == INIT_START) cmdTimerMs = 0;
  
    if (cmdTimerMs == 0) {

      cmdTimerMs = getMillis();
      
      switch (initStep) {
        
          case INIT_START:
              clearRxBuf(0x07);
              status = M590_NONE;
              cmdTimerMs = 0;
              initAttempts = 0;
              initFlag = false;
              initNextStep();
              break;
  
          case INIT_ATE0:
              sendStr_P(PSTR("ATE0\r"));
              break;
  
          case INIT_CMGF:
              sendStr_P(PSTR("AT+CMGF=1\r"));
              break;

          case INIT_CLIP:
              sendStr_P(PSTR("AT+CLIP=1\r"));
              break;
  
          case INIT_CSCS:
              sendStr_P(PSTR("AT+CSCS=\"GSM\"\r"));
              break;
  
          case INIT_CNMI:
              sendStr_P(PSTR("AT+CNMI=2,2,0,0,0\r"));
              break;
  
          case INIT_END:
              if (autoInitFlag) powerFlag = true;
              initFlag = true;
              status = M590_OK;
              state = MS_IDLE;
              cmdTimerMs = 0;
              return;
      }      
    }
    
  // таймаут шага
  if (millis() - cmdTimerMs > 1000) {
      if (initAttempts > 3) {
          initFlag = false;
          status = M590_ERROR;
          state = MS_IDLE;
          return;
      } else {
          cmdTimerMs = 0; 
          initAttempts++;
      }
  }
  
}

void M590::processCall() {

    if (millis() - cmdTimerMs > M590_WAIT_CALL_TIMEOUT) {
        status = M590_TIMEOUT;
        state = MS_IDLE;
        return;
    } 
}

void M590::processWaitReady() {

    if (millis() - cmdTimerMs > M590_WAIT_READY_TIMEOUT) {
        status = M590_TIMEOUT;
        state = MS_IDLE;
        return;
    } 
}

void M590::processSendSMS() {

  switch (sendSmsStep) {

    case SENDSMS_START:
      if (millis() - cmdTimerMs > 5000) {
        status = M590_TIMEOUT;
        state = MS_IDLE;
        return;
      }
    break;   
        
    case SENDSMS_SENDING:
      if (ram_txt) sendStr_RAM(ram_txt);
      else if (pgm_txt) sendStr_P(pgm_txt);
      serial.write(0x1A); // Ctrl+Z
      cmdTimerMs = millis();
      sendSmsNextStep();
    break;    

    case SENDSMS_WAIT:
      if (millis() - cmdTimerMs > 60000) {
        status = M590_TIMEOUT;
        state = MS_IDLE;
        return;
      }
    break;

    case SENDSMS_END:
      status = M590_OK;
      state = MS_IDLE;
      cmdTimerMs = 0;
    break;
        
  }
  
}

void M590::processSleep() {

    if (cmdTimerMs == 0 || 
        sleepStep == SLEEP_ON || 
        sleepStep == SLEEP_OFF ||
        sleepStep == SLEEP_OFF_1) {

      switch (sleepStep) {
        
          case SLEEP_ON:
              sendStr_P(PSTR("AT+ENPWRSAVE=1\r"));
              cmdTimerMs = getMillis();
              sleepNextStep();
              break;
              
          case SLEEP_ON_1:
              break;
              
          case SLEEP_ON_END:
              #if IS_M590_SLEEP_PIN
                  digitalWriteFast(M590_SLEEP_PIN, M590_SLEEP_PIN_ACTIVE);
              #endif
              sleepTimer = getMillis();
              sleepFlag = false;              
              sleepEnabledFlag = true;
              status = M590_OK;
              state = MS_IDLE;
              return;
              

          case SLEEP_OFF:
              #if IS_M590_SLEEP_PIN
                  digitalWriteFast(M590_SLEEP_PIN, !M590_SLEEP_PIN_ACTIVE);
                  cmdTimerMs = getMillis();
              #endif
              sleepNextStep();
              break;

          case SLEEP_OFF_1:
              #if IS_M590_SLEEP_PIN
                if (millis() - cmdTimerMs < 100) break;
              #endif 
              sendStr_P(PSTR("AT+ENPWRSAVE=0\r"));
              cmdTimerMs = getMillis();
              break;
  
          case SLEEP_OFF_END:
              sleepTimer = 0;
              status = M590_OK;
              state = MS_IDLE;
              sleepEnabledFlag = false;
              sleepFlag = false;
              break;
      }
    }

    if (millis() - cmdTimerMs > 2000) {
        status = M590_ERROR;
        state = MS_IDLE;
        return;
    }
}

void M590::processAutoSleep() {

  if (sleepEnabledFlag && state == MS_IDLE) {

    if (wake_flag) {
      wake_flag = false;
      sleepFlag = false;
      sleepTimer = getMillis();
      #if IS_M590_SLEEP_PIN
        digitalWriteFast(M590_SLEEP_PIN, M590_SLEEP_PIN_ACTIVE);
      #endif          
    }
    
    if (sleepTimer && (millis() - sleepTimer > M590_AUTO_SLEEP_MS)) {
        sleepFlag = true;
        sleepTimer = 0;
    }
  }
}

void M590::processHardReset() {

    // --- Hard Reset через EMERGOFF ---
    #if IS_M590_EMERGENCY_PIN

      switch(hardresetStep) {
    
        case HARDRESET_START:
            digitalWriteFast(M590_EMERGENCY_PIN, M590_EMERGENCY_PIN_ACTIVE);
            cmdTimerMs = millis();
            hardresetNextStep();
            break;
        
        case HARDRESET_WAIT:
            if (millis() - cmdTimerMs >= 100) { 
                digitalWriteFast(M590_EMERGENCY_PIN, !M590_EMERGENCY_PIN_ACTIVE);
                cmdTimerMs = millis(); 
                hardresetNextStep();
            }
            break;
        
        case HARDRESET_END:
            if (millis() - cmdTimerMs >= 5000) {
              initVars(0);
              if (autoInitFlag) { state = MS_POWER; powerStep = POWER_ON_START; }
              else { status = M590_OK; state = MS_IDLE; }
            }
            break;            
      }
      
    #elif  
        status = M590_OK; 
        state = MS_IDLE; 
    #endif
}

void M590::processAnswer() {

    char* s = uartBuf;

    if (!s[0]) return;

    if (strcmp_P(s, PSTR("+PBREADY")) == 0) { 
      readyFlag = true; 
      status = M590_READY; 
      if (state == MS_WAIT_READY) state = MS_IDLE;
      return;
      }
    else if (strcmp_P(s, PSTR("RDY")) == 0) return;   
    else if (strcmp_P(s, PSTR("Call Ready")) == 0) return;
    else if (strcmp_P(s, PSTR("SMS Ready")) == 0) return;    

    else if (strcmp_P(s, PSTR("OK")) == 0) {
        if (state == MS_POWER) { if (powerStep == POWER_ON_STEP_3) powerNextStep(); return; }
        if (state == MS_INIT) { initNextStep(); cmdTimerMs = 0; return; }
        if (state == MS_SLEEP) { sleepNextStep(); cmdTimerMs = 0; return; }
        if (state == MS_SMS_CMGS) { if (sendSmsStep == SENDSMS_SENDING) sendSmsNextStep(); return; }
        if (state == MS_CALL) { status = M590_OK; return; }
      state = MS_IDLE;  
      status = M590_OK;
      return;   
    }
    else if (strstr_P(s, PSTR("ERROR"))) {    
        if (state == MS_POWER || state == MS_INIT || state == MS_SLEEP) return; 
        state = MS_IDLE;
        status = M590_ERROR;
        return;
    }
    else if (strcmp_P(s, PSTR("NO CARRIER")) == 0) { 
      state = MS_IDLE;      
      status = M590_NO_CARRIER;  
      return; 
    }
    else if (strcmp_P(s, PSTR("NO ANSWER")) == 0) { 
      state = MS_IDLE;      
      status = M590_NO_ANSWER; 
      return; 
    }
    else if (strcmp_P(s, PSTR("BUSY")) == 0) {
      state = MS_IDLE;
      status = M590_BUSY; 
      return; 
      }
    else if (strcmp_P(s, PSTR("CONNECT")) == 0) {
      status = M590_CONNECT; 
      return; 
      }
      
    else if (strncmp_P(s, PSTR("+CMGS:"), 6) == 0) {
        return;
    }

    // waiting '>' for send SMS
    if (state == MS_SMS_CMGS && s[0] == '>') {
        if (sendSmsStep == SENDSMS_START) sendSmsNextStep();
    }

    // Incomming call
    if (strcmp_P(s, PSTR("RING")) == 0) {
      if (!incomingCall)  {
          incomingCall = true;
          incomingCallNumberReady = false;
          callerNumber[0] = 0;
      }
    }

    // Incomming call number
    else if (strncmp_P(s, PSTR("+CLIP:"), 6) == 0) {
        if (incomingCall) {
            parseNumber(s, callerNumber, sizeof(callerNumber));
            insertPlus(callerNumber,sizeof(callerNumber));
            incomingCallNumberReady = true;
        }
    }

    // Notification of a new SMS
    else if (strncmp_P(s, PSTR("+CMTI:"), 6) == 0) {
        incomingSMS = true;
        incomingSMSall = false;
        smsText[0] = 0;
        smsNumber[0] = 0;
    }

    // Start SMS
    else if (strncmp_P(s, PSTR("+CMT:"), 5) == 0) {
        incomingSMS = true;
        incomingSMSall = false;   
        smsText[0] = 0;
        smsNumber[0] = 0;
        parseNumber(s, smsNumber, sizeof(smsNumber));
    }

    // Text SMS
    else if (incomingSMS && !incomingSMSall && smsNumber[0]) {
        strncpy(smsText, s, sizeof(smsText)-1);
        smsText[sizeof(smsText)-1] = 0;
        incomingSMSall = true;
    }

    // Other
    else if (state != MS_OFF)  {
        status = M590_UNKNOWN;
    }
}

void M590::parseNumber(const char *s, char *out, uint8_t outSize) {

    if (!out || outSize == 0) return;
    out[0] = 0;

    const char *p1 = strchr(s, '"');
    if (!p1) return;
    p1++;

    const char *p2 = strchr(p1, '"');
    if (!p2) return;

    uint8_t len = p2 - p1;
    if (len == 0) return;
    if (len > outSize - 1) len = outSize - 1;

    if (strncmp(p1, "UNKNOWN", 7) == 0) return;

    memcpy(out, p1, len);
    out[len] = 0;
}

void M590::sendStr_RAM(const char* s) {

    if (sleepFlag) wake();
      
    serial.print(s);
}

void M590::sendStr_P(PGM_P s) {

    if (sleepFlag) wake();
      
    while (1) {
        char c = pgm_read_byte(s++);
        if (!c) break;
        serial.write(c);
    }
}

bool M590::sendCmd(const char* cmd, uint16_t timeout = M590_CMD_TIMEOUT_DEFAULT) {
    if (isOff() || isBusy() || !isBegin()) return false; 
    if (!cmd || !cmd[0]) return false;

    if (sleepFlag) wake();
    
    serial.print(cmd);
    serial.print('\r');
    
    state = MS_COMMAND;
    status = M590_NONE;
    cmdTimerMs = getMillis();
    cmdTimeout = timeout;
    clearRxBuf();    
    return true;    
}

bool M590::sendCmd_P(PGM_P cmd, uint16_t timeout = M590_CMD_TIMEOUT_DEFAULT) {
    if (isOff() || isBusy() || !isBegin()) return false;
    if (!cmd || !pgm_read_byte(cmd)) return false;

    if (sleepFlag) wake();

    while (1) {
        char c = pgm_read_byte(cmd++);
        if (!c) break;
        serial.write(c);
    }
    serial.write('\r');
    
    state = MS_COMMAND;
    status = M590_NONE;
    cmdTimerMs = getMillis();
    cmdTimeout = timeout;
    clearRxBuf();    
    return true;      
}

bool M590::sendSMS(const char* number, const char* text) {
    if (isOff() || isBusy() || !isBegin()) return false;
    if (!number || !number[0] || !text || !text[0]) return false;
    
    pgm_num = pgm_txt = nullptr;
    ram_num = number; ram_txt = text;

    sendStr_P(PSTR("AT+CMGS=\""));
    sendStr_RAM(number);
    sendStr_P(PSTR("\"\r"));

    state = MS_SMS_CMGS;
    status = M590_NONE;
    sendSmsStep = 0;
    cmdTimerMs = getMillis();    
    return true;
}

bool M590::sendSMS_P(PGM_P number, PGM_P text) {
    if (isOff() || isBusy() || !isBegin()) return false;
    if (!number || !pgm_read_byte(number) || !text || !pgm_read_byte(text)) return false;

    ram_num = ram_txt = nullptr;
    pgm_num = number; pgm_txt = text;

    sendStr_P(PSTR("AT+CMGS=\""));
    sendStr_P(number);
    sendStr_P(PSTR("\"\r"));

    state = MS_SMS_CMGS;
    status = M590_NONE;
    sendSmsStep = 0;
    cmdTimerMs = getMillis();   
    return true;
}

bool M590::sendSMS_RAM_P(const char* number, PGM_P text) {
    if (isOff() || isBusy() || !isBegin()) return false;
    if (!number || !number[0] || !text || !pgm_read_byte(text)) return false;

    ram_txt = pgm_num = nullptr;
    ram_num = number; pgm_txt = text;

    sendStr_P(PSTR("AT+CMGS=\""));
    sendStr_RAM(number);
    sendStr_P(PSTR("\"\r"));

    state = MS_SMS_CMGS;
    status = M590_NONE;
    sendSmsStep = 0;
    cmdTimerMs = getMillis();   
    return true;
}

bool M590::sendCall(const char* number) {
    if (isOff() || isBusy() || !isBegin()) return false;
    if (!number || !number[0]) return false;

    ram_txt = pgm_num = pgm_txt = nullptr;    
    ram_num = number;

    sendStr_P(PSTR("ATD"));
    sendStr_RAM(number);
    sendStr_P(PSTR(";\r"));

    state = MS_CALL;
    status = M590_NONE;    
    sendSmsStep = 0;
    cmdTimerMs = getMillis();
    return true;
}

bool M590::sendCall_P(PGM_P number) {
    if (isOff() || isBusy() || !isBegin()) return false;
    if (!number || !pgm_read_byte(number)) return false;

    ram_txt = pgm_num = pgm_txt = nullptr;    
    ram_num = number;

    sendStr_P(PSTR("ATD"));
    sendStr_P(number);
    sendStr_P(PSTR(";\r"));

    status = M590_NONE;
    state = MS_CALL;
    sendSmsStep = 0;
    cmdTimerMs = getMillis();
    return true;
}

bool M590::waitReady() {
  if (!isBegin() || isOff() || isBusy()) return false;

    status = M590_NONE;
    state = MS_WAIT_READY;
    cmdTimerMs = getMillis(); 
    return true;
}


//===== END =======
