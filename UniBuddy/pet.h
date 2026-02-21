#pragma once
/*
 * ────────────────────────────────────────────────────────────
 *  pet.h — Virtual-pet mood & pixel-art sprites
 *
 *  Each sprite is a 16×16 monochrome bitmap (32 bytes, PROGMEM).
 *  The animation system toggles between two frames for blink.
 * ────────────────────────────────────────────────────────────
 */
#include <Arduino.h>

// ── Pet emotions ─────────────────────────────────────────────
enum PetMood {
  MOOD_HAPPY,
  MOOD_INTERESTED,
  MOOD_SAD,
  MOOD_ANGRY,
  MOOD_CONFUSED,
  MOOD_DESPISED,
  MOOD_FOCUSED,
  MOOD_TIRED,
  MOOD_ASLEEP,
};

// ── Internal state ──────────────────────────────────────────
static PetMood  _mood          = MOOD_HAPPY;
static uint8_t  _animPhase     = 0;
static uint32_t _lastAnimTick  = 0;

const uint8_t PET_ANIM_PHASES = 8;
const uint16_t PET_ANIM_INTERVALS[PET_ANIM_PHASES] = {
  1800, // open center
  350,  // glance left
  350,  // center
  350,  // glance right
  1200, // center
  140,  // blink half
  120,  // blink closed
  140   // blink half
};

bool tickPetAnimation() {
  uint16_t interval = PET_ANIM_INTERVALS[_animPhase];
  if (millis() - _lastAnimTick < interval) return false;
  _lastAnimTick = millis();
  _animPhase = (_animPhase + 1) % PET_ANIM_PHASES;
  return true;
}

void setPetMood(PetMood mood) {
  _mood = mood;
  _animPhase = 0;
  _lastAnimTick = millis();
}

PetMood getPetMood() { return _mood; }

const char* getPetMoodName() {
  switch (_mood) {
    case MOOD_HAPPY:      return "HAPPY";
    case MOOD_INTERESTED: return "INTERESTED";
    case MOOD_SAD:        return "SAD";
    case MOOD_ANGRY:      return "ANGRY";
    case MOOD_CONFUSED:   return "CONFUSED";
    case MOOD_DESPISED:   return "DESPISED";
    case MOOD_TIRED:      return "TIRED";
    case MOOD_ASLEEP:     return "ASLEEP";
    case MOOD_FOCUSED:
    default:              return "FOCUSED";
  }
}

uint8_t getPetAnimPhase() {
  return _animPhase;
}

int8_t getPetEyeOffsetX() {
  if (_animPhase == 1) return -4;
  if (_animPhase == 3) return 4;
  return 0;
}

uint8_t getPetBlinkLevel() {
  if (_animPhase == 6) return 2; // closed
  if (_animPhase == 5 || _animPhase == 7) return 1; // half
  return 0; // open
}

// Call after N sessions to update a default ambient emotion
void updatePetMoodFromSessions(uint8_t sessions) {
  if      (sessions == 0) setPetMood(MOOD_HAPPY);
  else if (sessions <= 1) setPetMood(MOOD_INTERESTED);
  else if (sessions <= 3) setPetMood(MOOD_FOCUSED);
  else if (sessions <= 5) setPetMood(MOOD_HAPPY);
  else if (sessions <= 7) setPetMood(MOOD_TIRED);
  else                    setPetMood(MOOD_SAD);
}
