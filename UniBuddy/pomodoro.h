#pragma once
/*
 * ────────────────────────────────────────────────────────────
 *  pomodoro.h — Focus & break countdown timers
 *
 *  Call flow:  initPomodoro → startPomodoro → updatePomodoro
 *              → isPomodoroFinished → startBreak → tickBreakTimer
 *              → isBreakFinished  (loop)
 *
 *  Long break triggers every 4th completed cycle.
 * ────────────────────────────────────────────────────────────
 */
#include <Arduino.h>
#include "config.h"

// ── Internal state ──────────────────────────────────────────
static uint32_t _pomStart      = 0;
static uint32_t _pomDuration   = POMODORO_DURATION;
static bool     _pomRunning    = false;
static bool     _pomFinished   = false;

static uint32_t _breakStart    = 0;
static uint32_t _breakDuration = SHORT_BREAK;
static bool     _breakFinished = false;

static uint8_t  _completedCycles = 0;

// ── Focus timer ─────────────────────────────────────────────

void initPomodoro() {
  _pomRunning  = false;
  _pomFinished = false;
}

void startPomodoro() {
  _pomStart    = millis();
  _pomRunning  = true;
  _pomFinished = false;
}

void pausePomodoro() {
  _pomRunning = false;
}

void updatePomodoro() {
  if (!_pomRunning) return;
  if (millis() - _pomStart >= _pomDuration) {
    _pomRunning  = false;
    _pomFinished = true;
    _completedCycles++;
  }
}

bool isPomodoroFinished() {
  if (_pomFinished) { _pomFinished = false; return true; }
  return false;
}

// Returns seconds remaining
uint32_t pomodoroSecondsLeft() {
  if (!_pomRunning) return _pomDuration / 1000;
  uint32_t elapsed = millis() - _pomStart;
  if (elapsed >= _pomDuration) return 0;
  return (_pomDuration - elapsed) / 1000;
}

// ── Break timer ─────────────────────────────────────────────

void startBreak() {
  _breakDuration = (_completedCycles % 4 == 0) ? LONG_BREAK : SHORT_BREAK;
  _breakStart    = millis();
  _breakFinished = false;
}

void tickBreakTimer() {
  if (millis() - _breakStart >= _breakDuration) {
    _breakFinished = true;
  }
}

bool isBreakFinished() {
  if (_breakFinished) { _breakFinished = false; return true; }
  return false;
}

uint32_t breakSecondsLeft() {
  uint32_t elapsed = millis() - _breakStart;
  if (elapsed >= _breakDuration) return 0;
  return (_breakDuration - elapsed) / 1000;
}

// ── Getters ─────────────────────────────────────────────────
uint8_t getCompletedCycleCount() { return _completedCycles; }
