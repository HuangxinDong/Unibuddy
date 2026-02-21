/*
 * ============================================================
 *  UniBuddy — Main Sketch
 *  Board: Arduino Uno Q (MCU side, Zephyr)
 *
 *  Display: Waveshare 2.13" e-Paper V4 (landscape 250×122)
 *  Modules: Pomodoro Timer, Virtual Pet, Behaviour Tracker,
 *           Soft Nudge, Button / Tap / Movement input
 *
 *  Refresh strategy (e-paper friendly):
 *    • Full refresh on mode change  (DisplayPartBaseImage)
 *    • Partial refresh on timer tick (DisplayPart, 1 Hz)
 *    • No refresh when nothing changes
 * ============================================================
 */

#include <SPI.h>
#include "config.h"
#include "epaper.h"        // replaces oled.h
#include "pet.h"
#include "pomodoro.h"
#include "behaviour.h"
#include "input.h"

#if USE_SERVO_NUDGE
#include "servo_arm.h"
#endif

// ── State Machine ───────────────────────────────────────────
enum AppMode {
  MODE_IDLE,
  MODE_POMODORO,
  MODE_BREAK,
  MODE_NUDGE,
  MODE_STATS
};

AppMode currentMode = MODE_IDLE;

// ── Display tracking ────────────────────────────────────────
static int      lastMode        = -1;        // force first full refresh
static uint32_t lastTimerSecond = 0xFFFFFFFF;
static bool     displayDirty    = false;     // set true on any event
static bool     softNudgeActive = false;
static uint32_t softNudgeUntil  = 0;

void startNudge() {
#if USE_SERVO_NUDGE
  triggerNudge();
#else
  softNudgeActive = true;
  softNudgeUntil = millis() + 1500;
  Serial.println(F("[Nudge] Soft nudge triggered (no servo)."));
#endif
}

void tickNudge() {
#if USE_SERVO_NUDGE
  tickServoNudge();
#else
  if (softNudgeActive && millis() >= softNudgeUntil) {
    softNudgeActive = false;
  }
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

  initDisplay();             // e-paper init (FULL mode, Clear)
#if USE_SERVO_NUDGE
  initServoArm();
#endif
  initInput();
  initPomodoro();
  initBehaviour();

  showSplashScreen();
  delay(2000);               // longer pause — e-paper needs time

  deepRefresh(MODE_IDLE);    // paint IDLE screen as partial-refresh base (with flash)
  Serial.println(F("[Buddy] Ready!"));
}

// ── Main Loop ───────────────────────────────────────────────
void loop() {
  InputEvent evt = readInput();

  // Handle mode switching (may change currentMode)
  handleModeSwitch(evt);

  // Update active module
  switch (currentMode) {
    case MODE_IDLE:     updateIdle();     break;
    case MODE_POMODORO: updatePomodoro(); break;
    case MODE_BREAK:    updateBreak();    break;
    case MODE_NUDGE:    updateNudge();    break;
    case MODE_STATS:    updateStats();    break;
  }

  // ── Decide whether the display needs a refresh ───────────
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
    lastTimerSecond = 0xFFFFFFFF;   // reset so first tick triggers partial
  } else if (timerTicked) {
    partialRefresh(currentMode);
  } else if (animTicked) {
    partialRefresh(currentMode);
  }

  delay(50);     // 20 Hz input polling; display updates ≤ 1 Hz
}

// Mode Switching
void handleModeSwitch(InputEvent evt) {
  switch (evt) {
    case EVT_BTN_LONG:          // double tap → toggle pomodoro
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

    case EVT_BTN_SHORT:         // single tap → cycle stats / break ack
      if (currentMode == MODE_BREAK) {
        updatePetMoodFromSessions(getSessionCount());
        currentMode = MODE_IDLE;
      }
      else if (currentMode == MODE_IDLE) currentMode = MODE_STATS;
      else if (currentMode == MODE_STATS) currentMode = MODE_IDLE;
      break;

    case EVT_TAP:               // tap → interested nudge mode / dismiss
      if (currentMode != MODE_NUDGE) {
        setPetMood(MOOD_INTERESTED);
        currentMode = MODE_NUDGE;
        startNudge();
      } else {
        updatePetMoodFromSessions(getSessionCount());
        currentMode = MODE_IDLE;
      }
      break;

    case EVT_MOTION:            // motion → confused nudge mode / dismiss
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

  // Auto-transition: pomodoro finished → break
  if (currentMode == MODE_POMODORO && isPomodoroFinished()) {
    recordSession();            // behaviour tracker
    if (getSessionCount() >= 6) setPetMood(MOOD_TIRED);
    else                        setPetMood(MOOD_HAPPY);
    startBreak();
    currentMode = MODE_BREAK;
    setPetMood(MOOD_ASLEEP);
    startNudge();               // celebrate alert (soft nudge / servo)
  }
}

// Idle
void updateIdle() {
  // Animation tick handled in main refresh decision
}

// Break
void updateBreak() {
  tickBreakTimer();
  if (isBreakFinished()) {
    setPetMood(MOOD_FOCUSED);
    currentMode = MODE_POMODORO;
    startPomodoro();
  }
}

// Nudge
void updateNudge() {
  tickNudge();
  if (!isNudgeActive()) {
    updatePetMoodFromSessions(getSessionCount());
    currentMode = MODE_IDLE;
  }
}

// Stats
void updateStats() {
  // Behaviour tracker shows sessions today, streak, mood
  // Rendered inside renderFrame()
}
