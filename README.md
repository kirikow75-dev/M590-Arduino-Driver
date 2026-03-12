# M590 Arduino Driver

![Platform](https://img.shields.io/badge/platform-Arduino-blue)
![Status](https://img.shields.io/badge/status-stable-brightgreen)
![License](https://img.shields.io/badge/license-MIT-green)
[![Compile Examples](https://github.com/kirikow75-dev/M590-Arduino-Driver/actions/workflows/compile.yml/badge.svg)](https://github.com/kirikow75-dev/M590-Arduino-Driver/actions/workflows/compile.yml)

Lightweight asynchronous driver for the **M590 GSM modem** for Arduino platforms.

The library is built around a **Finite State Machine (FSM)** and provides fully **non-blocking modem control** without using `delay()`.

Designed for **reliable embedded systems and long-running devices**.

---

# Table of Contents

* Features
* Supported Hardware
* Hardware Connection
* Driver Architecture
* Driver Workflow
* Typical Usage Pattern
* Examples
* Project Structure
* Important Notes
* Design Goals
* Contributing
* License
* Commercial Licensing

---

# Features

* Non-blocking modem control
* Finite State Machine architecture
* SMS send / receive support
* Incoming call detection
* Designed for integration inside `loop()`
* No blocking delays
* Minimal RAM usage
* Suitable for long-running embedded systems

---

# Supported Hardware

* M590 GSM modem
* Arduino compatible boards
* AVR microcontroller supporting:
  * LGT8F328P
  * ATMEGA328 (no testing)

---

# Hardware Connection

Example connection:

 LGT8F328P Mini ->  M590
 
 RX -> RESISTOR 100 Ohm -> TX
 
 TX -> RESISTOR 100 Ohm -> RX
 
 PIN12 -> RESISTOR 100 Ohm -> SLEEP
 
 PIN9 -> RESISTOR 100 Ohm -> POWER ON/OFF
 
 PIN8 -> RESISTOR 100 Ohm -> EMERGENCY 
 
 Use decoupling resistors 100 Ohm 0,1W (0603).

WARNING!
I. The M590 modem requires a **stable power supply capable of handling current peaks** during transmission.
II.The LGT8F328P-Mini board is designed for 3-5V power, while the M490 modem operates on 3.3-4.5V. 
However, due to the M590 modem's potential peak current consumption of up to 2A, it is recommended 
to use separate power supply circuits. In this setup, the LGT8F328P's supply voltage should be at 
least 3.3V and not exceed the M590's supply voltage to ensure logical level voltage compatibility.

---

# Driver Architecture

The driver is implemented as a **Finite State Machine (FSM)**.

All modem communication is processed inside the `process()` function.

This architecture provides:

* non-blocking execution
* deterministic timing
* reliable modem communication
* safe integration into embedded systems

The FSM advances step-by-step every time `process()` is called.

---

# Driver Workflow

The driver works asynchronously and **must be serviced continuously**.

The application must call:

```
modem.process();
```

inside the main loop.

The driver **can process only one command at a time** and **does not implement a command queue**.

Before sending a new command, the application must ensure the driver is free.

Driver state can be checked using:

```
modem.isBusy()
```

---

# Typical Usage Pattern

```
void loop() {

    modem.process();

    if (modem.isBusy())
        return;

    if (modem.isIncomingSMS()) {

        // Process received SMS here

        modem.clearIncomingSMS();
        return;
    }

}
```

This pattern guarantees that:

* the modem FSM runs continuously
* only one command executes at a time
* commands do not overlap

---

# Examples

## Sending SMS

```
void loop() {

	static bool flagSend = false;
	
    modem.process();

    if (modem.isBusy())
        return;

    if (!flagSend) {
		modem.sendSMS("+1234567890", "Hello!");
		flagSend = true;
		return;
	}
}
```

---

## Receiving SMS

```
void loop() {

    modem.process();

    if (modem.isBusy())
        return;

    if (modem.isIncomingSMS()) {

		char* number = modem.getSMSNumber();
		char* text = modem.getSMSText();

        // Process message

        modem.clearIncomingSMS();
        return;
    }

}
```

---

# Project Structure

```
M590-driver
│
├── src
│   ├── M590Driver.h
│   ├── M590Func.h
│   └── fast_gpio.h
│
├── examples
│   ├── BasicUsage
│   ├── SendSMS
│   ├── ReceiveSMS
│	├── SendCall
│   ├── ReceiveCall
│   ├── UserCommands
│   ├── SleepMode
│   ├── HardwareReset
│   └── FullInit
│
├── docs
│   ├── driver_description.txt
│   ├── wiring_diagram.png
│   ├── m590_commands.pdf
│   └── m590_hardware.pdf
│
└── README.md
```

---

# Important Notes

* The driver **does not support multiple simultaneous commands**
* Always check `isBusy()` before starting a new command
* `process()` must be called **as frequently as possible**
* Commands must not overlap

---

# Design Goals

This driver was designed for:

* reliable embedded systems
* non-blocking Arduino applications
* autonomous devices running for long periods
* simple integration into existing projects

---

# Contributing

Contributions are welcome.

If you find a bug or want to improve the driver, please open an issue or submit a pull request.

---

# License

MIT License

---

# Commercial Licensing

This library is provided as open-source software.

For companies developing commercial devices, a professional
version with extended features and engineering support
is available under a commercial license.


---




