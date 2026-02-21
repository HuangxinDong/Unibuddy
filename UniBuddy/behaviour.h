#pragma once
#include <Arduino.h>

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

// ── EEPROM layout ─────────────────────────────────────────────
// Addr 0:   uint8_t  sessions_today
// Addr 1:   uint8_t  streak_days
// Addr 2-3: uint16_t last_day_stamp  (day count since epoch, approx)
//
// Note: Arduino Uno has 1KB EEPROM. We keep it simple.

#define EE_SESSIONS  0
#define EE_STREAK    1
#define EE_DAY_LO    2
#define EE_DAY_HI    3

static uint8_t  _sessionsToday = 0;
static uint8_t  _streakDays    = 0;

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

uint8_t getSessionCount() { return _sessionsToday; }
uint8_t getStreakDays()    { return _streakDays;    }

// Mood recommendation based on behaviour
// (used by pet.h to auto-set mood)
bool shouldNudge() {
  // Simple heuristic: if no session started in last 45 min
  // For hackathon, just trigger every shake
  return false;
}
