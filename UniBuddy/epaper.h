#pragma once
/*
 * ════════════════════════════════════════════════════════════
 *  epaper.h — E-Paper display driver & UI renderer
 *
 *  Hardware : Waveshare 2.13" e-Paper V4 (250×122, BW)
 *  Layout   : Landscape via ROTATE_270  →  250 w × 122 h
 *  Buffer   : 4000 bytes full-frame  (128/8 × 250)
 *
 *  Refresh strategy
 *  ────────────────
 *  fullRefresh()    FULL init + DisplayPartBaseImage  (mode change)
 *  partialRefresh() PART init + DisplayPart           (timer tick)
 *  Every PARTIAL_LIMIT partials → auto full refresh to de-ghost.
 * ════════════════════════════════════════════════════════════
 */

#include <SPI.h>
#include "config.h"
#include "epd2in13_V4.h"
#include "epdpaint.h"
#include "pet.h"
#include "pomodoro.h"
#include "behaviour.h"

// ── Colour aliases (e-paper: 0 = black, 1 = white) ─────────
#define COL_BLACK  0
#define COL_WHITE  1

// ── Framebuffer & paint object ──────────────────────────────
static Epd           epd;
static unsigned char _framebuf[128 / 8 * 250];   // 4000 B
static Paint         paint(_framebuf, 128, 250);  // physical w, h

static int _partialCount            = 0;
static const int PARTIAL_LIMIT      = 30;

// ── Forward declarations ────────────────────────────────────
static void renderToBuffer(AppMode mode);
static void drawBitmap16(int x, int y, const uint8_t* bmp);
static void drawTimer(uint32_t seconds, int x, int y, sFONT* font);
static void drawProgressBar(int x, int y, int w, int h, float pct);
static void drawCenteredString(int y, const char* text, sFONT* font);

// ── Init / Splash ───────────────────────────────────────────

static void initDisplay() {
  if (epd.Init(FULL) != 0) {
    Serial.println(F("[EPD] Init failed!"));
    return;
  }
  epd.Clear();
  paint.SetRotate(ROTATE_270);       // landscape
  Serial.println(F("[EPD] Ready (landscape 250x122)"));
}

static void showSplashScreen() {
  paint.Clear(COL_WHITE);

  // Title
  paint.DrawStringAt(40, 20, "UniBuddy", &Font24, COL_BLACK);

  // Subtitle
  paint.DrawStringAt(52, 54, "waking up...", &Font16, COL_BLACK);

  // Decorative line
  paint.DrawHorizontalLine(30, 50, 190, COL_BLACK);

  // Pet in bottom-left
  drawBitmap16(10, 80, PET_HAPPY_0);

  epd.Display(_framebuf);
}

// ── Refresh helpers ─────────────────────────────────────────

static void fullRefresh(AppMode mode) {
  epd.Init(FULL);
  renderToBuffer(mode);
  epd.DisplayPartBaseImage(_framebuf);   // full refresh + set base
  _partialCount = 0;
}

static void partialRefresh(AppMode mode) {
  if (_partialCount >= PARTIAL_LIMIT) {
    fullRefresh(mode);                   // fight ghosting
    return;
  }
  epd.Init(PART);
  renderToBuffer(mode);
  epd.DisplayPart(_framebuf);
  _partialCount++;
}

static void sleepDisplay() {
  epd.Sleep();
}

// ── Render frame into buffer ────────────────────────────────

static void renderToBuffer(AppMode mode) {
  paint.Clear(COL_WHITE);

  // Pet face — top-left corner in every mode
  drawBitmap16(6, 6, getPetBitmap());

  switch (mode) {
    // ── IDLE ────────────────────────────────────────────────
    case MODE_IDLE: {
      paint.DrawStringAt(30, 4, "UniBuddy", &Font20, COL_BLACK);

      paint.DrawHorizontalLine(30, 26, 120, COL_BLACK);

      char buf[24];
      snprintf(buf, sizeof(buf), "Sessions: %d", getSessionCount());
      paint.DrawStringAt(30, 34, buf, &Font12, COL_BLACK);

      snprintf(buf, sizeof(buf), "Streak: %d days", getStreakDays());
      paint.DrawStringAt(30, 52, buf, &Font12, COL_BLACK);

      paint.DrawStringAt(20, 90, "Hold button to focus", &Font12, COL_BLACK);
      break;
    }

    // ── POMODORO ────────────────────────────────────────────
    case MODE_POMODORO: {
      uint32_t sLeft = pomodoroSecondsLeft();

      // Mode label
      char label[16];
      snprintf(label, sizeof(label), "FOCUS #%d", getSessionCount() + 1);
      paint.DrawStringAt(30, 4, label, &Font16, COL_BLACK);

      // Big timer
      drawTimer(sLeft, 60, 30, &Font24);

      // Progress bar
      float progress = 1.0f - (float)sLeft / (POMODORO_DURATION / 1000.0f);
      drawProgressBar(20, 65, 210, 14, progress);

      // Hint
      paint.DrawStringAt(20, 95, "Long press to cancel", &Font12, COL_BLACK);
      break;
    }

    // ── BREAK ───────────────────────────────────────────────
    case MODE_BREAK: {
      uint32_t sLeft = breakSecondsLeft();

      paint.DrawStringAt(30, 4, "BREAK TIME", &Font16, COL_BLACK);

      drawTimer(sLeft, 60, 30, &Font24);

      char buf[24];
      snprintf(buf, sizeof(buf), "Cycle %d done!", getCompletedCycleCount());
      paint.DrawStringAt(30, 65, buf, &Font12, COL_BLACK);

      paint.DrawStringAt(30, 95, "Press to skip", &Font12, COL_BLACK);
      break;
    }

    // ── NUDGE ───────────────────────────────────────────────
    case MODE_NUDGE: {
      paint.DrawStringAt(30, 10, "Hey!", &Font24, COL_BLACK);
      paint.DrawStringAt(30, 44, "Get back to", &Font16, COL_BLACK);
      paint.DrawStringAt(30, 66, "work! :)", &Font16, COL_BLACK);
      break;
    }

    // ── STATS ───────────────────────────────────────────────
    case MODE_STATS: {
      paint.DrawStringAt(30, 4, "TODAY'S STATS", &Font16, COL_BLACK);
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

      paint.DrawStringAt(10, 100, "Press to go back", &Font12, COL_BLACK);
      break;
    }
  }
}

// ── Drawing helpers ─────────────────────────────────────────

static void drawBitmap16(int x, int y, const uint8_t* bmp) {
  for (int row = 0; row < 16; row++) {
    uint8_t b0 = pgm_read_byte(&bmp[row * 2]);
    uint8_t b1 = pgm_read_byte(&bmp[row * 2 + 1]);
    for (int col = 0; col < 8; col++) {
      if (b0 & (0x80 >> col)) paint.DrawPixel(x + col, y + row, COL_BLACK);
    }
    for (int col = 0; col < 8; col++) {
      if (b1 & (0x80 >> col)) paint.DrawPixel(x + 8 + col, y + row, COL_BLACK);
    }
  }
}

static void drawTimer(uint32_t seconds, int x, int y, sFONT* font) {
  char buf[6];
  uint32_t m = seconds / 60;
  uint32_t s = seconds % 60;
  snprintf(buf, sizeof(buf), "%02d:%02d", (int)m, (int)s);
  paint.DrawStringAt(x, y, buf, font, COL_BLACK);
}

static void drawProgressBar(int x, int y, int w, int h, float pct) {
  if (pct < 0.0f) pct = 0.0f;
  if (pct > 1.0f) pct = 1.0f;

  // Outline
  paint.DrawRectangle(x, y, x + w - 1, y + h - 1, COL_BLACK);

  // Fill
  int fill = (int)(pct * (w - 4));
  if (fill > 0) {
    paint.DrawFilledRectangle(x + 2, y + 2, x + 2 + fill - 1, y + h - 3, COL_BLACK);
  }
}

static void drawCenteredString(int y, const char* text, sFONT* font) {
  int textWidth = strlen(text) * font->Width;
  int x = (DISPLAY_WIDTH - textWidth) / 2;
  if (x < 0) x = 0;
  paint.DrawStringAt(x, y, text, font, COL_BLACK);
}
