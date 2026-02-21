#pragma once

// ── Test Mode ─────────────────────────────────────────────────
// Uncomment the line below for rapid testing (10s focus / 3s break)
// #define TEST_MODE

// ── E-Paper Display (Waveshare 2.13" V4, 250×122, SPI) ───────
// Pin mapping defined in epdif.h: RST=D8, DC=D9, CS=D10, BUSY=D7
// SPI: DIN=D11(MOSI), CLK=D13(SCK)
// Landscape dimensions after rotation:
#define DISPLAY_WIDTH   250
#define DISPLAY_HEIGHT  122

// Pins
#define PIN_BUTTON        2     // main button (INPUT_PULLUP)
#define PIN_SERVO         6     // SG90 signal (moved from D9; D9 is e-paper DC)
#define PIN_SHAKE_SW      3     // SW-420 vibration sensor (or MPU-6050 INT)

// Pomodoro Durations (ms)
#ifdef TEST_MODE
  #define POMODORO_DURATION  (10UL * 1000)        // 10 sec
  #define SHORT_BREAK        (3UL  * 1000)         // 3 sec
  #define LONG_BREAK         (5UL  * 1000)         // 5 sec
#else
  #define POMODORO_DURATION  (25UL * 60 * 1000)    // 25 min
  #define SHORT_BREAK        (5UL  * 60 * 1000)    // 5 min
  #define LONG_BREAK         (15UL * 60 * 1000)    // 15 min (every 4 sessions)
#endif

// Tap sensor timing
#define TAP_DEBOUNCE_MS       80    // ignore bounces shorter than this
#define DOUBLE_TAP_WINDOW_MS  400   // max gap between taps for double-tap

// Servo
#define SERVO_REST_ANGLE  0
#define SERVO_WAVE_ANGLE  90
#define SERVO_NUDGE_ANGLE 45

// Behaviour
#define MAX_SESSIONS_STORED  20   // rolling buffer in EEPROM