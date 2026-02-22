#pragma once
/*
 * ============================================================
 *  tilt.h — Modulino Movement: tilt + shake detection
 *
 *  Robust 3-layer design:
 *    1. EMA low-pass filter on accel → stable tilt readings
 *    2. Shake detection via raw magnitude spike
 *    3. Shake lockout → tilt classification frozen for 800ms
 *       after a shake to prevent false mode switches
 * ============================================================
 */
#include <Arduino.h>
#include <math.h>
#include "Modulino.h"
#include "config.h"

#ifndef ROTATE_0
  #define ROTATE_0    0
  #define ROTATE_90   1
  #define ROTATE_180  2
  #define ROTATE_270  3
#endif

static ModulinoMovement _imu;

// ── Raw accelerometer (shake pipeline only) ─────────────────
static float _rawX, _rawY, _rawZ;
static float _rawMag = 1.0f;
static float _prevRawX = 0.0f, _prevRawY = 0.0f, _prevRawZ = 1.0f;
static float _rawJerk = 0.0f;

// ── Posture pipeline (for mode switching only) ─────────────
// Low-pass filtered gravity estimate
static float _lpX, _lpY, _lpZ;
static float _lpMag = 1.0f;
static bool  _lpInit = false;
static const float LP_ALPHA = 0.10f;

// Unit gravity direction
static float _gX = 0.0f, _gY = 0.0f, _gZ = 1.0f;

// Orientation angles from gravity vector (degrees)
static float _tRoll = 0.0f;
static float _tPitch = 0.0f;
static float _tYawProxy = 0.0f;   // proxy only (accelerometer cannot give true yaw)

// Posture reliability / stillness gate (used for mode switching)
static bool _postureReliable = false;
static uint32_t _stableSince = 0;
static const float STABLE_MAG_MIN = 0.78f;
static const float STABLE_MAG_MAX = 1.25f;
static const float STABLE_JERK_MAX = 0.22f;
static const uint16_t STABLE_MIN_MS = 140;

// ── Shake detection ─────────────────────────────────────────
static bool     _shakeDetected    = false;
static uint32_t _lastShakeEdge    = 0;
static uint32_t _shakeLockoutEnd  = 0;
static float    _magEma           = 1.0f;

static const float    SHAKE_HP_THRESH    = 0.55f;  // |rawMag - magEma|
static const uint16_t SHAKE_COOLDOWN_MS  = 300;     // min gap between shakes
static const uint16_t SHAKE_LOCKOUT_MS   = 900;     // tilt frozen after shake

void initTilt() {
  Modulino.begin();
  _imu.begin();
  Serial.println(F("[Tilt] Modulino Movement ready"));
}

void updateTilt() {
  _imu.update();
  _rawX = _imu.getX();
  _rawY = _imu.getY();
  _rawZ = _imu.getZ();

  // ── Raw magnitude / jerk (shake-only observability) ──────
  _rawMag = sqrt(_rawX * _rawX + _rawY * _rawY + _rawZ * _rawZ);
  float dx = _rawX - _prevRawX;
  float dy = _rawY - _prevRawY;
  float dz = _rawZ - _prevRawZ;
  _rawJerk = sqrt(dx * dx + dy * dy + dz * dz);
  _prevRawX = _rawX;
  _prevRawY = _rawY;
  _prevRawZ = _rawZ;

  // ── Posture low-pass (gravity estimate) ───────────────────
  if (!_lpInit) {
    _lpX = _rawX; _lpY = _rawY; _lpZ = _rawZ;
    _lpInit = true;
  } else {
    _lpX += LP_ALPHA * (_rawX - _lpX);
    _lpY += LP_ALPHA * (_rawY - _lpY);
    _lpZ += LP_ALPHA * (_rawZ - _lpZ);
  }
  _lpMag = sqrt(_lpX * _lpX + _lpY * _lpY + _lpZ * _lpZ);

  // Normalize to unit gravity direction
  if (_lpMag > 0.0001f) {
    _gX = _lpX / _lpMag;
    _gY = _lpY / _lpMag;
    _gZ = _lpZ / _lpMag;
  }

  // ── Roll / pitch / yaw-proxy from filtered gravity ───────
  _tRoll  = atan2(_gY, _gZ) * 180.0f / M_PI;
  _tPitch = atan2(-_gX, sqrt(_gY * _gY + _gZ * _gZ))
            * 180.0f / M_PI;
  _tYawProxy = atan2(_gY, _gX) * 180.0f / M_PI;

  // ── Posture reliability gate (mode switching only) ───────
  uint32_t now = millis();
  bool instantStable =
      (_lpMag >= STABLE_MAG_MIN && _lpMag <= STABLE_MAG_MAX) &&
      (_rawJerk <= STABLE_JERK_MAX);

  if (instantStable) {
    if (_stableSince == 0) _stableSince = now;
  } else {
    _stableSince = 0;
  }

  _postureReliable = (_stableSince != 0) && (now - _stableSince >= STABLE_MIN_MS);

  // ── Shake detection (independent high-pass magnitude) ─────
  _magEma += 0.10f * (_rawMag - _magEma);
  float magHp = fabs(_rawMag - _magEma);
  _shakeDetected = false;

  if (magHp > SHAKE_HP_THRESH && now - _lastShakeEdge > SHAKE_COOLDOWN_MS) {
    _shakeDetected   = true;
    _lastShakeEdge   = now;
    _shakeLockoutEnd = now + SHAKE_LOCKOUT_MS;
    _stableSince = 0;
    _postureReliable = false;
    Serial.print(F("[Tilt] SHAKE hp="));
    Serial.print(magHp, 2);
    Serial.print(F(" raw="));
    Serial.println(_rawMag, 2);
  }
}

// ── Getters ─────────────────────────────────────────────────
float getRollDeg()        { return _tRoll; }
float getPitchDeg()       { return _tPitch; }
float getYawProxyDeg()    { return _tYawProxy; }
float getAccZ()           { return _gZ; }      // normalized filtered gravity Z
bool  wasShakeDetected()  { return _shakeDetected; }

/*
 * isTiltReliable() — false during & shortly after shaking.
 * Callers MUST skip tilt-based mode switching when false.
 */
bool isTiltReliable() {
  return _postureReliable && (millis() >= _shakeLockoutEnd);
}

/* classify tilt with hysteresis — sticky current mode.
 *   • Leaving the current mode requires crossing threshold + HYSTERESIS.
 *   • Calendar zones are narrower and harder to enter.
 *   • Sleep zone is wider.
 */
AppMode classifyTilt(AppMode cur) {
  if (!isTiltReliable()) return cur;

  float r  = _tRoll;
  float p  = _tPitch;
  float az = _gZ;
  float H  = TILT_HYSTERESIS;   // 10° extra to leave current mode
  float calEnterLo = TILT_CAL_PITCH_CENTER - TILT_CAL_PITCH_WINDOW;
  float calEnterHi = TILT_CAL_PITCH_CENTER + TILT_CAL_PITCH_WINDOW;
  float calLeaveLo = calEnterLo - TILT_CAL_HYSTERESIS;
  float calLeaveHi = calEnterHi + TILT_CAL_HYSTERESIS;

  bool inCalPosEnter = (p >= calEnterLo && p <= calEnterHi);
  bool inCalNegEnter = (p <= -calEnterLo && p >= -calEnterHi);
  bool inCalPosLeave = (p >= calLeaveLo && p <= calLeaveHi);
  bool inCalNegLeave = (p <= -calLeaveLo && p >= -calLeaveHi);

  // face-down always takes priority
  if (az < TILT_FACEDOWN_Z)  return MODE_FACEDOWN;

  // --- CALENDAR zones: only around y-axis side orientation (±90 pitch) ---
  if (cur == MODE_TEMPTIME_L && inCalNegLeave) return MODE_TEMPTIME_L;
  if (cur == MODE_TEMPTIME_R && inCalPosLeave) return MODE_TEMPTIME_R;
  if (inCalNegEnter) return MODE_TEMPTIME_L;
  if (inCalPosEnter) return MODE_TEMPTIME_R;

  // --- PET (roll < -70) with hysteresis ---
  if (cur == MODE_PET) {
    // stay in PET until roll rises above PET + H = -60
    if (r < TILT_ROLL_PET + H) return MODE_PET;
  } else {
    if (r < TILT_ROLL_PET) return MODE_PET;
  }

  // --- POMODORO (roll > 70) with hysteresis ---
  if (cur == MODE_POMODORO || cur == MODE_BREAK) {
    // stay until roll drops below POMO - H = 60
    if (r > TILT_ROLL_POMO - H) return cur;
  } else {
    if (r > TILT_ROLL_POMO) return MODE_POMODORO;
  }

  // --- SLEEP (-35..35) with hysteresis ---
  if (cur == MODE_SLEEP) {
    // stay until roll leaves (-35-H .. 35+H) = (-45..45)
    if (r > TILT_ROLL_SLEEP_LO - H && r < TILT_ROLL_SLEEP_HI + H)
      return MODE_SLEEP;
  } else {
    if (r > TILT_ROLL_SLEEP_LO && r < TILT_ROLL_SLEEP_HI)
      return MODE_SLEEP;
  }

  // fallback: stay in current mode if nothing else matched
  return cur;
}

/* choose display rotation for a given mode */
int rotationForMode(AppMode mode, AppMode prev) {
  switch (mode) {
    case MODE_PET:        return ROTATE_270;
    case MODE_POMODORO:
    case MODE_BREAK:      return ROTATE_90;
    case MODE_TEMPTIME_L: return ROTATE_0;
    case MODE_TEMPTIME_R: return ROTATE_180;
    case MODE_SLEEP:
      if (prev == MODE_POMODORO || prev == MODE_BREAK || prev == MODE_TEMPTIME_R)
        return ROTATE_90;
      return ROTATE_270;
    default:              return ROTATE_270;
  }
}
