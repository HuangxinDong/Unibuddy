#pragma once
/*
 * ============================================================
 *  config.h - Central configuration for UniBuddy
 * ============================================================
 */

// Uncomment for rapid testing (10s focus / 3s break / 5s long break)
// #define TEST_MODE

enum AppMode {
  MODE_IDLE,
  MODE_POMODORO,
  MODE_BREAK,
  MODE_NUDGE,
  MODE_STATS
};

// Pin assignments
// E-Paper SPI: RST=D8 DC=D9 CS=D10 BUSY=D7 DIN=D11 CLK=D13
#define PIN_BUTTON        4     // main button (INPUT_PULLUP)
#define PIN_TAP_KY031     2     // KY031 knock/tap sensor
#define PIN_MOVEMENT      3     // Movement sensor trigger

#define USE_SERVO_NUDGE   0     // 1 when SG90 wired

// Display (landscape after ROTATE_270)
#define DISPLAY_WIDTH   250
#define DISPLAY_HEIGHT  122

// Pomodoro durations (ms)
#ifdef TEST_MODE
  #define POMODORO_DURATION  (10UL * 1000)
  #define SHORT_BREAK        ( 3UL * 1000)
  #define LONG_BREAK         ( 5UL * 1000)
#else
  #define POMODORO_DURATION  (25UL * 60 * 1000)
  #define SHORT_BREAK        ( 5UL * 60 * 1000)
  #define LONG_BREAK         (15UL * 60 * 1000)
#endif

// Button timing (ms)
#define BTN_DEBOUNCE_MS    50
#define BTN_LONG_PRESS_MS  600

// Servo
#define SERVO_REST_ANGLE   0
#define SERVO_WAVE_ANGLE   90
#define SERVO_NUDGE_ANGLE  45

// Behaviour / EEPROM
#define MAX_SESSIONS_STORED  20
