/*
 * ============================================================
 *  UniBuddy — Main Sketch
 *  Board: Arduino Uno Q (MCU side, Zephyr)
 *
 *  Display: Waveshare 2.13" e-Paper V4 (landscape 250×122)
 *  Modules: Pomodoro Timer, Virtual Pet, Behaviour Tracker,
 *           Servo Arm (nudge), Button / Shake input
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
#include "servo_arm.h"
#include "behaviour.h"
#include "input.h"

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

// ── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  initDisplay();             // e-paper init (FULL mode, Clear)
  initServoArm();
  initInput();
  initPomodoro();
  initBehaviour();

  showSplashScreen();
  delay(2000);               // longer pause — e-paper needs time

  fullRefresh(MODE_IDLE);    // paint IDLE screen as partial-refresh base
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
  }

  delay(50);     // 20 Hz input polling; display updates ≤ 1 Hz
}

// Mode Switching
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

// Idle
void updateIdle() {
  tickPetAnimation();           // pet breathes / blinks
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
  tickServoNudge();             // servo wave animation
  if (!isNudging()) {
    updatePetMoodFromSessions(getSessionCount());
    currentMode = MODE_IDLE;
  }
}

// Stats
void updateStats() {
  // Behaviour tracker shows sessions today, streak, mood
  // Rendered inside renderFrame()
}
