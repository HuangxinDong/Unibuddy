#pragma once
#include <Arduino.h>
#include "config.h"

enum InputEvent {
  EVT_NONE,
  EVT_BTN_SHORT,
  EVT_BTN_LONG,
  EVT_SHAKE
};

static bool     _lastBtnState   = HIGH;
static uint32_t _btnPressTime   = 0;
static bool     _btnHandled     = false;

void initInput() {
  pinMode(PIN_BUTTON,   INPUT_PULLUP);
  pinMode(PIN_SHAKE_SW, INPUT_PULLUP);
}

InputEvent readInput() {
  // ── Button ──────────────────────────────────────────────────
  bool btnState = digitalRead(PIN_BUTTON);

  if (btnState == LOW && _lastBtnState == HIGH) {
    // pressed
    _btnPressTime = millis();
    _btnHandled   = false;
  }

  if (btnState == LOW && !_btnHandled) {
    if (millis() - _btnPressTime >= BTN_LONG_PRESS_MS) {
      _btnHandled = true;
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

  // ── Shake sensor ────────────────────────────────────────────
  // SW-420 goes LOW when vibration detected
  if (digitalRead(PIN_SHAKE_SW) == LOW) {
    delay(50); // debounce
    if (digitalRead(PIN_SHAKE_SW) == LOW) {
      return EVT_SHAKE;
    }
  }

  return EVT_NONE;
}
