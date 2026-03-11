/*
 *  Fast and secure port operations AVR:
 * digitalWriteFast(pin, HIGH/LOW)   - writes to the output port
 * digitalReadFast(pin)              - reads the value of the input port
 * digitalReadFastPort(pin)          - reads PORTx (output latch)
 * digitalToggleFast(pin)            - inverts the value of the output port
 * pinModeFast(pin, INPUT/OUTPUT/INPUT_PULLUP) - sets the port mode
 *
*/

#pragma once

#ifndef FASTGPIO_H
#define FASTGPIO_H

#include <Arduino.h>

// Note: Arduino pin numbers mapping:
//  0..7   -> PD0..PD7
//  8..13  -> PB0..PB5  (8 -> PB0, 13 -> PB5)
//  14..19 -> PC0..PC5  (A0..A5 -> 14..19)

// ---------- low-level inline helpers ----------
#define _FG_BIT_MASK(b) (1UL << (b))

// ---------- compile-time optimized macros (expression form) ----------
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || defined(__LGT8F328P)

// digitalWriteFast(pin, val) -> EXPRESSION
#define digitalWriteFast(pin, val) \
  ( __builtin_constant_p(pin) ? \
      ( (pin) < 8 ? ( (val) ? (PORTD |= _FG_BIT_MASK(pin)) : (PORTD &= ~_FG_BIT_MASK(pin)) ) : \
        ( (pin) < 14 ? ( (val) ? (PORTB |= _FG_BIT_MASK((pin)-8)) : (PORTB &= ~_FG_BIT_MASK((pin)-8)) ) : \
                        ( (val) ? (PORTC |= _FG_BIT_MASK((pin)-14)) : (PORTC &= ~_FG_BIT_MASK((pin)-14)) ) ) ) \
    : (digitalWrite((pin),(val)), 0) )

// digitalToggleFast(pin) -> EXPRESSION
#define digitalToggleFast(pin) \
  ( __builtin_constant_p(pin) ? \
      ( (pin) < 8 ? (PORTD ^= _FG_BIT_MASK(pin)) : \
        ( (pin) < 14 ? (PORTB ^= _FG_BIT_MASK((pin)-8)) : (PORTC ^= _FG_BIT_MASK((pin)-14)) ) ) \
    : (digitalWrite((pin), !digitalRead((pin))), 0) )

// digitalReadFast(pin) -> reads PINx actual physical level (0/1)
#define digitalReadFast(pin) \
  ( __builtin_constant_p(pin) ? \
      ( (pin) < 8 ? ((PIND >> (pin)) & 1) : \
        ( (pin) < 14 ? ((PINB >> ((pin)-8)) & 1) : ((PINC >> ((pin)-14)) & 1) ) ) \
    : digitalRead(pin) )

// digitalReadFastPort(pin) -> reads PORTx latch (0/1)
#define digitalReadFastPort(pin) \
  ( __builtin_constant_p(pin) ? \
      ( (pin) < 8 ? ((PORTD >> (pin)) & 1) : \
        ( (pin) < 14 ? ((PORTB >> ((pin)-8)) & 1) : ((PORTC >> ((pin)-14)) & 1) ) ) \
    : (digitalRead(pin)) )

// pinModeFast(pin, mode) -> EXPRESSION (mode: INPUT, INPUT_PULLUP, OUTPUT)
#define pinModeFast(pin, mode) \
  ( __builtin_constant_p(pin) ? \
      ( ((mode) == OUTPUT) ? \
          ( ( (pin) < 8 ? (DDRD |= _FG_BIT_MASK(pin)) : \
              ( (pin) < 14 ? (DDRB |= _FG_BIT_MASK((pin)-8)) : (DDRC |= _FG_BIT_MASK((pin)-14)) ) ), 0 ) : \
        ( ((mode) == INPUT_PULLUP) ? \
          ( ( (pin) < 8 ? (DDRD &= ~_FG_BIT_MASK(pin), PORTD |= _FG_BIT_MASK(pin)) : \
              ( (pin) < 14 ? (DDRB &= ~_FG_BIT_MASK((pin)-8), PORTB |= _FG_BIT_MASK((pin)-8)) : (DDRC &= ~_FG_BIT_MASK((pin)-14), PORTC |= _FG_BIT_MASK((pin)-14)) ) ), 0 ) : \
          ( /* INPUT */ ( (pin) < 8 ? (DDRD &= ~_FG_BIT_MASK(pin), PORTD &= ~_FG_BIT_MASK(pin)) : \
                         ( (pin) < 14 ? (DDRB &= ~_FG_BIT_MASK((pin)-8), PORTB &= ~_FG_BIT_MASK((pin)-8)) : (DDRC &= ~_FG_BIT_MASK((pin)-14), PORTC &= ~_FG_BIT_MASK((pin)-14)) ) ), 0 ) ) ) \
    : (pinMode((pin),(mode)), 0) )

#else
// Fallback for other MCUs — use Arduino API (not optimized)
#define digitalWriteFast(pin, val) digitalWrite((pin),(val))
#define digitalToggleFast(pin)    digitalWrite((pin), !digitalRead((pin)))
#define digitalReadFast(pin)      digitalRead((pin))
#define digitalReadFastPort(pin)  digitalRead((pin))
#define pinModeFast(pin, mode)    pinMode((pin),(mode))
#endif

// ---------- small inline wrappers (optional) ----------
static inline void fg_pinMode(uint8_t pin, uint8_t mode) { (void)pinModeFast(pin, mode); }
static inline void fg_digitalWrite(uint8_t pin, uint8_t val) { (void)digitalWriteFast(pin, val); }
static inline void fg_toggle(uint8_t pin) { (void)digitalToggleFast(pin); }
static inline int  fg_digitalRead(uint8_t pin) { return digitalReadFast(pin); }
static inline int  fg_digitalReadPort(uint8_t pin) { return digitalReadFastPort(pin); }

#endif // FASTGPIO_H
