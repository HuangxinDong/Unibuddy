#pragma once
#include "config.h"
#include "pet.h"
#include "pomodoro.h"
#include "behaviour.h"

#if defined(__has_include)
  #if __has_include(<Adafruit_GFX.h>) && __has_include(<Adafruit_SSD1306.h>)
    #include <Wire.h>
    #include <Adafruit_GFX.h>
    #include <Adafruit_SSD1306.h>
    #define OLED_LIBS_AVAILABLE 1
  #else
    #define OLED_LIBS_AVAILABLE 0
  #endif
#else
  #define OLED_LIBS_AVAILABLE 0
#endif

#if OLED_LIBS_AVAILABLE
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

void initOLED() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("[OLED] Init failed!"));
    while (true);
  }
  display.clearDisplay();
  display.display();
}

void showSplashScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.println(F("Pocket Buddy"));
  display.setCursor(28, 34);
  display.println(F("waking up..."));
  display.display();
}

// ── Helper: draw time MM:SS ───────────────────────────────────
void drawTimer(uint32_t seconds, uint8_t x, uint8_t y, uint8_t size=2) {
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  uint32_t m = seconds / 60;
  uint32_t s = seconds % 60;
  if (m < 10) display.print('0');
  display.print(m);
  display.print(':');
  if (s < 10) display.print('0');
  display.print(s);
}

// ── Helper: draw progress bar ─────────────────────────────────
void drawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                     float progress) {  // progress 0.0–1.0
  display.drawRect(x, y, w, h, SSD1306_WHITE);
  uint8_t fill = (uint8_t)(progress * (w - 2));
  if (fill > 0)
    display.fillRect(x+1, y+1, fill, h-2, SSD1306_WHITE);
}

// ── Main render ───────────────────────────────────────────────
void renderFrame(int mode) {
  display.clearDisplay();

  // Pet always in top-left 32x32 area (scaled up 2x from 16x16)
  display.drawBitmap(0, 0, getPetBitmap(), 16, 16, SSD1306_WHITE);

  switch (mode) {
    // ── IDLE ──────────────────────────────────────────────────
    case 0: {  // MODE_IDLE
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(22, 2);
      display.println(F("Hold to focus"));
      display.setCursor(22, 14);
      display.print(F("Sessions: "));
      display.println(getSessionCount());
      // Streak bar
      display.setCursor(0, 48);
      display.print(F("Streak "));
      display.print(getStreakDays());
      display.println(F(" days"));
      break;
    }

    // ── POMODORO ──────────────────────────────────────────────
    case 1: {  // MODE_POMODORO
      uint32_t sLeft = pomodoroSecondsLeft();
      drawTimer(sLeft, 28, 0, 2);

      float progress = 1.0f - (float)sLeft / (POMODORO_DURATION / 1000.0f);
      drawProgressBar(0, 52, 128, 10, progress);

      display.setTextSize(1);
      display.setCursor(0, 40);
      display.print(F("#"));
      display.print(getSessionCount() + 1);
      display.print(F("  FOCUS"));
      break;
    }

    // ── BREAK ─────────────────────────────────────────────────
    case 2: {  // MODE_BREAK
      uint32_t sLeft = breakSecondsLeft();
      drawTimer(sLeft, 28, 0, 2);

      display.setTextSize(1);
      display.setCursor(0, 40);
      display.println(F("Break time!"));
      display.setCursor(0, 52);
      display.println(F("Press to skip"));
      break;
    }

    // ── NUDGE ─────────────────────────────────────────────────
    case 3: {  // MODE_NUDGE
      display.setTextSize(1);
      display.setCursor(22, 8);
      display.println(F("Hey! Get back"));
      display.setCursor(22, 20);
      display.println(F("to work! :)"));
      break;
    }

    // ── STATS ─────────────────────────────────────────────────
    case 4: {  // MODE_STATS
      display.setTextSize(1);
      display.setCursor(22, 0);
      display.println(F("Today's Stats"));
      display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

      display.setCursor(0, 14);
      display.print(F("Sessions: "));
      display.println(getSessionCount());

      display.setCursor(0, 26);
      display.print(F("Focus time: "));
      display.print(getSessionCount() * 25);
      display.println(F("m"));

      display.setCursor(0, 38);
      display.print(F("Streak: "));
      display.print(getStreakDays());
      display.println(F(" days"));

      display.setCursor(0, 52);
      display.println(F("Press to go back"));
      break;
    }
  }

  display.display();
}
#else

void initOLED() {
  Serial.println(F("[OLED] Adafruit libs missing, running headless."));
}

void showSplashScreen() {
  Serial.println(F("[OLED] Splash skipped (headless mode)."));
}

void renderFrame(int mode) {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint < 1000) return;
  lastPrint = millis();

  const char* modeNames[] = {"IDLE","FOCUS","BREAK","NUDGE","STATS"};
  Serial.print(F("[UI] "));
  Serial.print(modeNames[mode]);

  if (mode == 1) {  // POMODORO
    Serial.print(F(" remaining="));
    Serial.print(pomodoroSecondsLeft());
    Serial.print(F("s"));
  } else if (mode == 2) {  // BREAK
    Serial.print(F(" remaining="));
    Serial.print(breakSecondsLeft());
    Serial.print(F("s"));
  }

  Serial.print(F(" sessions="));
  Serial.print(getSessionCount());
  Serial.print(F(" streak="));
  Serial.println(getStreakDays());
}

#endif
