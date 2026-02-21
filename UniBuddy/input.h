#pragma once
/*
 * ────────────────────────────────────────────────────────────
 *  input.h — Button & shake-sensor driver
 *
 *  Button (PIN_BUTTON, D2):
 *    INPUT_PULLUP, active LOW while held.
 *    Short press  (<600 ms) → EVT_BTN_SHORT
 *    Long  press  (≥600 ms) → EVT_BTN_LONG   (fires while held)
 *
 *  Shake sensor (PIN_SHAKE_SW, D3):
 *    SW-420, goes LOW on vibration → EVT_SHAKE
 * ────────────────────────────────────────────────────────────
 */
#include <Arduino.h>
#include "config.h"

// ── Event type returned by readInput() ──────────────────────
enum InputEvent {
  EVT_NONE,
  EVT_BTN_SHORT,        // released before long-press threshold
  EVT_BTN_LONG,         // held past BTN_LONG_PRESS_MS
  EVT_SHAKE             // vibration detected
};

// ── Internal state ──────────────────────────────────────────
static bool     _btnPrev    = HIGH;
static uint32_t _btnDownMs  = 0;
static bool     _btnFired   = false;   // long-press already emitted?

// ── Public API ──────────────────────────────────────────────

void initInput() {
  pinMode(PIN_BUTTON,   INPUT_PULLUP);
  pinMode(PIN_SHAKE_SW, INPUT_PULLUP);
}

InputEvent readInput() {
  /* ─── Button ──────────────────────────────────────────── */
  bool cur = digitalRead(PIN_BUTTON);

  // Falling edge → record press time
  if (cur == LOW && _btnPrev == HIGH) {
    _btnDownMs = millis();
    _btnFired  = false;
  }

  // Still held → check for long press
  if (cur == LOW && !_btnFired) {
    if (millis() - _btnDownMs >= BTN_LONG_PRESS_MS) {
      _btnFired  = true;
      _btnPrev   = cur;
      return EVT_BTN_LONG;
    }
  }

  // Rising edge → short press if long press wasn't already fired
  if (cur == HIGH && _btnPrev == LOW && !_btnFired) {
    _btnPrev = cur;
    if (millis() - _btnDownMs >= BTN_DEBOUNCE_MS)
      return EVT_BTN_SHORT;
  }

  _btnPrev = cur;

  /* ─── Shake sensor ────────────────────────────────────── */
  if (digitalRead(PIN_SHAKE_SW) == LOW) {
    delay(50);                             // debounce
    if (digitalRead(PIN_SHAKE_SW) == LOW)
      return EVT_SHAKE;
  }

  return EVT_NONE;
}
