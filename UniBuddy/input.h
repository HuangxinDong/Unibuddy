#pragma once
#include <Arduino.h>
#include "config.h"

/*
 * ============================================================
 *  Input Module — Button + KY031 Tap (ISR) + Movement Sensor
 *
 *  Button (PIN_BUTTON D4, INPUT_PULLUP):
 *    Polling with debounce / long-press detection.
 *      • Short press → EVT_BTN_SHORT
 *      • Long press  → EVT_BTN_LONG
 *
 *  KY031 Tap Sensor (PIN_TAP_KY031 D2, INPUT_PULLUP):
 *    Produces a very brief LOW pulse (~1-5 ms) on each knock.
 *    Too short for polling — uses attachInterrupt(CHANGE) to
 *    count falling edges via ISR, then readInput() evaluates
 *    after DOUBLE_TAP_WINDOW_MS expires:
 *      • 1 tap   → EVT_TAP
 *      • 2+ taps → EVT_DOUBLE_TAP  (used to start Pomodoro)
 *
 *  Movement Sensor (PIN_MOVEMENT D3, INPUT_PULLUP):
 *    Polling (signal is long enough). Goes LOW → EVT_MOTION
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

// ── Button state ────────────────────────────────────────────
static bool     _lastBtnState   = HIGH;
static uint32_t _btnPressTime   = 0;
static bool     _btnHandled     = false;

// ── KY031 ISR state (volatile!) ─────────────────────────────
#define TAP_DEBOUNCE_MS       60    // ignore edges closer than this
#define DOUBLE_TAP_WINDOW_MS  400   // max gap between taps for double-tap

static volatile uint32_t _isrTapCount  = 0;
static volatile uint32_t _isrFirstTap  = 0;
static volatile uint32_t _isrLastEdge  = 0;

// ISR — fires on CHANGE; we count falling edges (HIGH→LOW = tap start)
static void _tapISR() {
  uint32_t now = millis();
  if (now - _isrLastEdge < TAP_DEBOUNCE_MS) return;   // bounce filter
  _isrLastEdge = now;
  // Only count falling edges (pin just went LOW = tap pulse start)
  if (digitalRead(PIN_TAP_KY031) == LOW) {
    if (_isrTapCount == 0) _isrFirstTap = now;
    _isrTapCount++;
  }
}

// ── Movement sensor state ───────────────────────────────────
static uint32_t _lastMotionTime = 0;
const uint16_t  MOTION_COOLDOWN_MS = 300;

void initInput() {
  pinMode(PIN_BUTTON,    INPUT_PULLUP);
  pinMode(PIN_TAP_KY031, INPUT_PULLUP);
  pinMode(PIN_MOVEMENT,  INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_TAP_KY031), _tapISR, CHANGE);
}

InputEvent readInput() {
  // ── Button (polling — pulse is long enough) ─────────────────
  bool btnState = digitalRead(PIN_BUTTON);

  if (btnState == LOW && _lastBtnState == HIGH) {
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

  // ── KY031 tap sensor (interrupt-driven) ─────────────────────
  // ISR counts falling edges. We wait for the double-tap window
  // to close, then evaluate how many taps were collected.
  if (_isrTapCount > 0) {
    noInterrupts();
    uint32_t firstTap = _isrFirstTap;
    uint32_t count    = _isrTapCount;
    interrupts();

    if (millis() - firstTap >= DOUBLE_TAP_WINDOW_MS) {
      // Window expired — evaluate
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

  // ── Movement sensor (polling — signal is long enough) ───────
  if (digitalRead(PIN_MOVEMENT) == LOW) {
    if (millis() - _lastMotionTime >= MOTION_COOLDOWN_MS) {
      delay(15);  // debounce
      if (digitalRead(PIN_MOVEMENT) == LOW) {
        _lastMotionTime = millis();
        return EVT_MOTION;
      }
    }
  }

  return EVT_NONE;
}
