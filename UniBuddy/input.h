#pragma once
#include <Arduino.h>
#include "config.h"

/*
 * ============================================================
 *  Input Module — Tap Sensor (interrupt) + Shake Sensor
 *
 *  Tap Sensor (D2, INPUT_PULLUP):
 *    Produces a very brief LOW pulse on each tap — too short
 *    for polling to catch reliably. We use attachInterrupt()
 *    on RISING edge (end of pulse = return to HIGH) to count
 *    taps via ISR, then evaluate in readInput():
 *      • Single tap   → EVT_BTN_SHORT
 *      • Double tap   → EVT_BTN_LONG
 *
 *  Shake Sensor (D3, INPUT_PULLUP):
 *    SW-420, goes LOW on vibration.
 * ============================================================
 */

enum InputEvent {
  EVT_NONE,
  EVT_BTN_SHORT,      // single tap
  EVT_BTN_LONG,       // double tap
  EVT_SHAKE
};

// ── ISR state (volatile!) ───────────────────────────────────
static volatile uint8_t  _isrTapCount   = 0;
static volatile uint32_t _isrFirstTap   = 0;
static volatile uint32_t _isrLastEdge   = 0;

// ISR — called on RISING edge (tap pulse ends, pin goes HIGH)
void _tapISR() {
  uint32_t now = millis();
  if (now - _isrLastEdge < TAP_DEBOUNCE_MS) return;  // debounce
  _isrLastEdge = now;
  if (_isrTapCount == 0) _isrFirstTap = now;
  _isrTapCount++;
}

void initInput() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), _tapISR, RISING);

  pinMode(PIN_SHAKE_SW, INPUT_PULLUP);
}

InputEvent readInput() {
  // ── Tap Sensor (interrupt-driven) ───────────────────────────
  // ISR increments _isrTapCount on each rising edge.
  // We wait for the double-tap window to expire, then evaluate.
  if (_isrTapCount > 0) {
    noInterrupts();
    uint32_t firstTap = _isrFirstTap;
    uint8_t  count    = _isrTapCount;
    interrupts();

    if (millis() - firstTap >= DOUBLE_TAP_WINDOW_MS) {
      noInterrupts();
      _isrTapCount = 0;        // reset for next detection
      interrupts();

      if (count >= 2) {
        return EVT_BTN_LONG;   // double tap
      } else {
        return EVT_BTN_SHORT;  // single tap
      }
    }
  }

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
