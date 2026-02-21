#pragma once
/*
 * ────────────────────────────────────────────────────────────
 *  behaviour.h — Session counter & streak persistence
 *
 *  Uses EEPROM when available; falls back to RAM-only mode.
 *  EEPROM layout (4 bytes total):
 *    Addr 0   uint8_t   sessions_today
 *    Addr 1   uint8_t   streak_days
 *    Addr 2-3 uint16_t  last_day_stamp (unused; future)
 * ────────────────────────────────────────────────────────────
 */
#include <Arduino.h>

// ── EEPROM availability (auto-detected) ─────────────────────
#if defined(__has_include)
  #if __has_include(<EEPROM.h>)
    #include <EEPROM.h>
    #define EEPROM_LIB_AVAILABLE 1
  #else
    #define EEPROM_LIB_AVAILABLE 0
  #endif
#else
  #define EEPROM_LIB_AVAILABLE 0
#endif

// ── EEPROM addresses ────────────────────────────────────────
#define EE_SESSIONS  0
#define EE_STREAK    1
#define EE_DAY_LO    2
#define EE_DAY_HI    3

// ── Internal state ──────────────────────────────────────────
static uint8_t _sessionsToday = 0;
static uint8_t _streakDays    = 0;

// ── Public API ──────────────────────────────────────────────

void initBehaviour() {
#if EEPROM_LIB_AVAILABLE
  _sessionsToday = EEPROM.read(EE_SESSIONS);
  _streakDays    = EEPROM.read(EE_STREAK);
#else
  _sessionsToday = 0;
  _streakDays    = 0;
  Serial.println(F("[Behaviour] EEPROM missing, persistence disabled."));
#endif

  // Sanity check
  if (_sessionsToday > 20) _sessionsToday = 0;
  if (_streakDays    > 30) _streakDays    = 0;
}

void recordSession() {
  _sessionsToday++;
#if EEPROM_LIB_AVAILABLE
  EEPROM.update(EE_SESSIONS, _sessionsToday);
#endif
  Serial.print(F("[Behaviour] Sessions today: "));
  Serial.println(_sessionsToday);
}

// Call this once per day (e.g. on first boot of the day)
// For hackathon: just call it manually or skip
void incrementStreak() {
  _streakDays++;
#if EEPROM_LIB_AVAILABLE
  EEPROM.update(EE_STREAK, _streakDays);
#endif
}

void resetStreak() {
  _streakDays = 0;
#if EEPROM_LIB_AVAILABLE
  EEPROM.update(EE_STREAK, 0);
#endif
}

// ── Getters ─────────────────────────────────────────────────
uint8_t getSessionCount() { return _sessionsToday; }
uint8_t getStreakDays()    { return _streakDays;    }
