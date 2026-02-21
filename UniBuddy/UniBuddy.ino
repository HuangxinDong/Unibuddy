/*
 * ============================================================
 *  UniBuddy.ino - Main sketch (state machine + loop)
 *  Board: Arduino Uno Q (MCU side, Zephyr)
 *  Display: Waveshare 2.13" e-Paper V4 (landscape 250x122)
 * ============================================================
 */

#include <SPI.h>
#include "config.h"
#include "epaper.h"
#include "pet.h"
#include "pomodoro.h"
#include "behaviour.h"
#include "input.h"

#if USE_SERVO_NUDGE
#include "servo_arm.h"
#endif

// ── Runtime state ───────────────────────────────────────────
AppMode  currentMode      = MODE_IDLE;
static int      lastMode        = -1;
static uint32_t lastTimerSecond = 0xFFFFFFFF;
static bool     softNudgeActive = false;
static uint32_t softNudgeUntil  = 0;

// ── Nudge helpers (soft fallback when no servo) ─────────────
void startNudge() {
#if USE_SERVO_NUDGE
  triggerNudge();
#else
  softNudgeActive = true;
  softNudgeUntil = millis() + 1500;
  Serial.println(F("[Nudge] Soft nudge (no servo)."));
#endif
}

void tickNudge() {
#if USE_SERVO_NUDGE
  tickServoNudge();
#else
  if (softNudgeActive && millis() >= softNudgeUntil)
    softNudgeActive = false;
#endif
}

bool isNudgeActive() {
#if USE_SERVO_NUDGE
  return isNudging();
#else
  return softNudgeActive;
#endif
}

// ── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  initDisplay();
#if USE_SERVO_NUDGE
  initServoArm();
#endif
  initInput();
  initPomodoro();
  initBehaviour();

  showSplashScreen();
  delay(2000);

  deepRefresh(MODE_IDLE);
  Serial.println(F("[Buddy] Ready!"));
}

// ── Main Loop ───────────────────────────────────────────────
void loop() {
  InputEvent evt = readInput();
  handleModeSwitch(evt);

  switch (currentMode) {
    case MODE_IDLE:     updateIdle();     break;
    case MODE_POMODORO: updatePomodoro(); break;
    case MODE_BREAK:    updateBreak();    break;
    case MODE_NUDGE:    updateNudge();    break;
    case MODE_STATS:    updateStats();    break;
  }

  bool modeChanged = ((int)currentMode != lastMode);
  bool timerTicked = false;
  bool animTicked  = tickPetAnimation();

  if (currentMode == MODE_POMODORO) {
    uint32_t sec = pomodoroSecondsLeft();
    if (sec != lastTimerSecond) { lastTimerSecond = sec; timerTicked = true; }
  } else if (currentMode == MODE_BREAK) {
    uint32_t sec = breakSecondsLeft();
    if (sec != lastTimerSecond) { lastTimerSecond = sec; timerTicked = true; }
  }

  if (modeChanged) {
    fullRefresh(currentMode);
    lastMode = (int)currentMode;
    lastTimerSecond = 0xFFFFFFFF;
  } else if (timerTicked) {
    partialRefresh(currentMode);
  } else if (animTicked) {
    partialRefresh(currentMode);
  }

  delay(50);
}

// ── Mode Switching ──────────────────────────────────────────
void handleModeSwitch(InputEvent evt) {
  switch (evt) {
    case EVT_BTN_LONG:
    case EVT_DOUBLE_TAP:
      if (currentMode == MODE_IDLE) {
        startPomodoro();
        setPetMood(MOOD_FOCUSED);
        currentMode = MODE_POMODORO;
      } else if (currentMode == MODE_POMODORO) {
        pausePomodoro();
        updatePetMoodFromSessions(getSessionCount());
        currentMode = MODE_IDLE;
      }
      break;

    case EVT_BTN_SHORT:
      if (currentMode == MODE_BREAK) {
        updatePetMoodFromSessions(getSessionCount());
        currentMode = MODE_IDLE;
      }
      else if (currentMode == MODE_IDLE) currentMode = MODE_STATS;
      else if (currentMode == MODE_STATS) currentMode = MODE_IDLE;
      break;

    case EVT_TAP:
      if (currentMode != MODE_NUDGE) {
        setPetMood(MOOD_INTERESTED);
        currentMode = MODE_NUDGE;
        startNudge();
      } else {
        updatePetMoodFromSessions(getSessionCount());
        currentMode = MODE_IDLE;
      }
      break;

    case EVT_MOTION:
      if (currentMode != MODE_NUDGE) {
        setPetMood(MOOD_CONFUSED);
        currentMode = MODE_NUDGE;
        startNudge();
      } else {
        updatePetMoodFromSessions(getSessionCount());
        currentMode = MODE_IDLE;
      }
      break;

    default: break;
  }

  if (currentMode == MODE_POMODORO && isPomodoroFinished()) {
    recordSession();
    if (getSessionCount() >= 6) setPetMood(MOOD_TIRED);
    else                        setPetMood(MOOD_HAPPY);
    startBreak();
    currentMode = MODE_BREAK;
    setPetMood(MOOD_ASLEEP);
    startNudge();
  }
}

// ── Per-mode updates ────────────────────────────────────────
void updateIdle() {}

void updateBreak() {
  tickBreakTimer();
  if (isBreakFinished()) {
    setPetMood(MOOD_FOCUSED);
    currentMode = MODE_POMODORO;
    startPomodoro();
  }
}

void updateNudge() {
  tickNudge();
  if (!isNudgeActive()) {
    updatePetMoodFromSessions(getSessionCount());
    currentMode = MODE_IDLE;
  }
}

void updateStats() {}
