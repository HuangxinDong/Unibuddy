#pragma once
/*
 * ────────────────────────────────────────────────────────────
 *  servo_arm.h — SG90 servo "nudge" wave animation
 *
 *  Falls back to Serial logging when <Servo.h> is unavailable.
 * ────────────────────────────────────────────────────────────
 */
#include <Arduino.h>
#include "config.h"

// ── Servo library availability (auto-detected) ──────────────
#if defined(__has_include)
  #if __has_include(<Servo.h>)
    #include <Servo.h>
    #define SERVO_LIB_AVAILABLE 1
  #else
    #define SERVO_LIB_AVAILABLE 0
  #endif
#else
  #define SERVO_LIB_AVAILABLE 0
#endif

// ── Internal state ──────────────────────────────────────────
static bool     _nudging       = false;
static uint8_t  _nudgeStep     = 0;
static uint32_t _lastNudgeTick = 0;

// Nudge keyframes: wave → rest → wave → rest → angle → rest
static const uint8_t NUDGE_SEQ[] = {
  SERVO_REST_ANGLE,  SERVO_WAVE_ANGLE,
  SERVO_REST_ANGLE,  SERVO_WAVE_ANGLE,
  SERVO_REST_ANGLE,  SERVO_NUDGE_ANGLE,
  SERVO_REST_ANGLE
};
static const uint8_t NUDGE_SEQ_LEN = sizeof(NUDGE_SEQ);

// ── Public API ──────────────────────────────────────────────

#if SERVO_LIB_AVAILABLE
Servo buddyArm;

void initServoArm() {
  buddyArm.attach(PIN_SERVO);
  buddyArm.write(SERVO_REST_ANGLE);
}

void triggerNudge() {
  _nudging   = true;
  _nudgeStep = 0;
  _lastNudgeTick = millis();
  buddyArm.write(NUDGE_SEQ[0]);
}

void tickServoNudge() {
  if (!_nudging) return;
  if (millis() - _lastNudgeTick < 300) return;  // 300ms per step
  _lastNudgeTick = millis();

  _nudgeStep++;
  if (_nudgeStep >= NUDGE_SEQ_LEN) {
    _nudging = false;
    buddyArm.write(SERVO_REST_ANGLE);
    return;
  }
  buddyArm.write(NUDGE_SEQ[_nudgeStep]);
}

#else

void initServoArm() {
  Serial.println(F("[Servo] Servo.h missing, servo disabled."));
}

void triggerNudge() {
  _nudging       = true;
  _nudgeStep     = 0;
  _lastNudgeTick = millis();
  Serial.println(F("[Servo] Nudge triggered (no hardware)."));
}

void tickServoNudge() {
  if (!_nudging) return;
  if (millis() - _lastNudgeTick < 300) return;
  _lastNudgeTick = millis();
  _nudgeStep++;
  if (_nudgeStep >= NUDGE_SEQ_LEN) {
    _nudging = false;
    Serial.println(F("[Servo] Nudge done."));
    return;
  }
}

#endif

bool isNudging() { return _nudging; }
