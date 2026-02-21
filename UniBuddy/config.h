#pragma once
/*
 * ============================================================
 *  config.h — Central configuration for UniBuddy
 *
 *  All pin assignments, timing constants, shared enums, and
 *  compile-time toggles live here. Every other module includes
 *  this file; nothing else should define hardware constants.
 * ============================================================
 */

// ── Compile-time toggles ────────────────────────────────────
// Uncomment for rapid testing (10 s focus / 3 s break / 5 s long break)
// #define TEST_MODE

// ── Application state machine ───────────────────────────────
enum AppMode {
  MODE_IDLE,
  MODE_POMODORO,
  MODE_BREAK,
  MODE_NUDGE,
  MODE_STATS
};

// ── Pin assignments ─────────────────────────────────────────
//  E-Paper SPI is handled by epdif.h: RST=D8 DC=D9 CS=D10 BUSY=D7
//  SPI data: DIN=D11 (MOSI), CLK=D13 (SCK)
#define PIN_BUTTON        2     // push-button / tap sensor (INPUT_PULLUP)
#define PIN_SHAKE_SW      3     // SW-420 vibration sensor  (INPUT_PULLUP)
#define PIN_SERVO         6     // SG90 signal (D9 used by e-paper DC)

// ── Display (Waveshare 2.13" e-Paper V4, landscape) ────────
#define DISPLAY_WIDTH   250     // pixels, after ROTATE_270
#define DISPLAY_HEIGHT  122

// ── Pomodoro timing (ms) ────────────────────────────────────
#ifdef TEST_MODE
  #define POMODORO_DURATION  (10UL * 1000)         // 10 s
  #define SHORT_BREAK        ( 3UL * 1000)         //  3 s
  #define LONG_BREAK         ( 5UL * 1000)         //  5 s
#else
  #define POMODORO_DURATION  (25UL * 60 * 1000)    // 25 min
  #define SHORT_BREAK        ( 5UL * 60 * 1000)    //  5 min
  #define LONG_BREAK         (15UL * 60 * 1000)    // 15 min (every 4th cycle)
#endif

// ── Button timing (ms) ─────────────────────────────────────
#define BTN_DEBOUNCE_MS    50
#define BTN_LONG_PRESS_MS  600

// ── Servo angles ────────────────────────────────────────────
#define SERVO_REST_ANGLE   0
#define SERVO_WAVE_ANGLE   90
#define SERVO_NUDGE_ANGLE  45

// ── Behaviour / EEPROM ──────────────────────────────────────
#define MAX_SESSIONS_STORED  20