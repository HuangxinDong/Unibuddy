#pragma once
#include <Arduino.h>
#include "config.h"

enum InputEvent {
  EVT_NONE,
  EVT_BTN_SHORT,
  EVT_BTN_LONG,
  EVT_TAP,
  EVT_MOTION
};

static bool     _lastBtnState   = HIGH;
static uint32_t _btnPressTime   = 0;
static bool     _btnHandled     = false;
static uint32_t _lastTapTime    = 0;
static uint32_t _lastMotionTime = 0;

const uint16_t SENSOR_COOLDOWN_MS = 250;
const uint16_t SENSOR_DEBOUNCE_MS = 15;

void initInput() {
  pinMode(PIN_BUTTON,   INPUT_PULLUP);
  pinMode(PIN_TAP_KY031, INPUT_PULLUP);
  pinMode(PIN_MOVEMENT,  INPUT_PULLUP);
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

  // ── KY031 tap sensor (active LOW with pull-up) ─────────────
  if (digitalRead(PIN_TAP_KY031) == LOW) {
    if (millis() - _lastTapTime >= SENSOR_COOLDOWN_MS) {
      delay(SENSOR_DEBOUNCE_MS);
      if (digitalRead(PIN_TAP_KY031) == LOW) {
        _lastTapTime = millis();
        return EVT_TAP;
      }
    }
  }

  // ── Movement sensor trigger (active LOW with pull-up) ──────
  if (digitalRead(PIN_MOVEMENT) == LOW) {
    if (millis() - _lastMotionTime >= SENSOR_COOLDOWN_MS) {
      delay(SENSOR_DEBOUNCE_MS);
      if (digitalRead(PIN_MOVEMENT) == LOW) {
        _lastMotionTime = millis();
        return EVT_MOTION;
      }
    }
  }

  return EVT_NONE;
}
