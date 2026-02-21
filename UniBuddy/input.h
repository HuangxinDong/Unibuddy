#pragma once
#include <Arduino.h>
#include "config.h"

/*
 * ============================================================
 *  input.h - Button + KY031 Tap (ISR) + Movement Sensor
 *
 *  Button (PIN_BUTTON D4, INPUT_PULLUP):
 *    Polling with debounce / long-press.
 *    Short press -> EVT_BTN_SHORT
 *    Long press  -> EVT_BTN_LONG
 *
 *  KY031 (PIN_TAP_KY031 D2, INPUT_PULLUP):
 *    Brief LOW pulse on knock - uses ISR to count taps.
 *    1 tap   -> EVT_TAP
 *    2+ taps -> EVT_DOUBLE_TAP
 *
 *  Movement (PIN_MOVEMENT D3, INPUT_PULLUP):
 *    Polling. Goes LOW -> EVT_MOTION
 * ============================================================
 */

enum InputEvent {
  EVT_NONE,
  EVT_BTN_SHORT,
  EVT_BTN_LONG,
  EVT_TAP,
  EVT_DOUBLE_TAP,
  EVT_MOTION
};

// Button state
static bool     _lastBtnState  = HIGH;
static uint32_t _btnPressTime  = 0;
static bool     _btnHandled    = false;

// KY031 ISR state
#define TAP_DEBOUNCE_MS       60
#define DOUBLE_TAP_WINDOW_MS  400

static volatile uint32_t _isrTapCount = 0;
static volatile uint32_t _isrFirstTap = 0;
static volatile uint32_t _isrLastEdge = 0;

static void _tapISR() {
  uint32_t now = millis();
  if (now - _isrLastEdge < TAP_DEBOUNCE_MS) return;
  _isrLastEdge = now;
  if (digitalRead(PIN_TAP_KY031) == LOW) {
    if (_isrTapCount == 0) _isrFirstTap = now;
    _isrTapCount++;
  }
}

// Movement state
static uint32_t _lastMotionTime = 0;
const uint16_t  MOTION_COOLDOWN_MS = 300;

void initInput() {
  pinMode(PIN_BUTTON,    INPUT_PULLUP);
  pinMode(PIN_TAP_KY031, INPUT_PULLUP);
  pinMode(PIN_MOVEMENT,  INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TAP_KY031), _tapISR, CHANGE);
}

InputEvent readInput() {
  // -- Button (polling) --
  bool btnState = digitalRead(PIN_BUTTON);

  if (btnState == LOW && _lastBtnState == HIGH) {
    _btnPressTime = millis();
    _btnHandled   = false;
  }

  if (btnState == LOW && !_btnHandled) {
    if (millis() - _btnPressTime >= BTN_LONG_PRESS_MS) {
      _btnHandled   = true;
      _lastBtnState = btnState;
      return EVT_BTN_LONG;
    }
  }

  if (btnState == HIGH && _lastBtnState == LOW && !_btnHandled) {
    uint32_t held = millis() - _btnPressTime;
    _lastBtnState = btnState;
    if (held >= BTN_DEBOUNCE_MS) return EVT_BTN_SHORT;
  }

  _lastBtnState = btnState;

  // -- KY031 tap (interrupt-driven) --
  if (_isrTapCount > 0) {
    noInterrupts();
    uint32_t firstTap = _isrFirstTap;
    uint32_t count    = _isrTapCount;
    interrupts();

    if (millis() - firstTap >= DOUBLE_TAP_WINDOW_MS) {
      noInterrupts();
      _isrTapCount = 0;
      interrupts();

      if (count >= 2) {
        Serial.print(F("[Input] DOUBLE TAP ("));
        Serial.print(count);
        Serial.println(F(" edges)"));
        return EVT_DOUBLE_TAP;
      } else {
        Serial.println(F("[Input] SINGLE TAP"));
        return EVT_TAP;
      }
    }
  }

  // -- Movement sensor (polling) --
  if (digitalRead(PIN_MOVEMENT) == LOW) {
    if (millis() - _lastMotionTime >= MOTION_COOLDOWN_MS) {
      delay(15);
      if (digitalRead(PIN_MOVEMENT) == LOW) {
        _lastMotionTime = millis();
        return EVT_MOTION;
      }
    }
  }

  return EVT_NONE;
}
