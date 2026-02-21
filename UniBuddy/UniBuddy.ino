/*
 * ════════════════════════════════════════════════════════════
 *  UniBuddy.ino — Main sketch (state machine + loop)
 *
 *  Board   : Arduino Uno Q (MCU side, Zephyr)
 *  Display : Waveshare 2.13" e-Paper V4 (landscape 250×122)
 *  Modules : config  → shared pins, constants, AppMode enum
 *            epaper  → display init / refresh / UI rendering
 *            input   → button debounce + shake sensor
 *            pomodoro→ focus & break countdown timers
 *            pet     → mood, sprites, blink animation
 *            behaviour → session tracker, streak persistence
 *            servo_arm → SG90 nudge wave animation
 *
 *  Refresh strategy (e-paper friendly)
 *  ──────────────────────────────────
 *  • Full refresh  on mode change  (DisplayPartBaseImage)
 *  • Partial refresh on timer tick  (DisplayPart, ≤ 1 Hz)
 *  • No refresh when nothing changes
 * ════════════════════════════════════════════════════════════
 */

#include <SPI.h>
#include "config.h"          // pins, timing, AppMode enum
#include "epaper.h"          // display driver & UI renderer
#include "pet.h"             // virtual pet sprites & mood
#include "pomodoro.h"        // focus / break timers
#include "servo_arm.h"       // SG90 servo nudge
#include "behaviour.h"       // session & streak persistence
#include "input.h"           // button + shake sensor

// ── Runtime state ───────────────────────────────────────────
static AppMode currentMode      = MODE_IDLE;
static int     _lastMode        = -1;           // force first full refresh
static uint32_t _lastTimerSecond = 0xFFFFFFFF;

// ── Forward declarations ────────────────────────────────────
void handleModeSwitch(InputEvent evt);
void updateIdle();
void updatePomodoro();
void updateBreak();
void updateNudge();
void updateStats();

// ── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  initDisplay();
  initServoArm();
  initInput();
  initPomodoro();
  initBehaviour();

  showSplashScreen();
  delay(2000);               // e-paper needs visible hold time

  fullRefresh(MODE_IDLE);    // paint IDLE screen as partial base
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

  // ── Display refresh logic ────────────────────────────────
  bool modeChanged = ((int)currentMode != _lastMode);
  bool timerTicked = false;

  if (currentMode == MODE_POMODORO) {
    uint32_t sec = pomodoroSecondsLeft();
    if (sec != _lastTimerSecond) { _lastTimerSecond = sec; timerTicked = true; }
  } else if (currentMode == MODE_BREAK) {
    uint32_t sec = breakSecondsLeft();
    if (sec != _lastTimerSecond) { _lastTimerSecond = sec; timerTicked = true; }
  }

  if (modeChanged) {
    fullRefresh(currentMode);
    _lastMode = (int)currentMode;
    _lastTimerSecond = 0xFFFFFFFF;   // reset so first tick triggers partial
  } else if (timerTicked) {
    partialRefresh(currentMode);
  }

  delay(50);     // 20 Hz input polling; display updates ≤ 1 Hz
}

// ── Mode Switching ──────────────────────────────────────────
void handleModeSwitch(InputEvent evt) {
  switch (evt) {
    case EVT_BTN_LONG:          // long press → toggle pomodoro
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

    case EVT_BTN_SHORT:         // short press → cycle stats / break ack
      if (currentMode == MODE_BREAK) {
        updatePetMoodFromSessions(getSessionCount());
        currentMode = MODE_IDLE;
      }
      else if (currentMode == MODE_IDLE) currentMode = MODE_STATS;
      else if (currentMode == MODE_STATS) currentMode = MODE_IDLE;
      break;

    case EVT_SHAKE:             // shake → nudge mode / dismiss
      if (currentMode != MODE_NUDGE) {
        setPetMood(MOOD_EXCITED);
        currentMode = MODE_NUDGE;
        triggerNudge();
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
    setPetMood(MOOD_HAPPY);
    startBreak();
    currentMode = MODE_BREAK;
    triggerNudge();             // arm wave to celebrate!
  }
}

// ── Per-mode update functions ───────────────────────────────
void updateIdle() {
  tickPetAnimation();
}

void updatePomodoro() {
  // timer ticks are checked in loop() display logic
}

void updateBreak() {
  tickBreakTimer();
  if (isBreakFinished()) {
    setPetMood(MOOD_FOCUSED);
    currentMode = MODE_POMODORO;
    startPomodoro();
  }
}

void updateNudge() {
  tickServoNudge();             // servo wave animation
  if (!isNudging()) {
    updatePetMoodFromSessions(getSessionCount());
    currentMode = MODE_IDLE;
  }
}

void updateStats() {
  // static screen — rendered by renderToBuffer(MODE_STATS)
}
