#pragma once
/*
 * ============================================================
 *  calendar.h — RTC + Thermo sensor logic for calendar mode
 *
 *  DS1307 RTC via RTClib  +  Modulino Thermo
 *  Provides date/time/temperature/humidity data to the
 *  e-paper renderer.
 * ============================================================
 */

#include <Wire.h>
#include <RTClib.h>
#include <Modulino.h>

// ── Calendar data buffers (read by epaper.h) ────────────────
static char _dayBuf[12]  = "--";
static char _dateBuf[20] = "----/--/--";
static char _timeBuf[8]  = "--:--";
static float _tempC  = NAN;
static float _humPct = NAN;

// ── Internal state ──────────────────────────────────────────
static RTC_DS1307   _rtc;
static bool         _rtcReady    = false;
static ModulinoThermo _thermo;
static bool         _thermoReady = false;
static uint32_t     _lastCalReadMs = 0;

// ── Helpers ─────────────────────────────────────────────────

const char* dayNameByIndex(uint8_t idx) {
  static const char* NAMES[7] = {
    "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
    "THURSDAY", "FRIDAY", "SATURDAY"
  };
  return (idx < 7) ? NAMES[idx] : "UNKNOWN";
}

// ── Init — call once in setup() AFTER Wire.begin() ─────────

void initCalendarSensors() {
  // Thermo — check return value like the working test
  _thermoReady = _thermo.begin();
  if (!_thermoReady) {
    Serial.println(F("[CAL] Thermo sensor not found!"));
  } else {
    Serial.println(F("[CAL] Thermo sensor OK."));
    delay(500);  // let sensor settle before first read
  }

  // RTC — use the same proven pattern from the standalone test
  _rtcReady = _rtc.begin();
  if (!_rtcReady) {
    Serial.println(F("[CAL] DS1307 not found on I2C bus!"));
    return;
  }
  Serial.println(F("[CAL] DS1307 found."));

  if (!_rtc.isrunning()) {
    Serial.println(F("[CAL] RTC not running — setting to compile time..."));
    _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    delay(200);
    Serial.println(F("[CAL] RTC time set."));
  } else {
    Serial.println(F("[CAL] RTC already running."));
  }
}

// ── Poll — call periodically (rate-limited to 1 Hz) ────────

void updateCalendarReadings() {
  uint32_t nowMs = millis();
  if (nowMs - _lastCalReadMs < 1000) return;
  _lastCalReadMs = nowMs;

  // Thermo — read directly (no update() needed, matches working test)
  if (_thermoReady) {
    _tempC  = _thermo.getTemperature();
    _humPct = _thermo.getHumidity();
  }

  // RTC
  if (_rtcReady) {
    DateTime now = _rtc.now();
    snprintf(_dayBuf,  sizeof(_dayBuf),  "%s",
             dayNameByIndex(now.dayOfTheWeek()));
    snprintf(_dateBuf, sizeof(_dateBuf), "%04d-%02d-%02d",
             now.year(), now.month(), now.day());
    snprintf(_timeBuf, sizeof(_timeBuf), "%02d:%02d",
             now.hour(), now.minute());
    return;
  }

  // Fallback when no RTC
  snprintf(_dayBuf,  sizeof(_dayBuf),  "NO RTC");
  snprintf(_dateBuf, sizeof(_dateBuf), "----/--/--");
  snprintf(_timeBuf, sizeof(_timeBuf), "--:--");
}
