#pragma once
/*
 * ============================================================
 *  epaper.h - E-Paper display driver & UI renderer
 *  Waveshare 2.13" V4 (250x122, BW), landscape ROTATE_270
 * ============================================================
 */

#include <SPI.h>
#include "config.h"
#include "epd2in13_V4.h"
#include "epdpaint.h"
#include "pet.h"
#include "pomodoro.h"
#include "behaviour.h"

#define COL_BLACK  0
#define COL_WHITE  1

static Epd epd;
static unsigned char _framebuf[128 / 8 * 250];   // 4000 bytes
static Paint paint(_framebuf, 128, 250);

static int  _partialCount = 0;
static const int PARTIAL_LIMIT = 30;

// Forward declarations
void drawBuddyEyes(int originX, int originY);
void drawSingleEye(int x, int y, int w, int h, PetMood mood, int8_t pupilOffsetX, uint8_t blinkLevel, bool leftEye);
void drawTimer(uint32_t seconds, int x, int y, sFONT* font);
void drawProgressBar(int x, int y, int w, int h, float progress);
void drawCenteredString(int y, const char* text, sFONT* font);
void renderToBuffer(int mode);

// ── Init / Splash ───────────────────────────────────────────

void initDisplay() {
  if (epd.Init(FULL) != 0) {
    Serial.println(F("[EPD] Init failed!"));
    return;
  }
  epd.Clear();
  paint.SetRotate(ROTATE_270);
  Serial.println(F("[EPD] Ready (landscape 250x122)"));
}

void showSplashScreen() {
  paint.Clear(COL_WHITE);
  paint.DrawStringAt(40, 20, "UniBuddy", &Font24, COL_BLACK);
  paint.DrawStringAt(52, 54, "waking up...", &Font16, COL_BLACK);
  paint.DrawHorizontalLine(30, 50, 190, COL_BLACK);
  setPetMood(MOOD_HAPPY);
  drawBuddyEyes(74, 70);
  epd.Display(_framebuf);
}

// ── Refresh helpers ─────────────────────────────────────────

void deepRefresh(int mode) {
  epd.Init(FULL);
  renderToBuffer(mode);
  epd.DisplayPartBaseImage(_framebuf);
  _partialCount = 0;
}

void fullRefresh(int mode) {
  epd.Init(FULL);
  renderToBuffer(mode);
  epd.DisplayPartBaseImage(_framebuf);
  _partialCount = 0;
}

void partialRefresh(int mode) {
  if (_partialCount >= PARTIAL_LIMIT) {
    fullRefresh(mode);
    return;
  }
  epd.Init(PART);
  renderToBuffer(mode);
  epd.DisplayPart(_framebuf);
  _partialCount++;
}

void sleepDisplay() {
  epd.Sleep();
}

// ── Render frame into buffer ────────────────────────────────

void renderToBuffer(int mode) {
  paint.Clear(COL_WHITE);

  drawBuddyEyes(6, 6);
  paint.DrawStringAt(165, 10, getPetMoodName(), &Font12, COL_BLACK);

  switch (mode) {
    case MODE_IDLE: {
      paint.DrawStringAt(70, 4, "UniBuddy", &Font20, COL_BLACK);
      paint.DrawHorizontalLine(30, 26, 120, COL_BLACK);
      char buf[24];
      snprintf(buf, sizeof(buf), "Sessions: %d", getSessionCount());
      paint.DrawStringAt(30, 34, buf, &Font12, COL_BLACK);
      snprintf(buf, sizeof(buf), "Streak: %d days", getStreakDays());
      paint.DrawStringAt(30, 52, buf, &Font12, COL_BLACK);
      paint.DrawStringAt(20, 90, "Double tap to focus", &Font12, COL_BLACK);
      break;
    }

    case MODE_POMODORO: {
      uint32_t sLeft = pomodoroSecondsLeft();
      char label[16];
      snprintf(label, sizeof(label), "FOCUS #%d", getSessionCount() + 1);
      paint.DrawStringAt(70, 4, label, &Font16, COL_BLACK);
      drawTimer(sLeft, 60, 30, &Font24);
      float progress = 1.0f - (float)sLeft / (POMODORO_DURATION / 1000.0f);
      drawProgressBar(20, 65, 210, 14, progress);
      paint.DrawStringAt(20, 95, "Double tap to cancel", &Font12, COL_BLACK);
      break;
    }

    case MODE_BREAK: {
      uint32_t sLeft = breakSecondsLeft();
      paint.DrawStringAt(70, 4, "BREAK TIME", &Font16, COL_BLACK);
      drawTimer(sLeft, 60, 30, &Font24);
      char buf[24];
      snprintf(buf, sizeof(buf), "Cycle %d done!", getCompletedCycleCount());
      paint.DrawStringAt(30, 65, buf, &Font12, COL_BLACK);
      paint.DrawStringAt(30, 95, "Tap to skip break", &Font12, COL_BLACK);
      break;
    }

    case MODE_NUDGE: {
      paint.DrawStringAt(70, 10, "Hey!", &Font24, COL_BLACK);
      paint.DrawStringAt(70, 44, "Get back to", &Font16, COL_BLACK);
      paint.DrawStringAt(70, 66, "work! :)", &Font16, COL_BLACK);
      break;
    }

    case MODE_STATS: {
      paint.DrawStringAt(70, 4, "TODAY'S STATS", &Font16, COL_BLACK);
      paint.DrawHorizontalLine(6, 24, 238, COL_BLACK);
      char buf[28];
      snprintf(buf, sizeof(buf), "Sessions:   %d", getSessionCount());
      paint.DrawStringAt(10, 32, buf, &Font12, COL_BLACK);
      { unsigned long totalSec = getSessionCount() * (POMODORO_DURATION / 1000UL);
        if (totalSec >= 60)
          snprintf(buf, sizeof(buf), "Focus time: %d min", (int)(totalSec / 60));
        else
          snprintf(buf, sizeof(buf), "Focus time: %ds", (int)totalSec);
      }
      paint.DrawStringAt(10, 50, buf, &Font12, COL_BLACK);
      snprintf(buf, sizeof(buf), "Streak:     %d days", getStreakDays());
      paint.DrawStringAt(10, 68, buf, &Font12, COL_BLACK);
      paint.DrawStringAt(10, 100, "Tap to go back", &Font12, COL_BLACK);
      break;
    }
  }
}

// ── Drawing helpers ─────────────────────────────────────────

void drawBuddyEyes(int originX, int originY) {
  const int eyeW = 58;
  const int eyeH = 34;
  const int eyeGap = 18;
  int8_t pupilOffsetX = getPetEyeOffsetX();
  uint8_t blinkLevel = getPetBlinkLevel();
  PetMood mood = getPetMood();
  drawSingleEye(originX, originY, eyeW, eyeH, mood, pupilOffsetX, blinkLevel, true);
  drawSingleEye(originX + eyeW + eyeGap, originY, eyeW, eyeH, mood, pupilOffsetX, blinkLevel, false);
}

void drawSingleEye(int x, int y, int w, int h, PetMood mood, int8_t pupilOffsetX, uint8_t blinkLevel, bool leftEye) {
  int cx = x + w / 2;
  int cy = y + h / 2;

  paint.DrawRectangle(x, y, x + w, y + h, COL_BLACK);

  if (blinkLevel == 2) {
    paint.DrawHorizontalLine(x + 4, cy, w - 8, COL_BLACK);
    return;
  }
  if (blinkLevel == 1) {
    paint.DrawHorizontalLine(x + 6, cy - 2, w - 12, COL_BLACK);
    paint.DrawHorizontalLine(x + 6, cy + 2, w - 12, COL_BLACK);
    return;
  }

  int pupilW = 10;
  int pupilH = 10;
  int pupilX = cx - pupilW / 2 + pupilOffsetX;
  int pupilY = cy - pupilH / 2;

  switch (mood) {
    case MOOD_HAPPY:
      paint.DrawLine(x + 5, y + h - 8, x + w - 6, y + h - 8, COL_BLACK);
      paint.DrawLine(x + 8, y + h - 11, x + w - 9, y + h - 11, COL_BLACK);
      break;
    case MOOD_INTERESTED:
      paint.DrawCircle(cx, cy, 12, COL_BLACK);
      break;
    case MOOD_SAD:
      paint.DrawLine(x + 6, y + 8, x + w - 8, y + 12, COL_BLACK);
      paint.DrawLine(x + 6, y + 10, x + w - 8, y + 14, COL_BLACK);
      pupilY += 3;
      break;
    case MOOD_ANGRY:
      if (leftEye)
        paint.DrawLine(x + 6, y + 8, x + w - 8, y + 2, COL_BLACK);
      else
        paint.DrawLine(x + 6, y + 2, x + w - 8, y + 8, COL_BLACK);
      paint.DrawLine(x + 6, y + 10, x + w - 8, y + 4, COL_BLACK);
      break;
    case MOOD_CONFUSED:
      if (leftEye)
        paint.DrawLine(x + 8, y + 5, x + w - 10, y + 5, COL_BLACK);
      else
        paint.DrawLine(x + 8, y + 10, x + w - 10, y + 2, COL_BLACK);
      break;
    case MOOD_DESPISED:
      paint.DrawHorizontalLine(x + 6, y + 12, w - 12, COL_BLACK);
      paint.DrawHorizontalLine(x + 6, y + 13, w - 12, COL_BLACK);
      pupilH = 6;
      pupilY += 2;
      break;
    case MOOD_TIRED:
      paint.DrawLine(x + 6, y + 11, x + w - 8, y + 13, COL_BLACK);
      paint.DrawLine(x + 6, y + 13, x + w - 8, y + 15, COL_BLACK);
      pupilH = 7;
      pupilY += 4;
      break;
    case MOOD_ASLEEP:
      paint.DrawLine(x + 8, y + 12, x + 18, y + 12, COL_BLACK);
      paint.DrawLine(x + 16, y + 16, x + 28, y + 16, COL_BLACK);
      paint.DrawLine(x + 24, y + 12, x + 36, y + 12, COL_BLACK);
      paint.DrawLine(x + 32, y + 16, x + 44, y + 16, COL_BLACK);
      return;
    case MOOD_FOCUSED:
    default:
      paint.DrawHorizontalLine(x + 6, y + 7, w - 12, COL_BLACK);
      paint.DrawHorizontalLine(x + 6, y + 8, w - 12, COL_BLACK);
      break;
  }

  paint.DrawFilledRectangle(pupilX, pupilY, pupilX + pupilW, pupilY + pupilH, COL_BLACK);
}

void drawTimer(uint32_t seconds, int x, int y, sFONT* font) {
  char buf[6];
  uint32_t m = seconds / 60;
  uint32_t s = seconds % 60;
  snprintf(buf, sizeof(buf), "%02d:%02d", (int)m, (int)s);
  paint.DrawStringAt(x, y, buf, font, COL_BLACK);
}

void drawProgressBar(int x, int y, int w, int h, float progress) {
  if (progress < 0.0f) progress = 0.0f;
  if (progress > 1.0f) progress = 1.0f;
  paint.DrawRectangle(x, y, x + w - 1, y + h - 1, COL_BLACK);
  int fill = (int)(progress * (w - 4));
  if (fill > 0) {
    paint.DrawFilledRectangle(x + 2, y + 2, x + 2 + fill - 1, y + h - 3, COL_BLACK);
  }
}

void drawCenteredString(int y, const char* text, sFONT* font) {
  int textWidth = strlen(text) * font->Width;
  int x = (DISPLAY_WIDTH - textWidth) / 2;
  if (x < 0) x = 0;
  paint.DrawStringAt(x, y, text, font, COL_BLACK);
}
