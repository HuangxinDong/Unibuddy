#pragma once
/*
 * ============================================================
 *  epaper.h — E-Paper driver & multi-screen renderer
 *  Waveshare 2.13" V4 (250×122, BW)
 *
 *  Every mood has a unique eye style.
 *  Focus screen: big eyes top → progress bar → time → dots+label
 * ============================================================
 */

#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include "config.h"
#include "epd2in13_V4.h"
#include "epdpaint.h"
#include "pet.h"
#include "pomodoro.h"
#include "behaviour.h"
#include "Modulino.h"
#include "calendar.h"

#define COL_BLACK  0
#define COL_WHITE  1
#define B  COL_BLACK
#define W  COL_WHITE

// ── Night mode state ────────────────────────────────────────
static bool _nightMode = false;

void  setNightMode(bool on) { _nightMode = on; }
bool  isNightMode()          { return _nightMode; }
void  toggleNightMode()      { _nightMode = !_nightMode; }

/* dynamic foreground / background for night-aware drawing */
static inline int FG() { return _nightMode ? W : B; }
static inline int BG() { return _nightMode ? B : W; }

static Epd epd;
static unsigned char _fb[128 / 8 * 250];
static Paint paint(_fb, 128, 250);

static int  _partialCount = 0;
static const int PARTIAL_LIMIT = 30;

static uint8_t  _sleepFrame  = 0;
static uint32_t _sleepTimer  = 0;

// forward decls
void drawPetFace();
void drawSleepFace();
void drawTempCalPortrait();
void drawFocusScreen();
void drawBreakScreen();
void renderToBuffer(int mode);

// ═══════════════════════════════════════════════════════════
//  Init / splash / refresh
// ═══════════════════════════════════════════════════════════

void initDisplay() {
  if (epd.Init(FULL) != 0) { Serial.println(F("[EPD] FAIL")); return; }
  epd.Clear();
  paint.SetRotate(ROTATE_270);
  initCalendarSensors();
  Serial.println(F("[EPD] Ready"));
}

void setDisplayRotation(int r) { paint.SetRotate(r); }

void showSplashScreen() {
  paint.Clear(W);
  paint.DrawStringAt(40, 12, "UniBuddy", &Font24, B);
  paint.DrawHorizontalLine(30, 42, 190, B);
  paint.DrawStringAt(42, 50, "Tilt to switch!", &Font16, B);
  paint.DrawStringAt(15, 76,  "Stand -> Pet   Flat -> Sleep", &Font12, B);
  paint.DrawStringAt(15, 92,  "Tilt -> Info   Flip -> Focus", &Font12, B);
  paint.DrawStringAt(15, 108, "Shake/Tap me! 2xTap->Night", &Font12, B);
  epd.Display(_fb);
}

void deepRefresh(int mode) {
  epd.Init(FULL); renderToBuffer(mode);
  epd.DisplayPartBaseImage(_fb); _partialCount = 0;
}
void fullRefresh(int mode) {
  epd.Init(FULL); renderToBuffer(mode);
  epd.DisplayPartBaseImage(_fb); _partialCount = 0;
}
void partialRefresh(int mode) {
  if (_partialCount >= PARTIAL_LIMIT) { fullRefresh(mode); return; }
  epd.Init(PART); renderToBuffer(mode);
  epd.DisplayPart(_fb); _partialCount++;
}
void sleepDisplay() { epd.Sleep(); }

// ═══════════════════════════════════════════════════════════
//  Drawing primitives
// ═══════════════════════════════════════════════════════════

void thickHLine(int x, int y, int w, int t, int col) {
  for (int d = -t; d <= t; d++) paint.DrawHorizontalLine(x, y + d, w, col);
}

void thickLine(int x0, int y0, int x1, int y1, int t, int col) {
  for (int d = -t; d <= t; d++) paint.DrawLine(x0, y0 + d, x1, y1 + d, col);
}

// ── Night-mode aware drawing helpers ────────────────────────
// These use FG()/BG() to auto-invert colors in night mode

void nThickHLine(int x, int y, int w, int t) {
  thickHLine(x, y, w, t, FG());
}
void nThickLine(int x0, int y0, int x1, int y1, int t) {
  thickLine(x0, y0, x1, y1, t, FG());
}

// ═══════════════════════════════════════════════════════════
//  Mood-specific eye styles  (all take centre x,y + radius)
// ═══════════════════════════════════════════════════════════

/* standard open: outline→white→pupil→sparkle */
void eyeOpen(int cx, int cy, int R, int pR, int8_t pox) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 3, BG());
  paint.DrawFilledCircle(cx + pox, cy + 2, pR, FG());
  paint.DrawFilledCircle(cx + pox - pR/3, cy + 2 - pR/3, pR/3 + 1, BG());
}

/* blink: thin bar */
void eyeBlink(int cx, int cy, int R) {
  paint.DrawFilledRectangle(cx - R, cy - 2, cx + R, cy + 2, FG());
}

/* happy ^_^ */
void eyeHappy(int cx, int cy, int R) {
  nThickLine(cx - R, cy, cx, cy - R*2/3, 2);
  nThickLine(cx, cy - R*2/3, cx + R, cy, 2);
}

/* cute  ⌒‿⌒  (happy arc + sparkle dot above) */
void eyeCute(int cx, int cy, int R) {
  eyeHappy(cx, cy, R);
  paint.DrawFilledCircle(cx - R/2, cy - R + 2, 2, FG());
  paint.DrawFilledCircle(cx + R/2, cy - R + 2, 2, FG());
}

/* interested: bigger pupil, slightly wider */
void eyeInterested(int cx, int cy, int R, int8_t pox) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 3, BG());
  int bigP = R * 2 / 3;
  paint.DrawFilledCircle(cx + pox, cy + 1, bigP, FG());
  paint.DrawFilledCircle(cx + pox - bigP/3, cy - bigP/4, bigP/3 + 1, BG());
}

/* bored: half-lid, small pupil off-centre */
void eyeBored(int cx, int cy, int R) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 3, BG());
  paint.DrawFilledRectangle(cx - R - 1, cy - R - 1, cx + R + 1, cy, BG());
  nThickHLine(cx - R, cy, R * 2, 1);
  paint.DrawFilledCircle(cx + R/3, cy + R/4, R/4, FG());
}

/* surprised  O_O: extra wide, tiny far-apart pupils */
void eyeSurprised(int cx, int cy, int R) {
  int bigR = R + 4;
  paint.DrawFilledCircle(cx, cy, bigR, FG());
  paint.DrawFilledCircle(cx, cy, bigR - 3, BG());
  paint.DrawFilledCircle(cx, cy, R/3, FG());
  paint.DrawFilledCircle(cx - 2, cy - 2, 2, BG());
}

/* worried: slightly droopy, brow line angled down-inward */
void eyeWorried(int cx, int cy, int R, bool isLeft) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 3, BG());
  paint.DrawFilledCircle(cx, cy + 2, R/3, FG());
  paint.DrawFilledCircle(cx - 1, cy, 2, BG());
  if (isLeft)
    nThickLine(cx - R, cy - R - 4, cx + R/2, cy - R + 2, 1);
  else
    nThickLine(cx - R/2, cy - R + 2, cx + R, cy - R - 4, 1);
}

/* annoyed  >_< */
void eyeAnnoyed(int cx, int cy, int R) {
  nThickLine(cx - R, cy - R/2, cx, cy, 2);
  nThickLine(cx, cy, cx + R, cy - R/2, 2);
  nThickLine(cx - R, cy + R/2, cx, cy, 2);
  nThickLine(cx, cy, cx + R, cy + R/2, 2);
}

/* dizzy  @_@: spiral-like concentric rings */
void eyeDizzy(int cx, int cy, int R) {
  paint.DrawCircle(cx, cy, R, FG());
  paint.DrawCircle(cx, cy, R * 2 / 3, FG());
  paint.DrawCircle(cx, cy, R / 3, FG());
  paint.DrawFilledCircle(cx, cy, 3, FG());
}

/* sad: droopy outline + small pupil low */
void eyeSad(int cx, int cy, int R) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 3, BG());
  paint.DrawFilledRectangle(cx - R - 1, cy - R - 1, cx + R + 1, cy - R/2, BG());
  nThickLine(cx - R, cy - R/3, cx + R, cy - R/2, 1);
  paint.DrawFilledCircle(cx, cy + R/4, R/4, FG());
  paint.DrawFilledCircle(cx - 1, cy + R/4 - 2, 2, BG());
}

/* angry: V brows + sharp pupil */
void eyeAngry(int cx, int cy, int R, bool isLeft) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 3, BG());
  paint.DrawFilledCircle(cx, cy + 2, R/3 + 1, FG());
  if (isLeft)
    nThickLine(cx - R, cy - R + 6, cx + R/2, cy - R - 2, 2);
  else
    nThickLine(cx - R/2, cy - R - 2, cx + R, cy - R + 6, 2);
}

/* confused: one brow up, one down */
void eyeConfused(int cx, int cy, int R, bool isLeft) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 3, BG());
  paint.DrawFilledCircle(cx, cy + 1, R/3, FG());
  paint.DrawFilledCircle(cx - 1, cy - 1, 2, BG());
  if (isLeft)
    nThickLine(cx - R, cy - R - 2, cx + R/2, cy - R + 4, 1);
  else
    nThickLine(cx - R/2, cy - R + 4, cx + R, cy - R - 6, 1);
}

/* focused: squinted half-circle */
void eyeFocused(int cx, int cy, int R, int pR) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 2, BG());
  paint.DrawFilledRectangle(cx - R - 1, cy - R - 1, cx + R + 1, cy - R/3, BG());
  nThickHLine(cx - R, cy - R/3, R * 2, 1);
  paint.DrawFilledCircle(cx, cy + 2, pR, FG());
  paint.DrawFilledCircle(cx - pR/4, cy + 1 - pR/4, pR/4 + 1, BG());
}

/* tired: heavy lids */
void eyeTired(int cx, int cy, int R) {
  paint.DrawFilledCircle(cx, cy, R, FG());
  paint.DrawFilledCircle(cx, cy, R - 3, BG());
  paint.DrawFilledRectangle(cx - R - 1, cy - R - 1, cx + R + 1, cy + R/6, BG());
  nThickHLine(cx - R, cy + R/6, R * 2, 1);
  paint.DrawFilledCircle(cx, cy + R/3, R/4, FG());
}

/* asleep: gentle closed curves */
void eyeAsleep(int cx, int cy, int R) {
  nThickLine(cx - R, cy, cx, cy + 4, 1);
  nThickLine(cx, cy + 4, cx + R, cy, 1);
}

// ═══════════════════════════════════════════════════════════
//  Composite eye drawer — dispatches per mood
// ═══════════════════════════════════════════════════════════

void drawEyePair(int lx, int rx, int ey, int R, int pR) {
  PetMood mood  = getPetMood();
  int8_t  pox   = getPetEyeOffsetX();
  uint8_t blink = getPetBlinkLevel();
  bool    spec  = isPetSpecialPhase();
  int fg = FG();

  /* always blink on phase 3, regardless of mood */
  if (blink == 2) {
    eyeBlink(lx, ey, R);
    eyeBlink(rx, ey, R);
    return;
  }

  /* mood-specific special on phase 7 */
  if (spec) {
    switch (mood) {
      case MOOD_HAPPY:
      case MOOD_CUTE:
        eyeCute(lx, ey, R); eyeCute(rx, ey, R); return;
      case MOOD_SURPRISED:
        eyeSurprised(lx, ey, R); eyeSurprised(rx, ey, R); return;
      case MOOD_ANNOYED:
        eyeAnnoyed(lx, ey, R); eyeAnnoyed(rx, ey, R); return;
      case MOOD_DIZZY:
        eyeDizzy(lx, ey, R); eyeDizzy(rx, ey, R); return;
      default: break;  /* fall through to normal for other moods */
    }
  }

  /* normal frames: mood-aware eye style */
  switch (mood) {
    case MOOD_HAPPY:
      eyeOpen(lx, ey, R, pR, pox); eyeOpen(rx, ey, R, pR, pox); break;
    case MOOD_CUTE:
      eyeOpen(lx, ey, R, pR, pox); eyeOpen(rx, ey, R, pR, pox);
      /* blush dots under eyes */
      paint.DrawFilledCircle(lx - R/2, ey + R + 4, 3, fg);
      paint.DrawFilledCircle(lx + R/2, ey + R + 4, 3, fg);
      paint.DrawFilledCircle(rx - R/2, ey + R + 4, 3, fg);
      paint.DrawFilledCircle(rx + R/2, ey + R + 4, 3, fg);
      break;
    case MOOD_INTERESTED:
      eyeInterested(lx, ey, R, pox); eyeInterested(rx, ey, R, pox); break;
    case MOOD_BORED:
      eyeBored(lx, ey, R); eyeBored(rx, ey, R); break;
    case MOOD_SURPRISED:
      eyeSurprised(lx, ey, R); eyeSurprised(rx, ey, R); break;
    case MOOD_WORRIED:
      eyeWorried(lx, ey, R, true); eyeWorried(rx, ey, R, false); break;
    case MOOD_ANNOYED:
      eyeAnnoyed(lx, ey, R); eyeAnnoyed(rx, ey, R); break;
    case MOOD_DIZZY:
      eyeDizzy(lx, ey, R); eyeDizzy(rx, ey, R); break;
    case MOOD_SAD:
      eyeSad(lx, ey, R); eyeSad(rx, ey, R); break;
    case MOOD_ANGRY:
      eyeAngry(lx, ey, R, true); eyeAngry(rx, ey, R, false); break;
    case MOOD_CONFUSED:
      eyeConfused(lx, ey, R, true); eyeConfused(rx, ey, R, false); break;
    case MOOD_FOCUSED:
      eyeFocused(lx, ey, R, pR); eyeFocused(rx, ey, R, pR); break;
    case MOOD_TIRED:
      eyeTired(lx, ey, R); eyeTired(rx, ey, R); break;
    case MOOD_ASLEEP:
      eyeAsleep(lx, ey, R); eyeAsleep(rx, ey, R); break;
    default:
      eyeOpen(lx, ey, R, pR, pox); eyeOpen(rx, ey, R, pR, pox); break;
  }
}

// ═══════════════════════════════════════════════════════════
//  Mood symbol — shapes only, no text labels
//  Drawn near the eyes to express emotion
// ═══════════════════════════════════════════════════════════

/* small heart shape at (cx, cy) */
void drawHeart(int cx, int cy, int s) {
  int fg = FG();
  paint.DrawFilledCircle(cx - s, cy, s, fg);
  paint.DrawFilledCircle(cx + s, cy, s, fg);
  for (int row = 0; row <= s * 2; row++) {
    int hw = s * 2 - row;
    if (hw < 0) hw = 0;
    paint.DrawHorizontalLine(cx - hw, cy + row, hw * 2 + 1, fg);
  }
}

/* manga anger cross ╳ at (cx, cy) */
void drawAngerMark(int cx, int cy, int s) {
  nThickLine(cx - s, cy - s, cx + s, cy + s, 1);
  nThickLine(cx + s, cy - s, cx - s, cy + s, 1);
}

/* sweat drop at (cx, cy) */
void drawSweatDrop(int cx, int cy, int s) {
  int fg = FG();
  paint.DrawFilledCircle(cx, cy + s, s, fg);
  paint.DrawLine(cx, cy - s, cx - s, cy + s, fg);
  paint.DrawLine(cx, cy - s, cx + s, cy + s, fg);
}

/* sparkle ✦ four-pointed star */
void drawSparkle(int cx, int cy, int s) {
  int fg = FG();
  paint.DrawLine(cx, cy - s, cx, cy + s, fg);
  paint.DrawLine(cx - s, cy, cx + s, cy, fg);
  paint.DrawFilledCircle(cx, cy, s/3, fg);
}

/* spiral @ */
void drawSpiral(int cx, int cy, int R) {
  int fg = FG();
  paint.DrawCircle(cx, cy, R, fg);
  paint.DrawCircle(cx, cy, R * 2/3, fg);
  paint.DrawCircle(cx + 2, cy - 1, R/3, fg);
}

void drawMoodSymbol(int rx, int ey, int R) {
  PetMood mood = getPetMood();
  int sx = rx + R + 8;
  int sy = ey - R/2;
  int fg = FG();

  switch (mood) {
    case MOOD_HAPPY:
      break;
    case MOOD_CUTE:
      drawSparkle(sx,     sy - 4, 5);
      drawSparkle(sx + 14, sy + 2, 4);
      break;
    case MOOD_SURPRISED:
      paint.DrawFilledRectangle(sx, sy - 6, sx + 3, sy + 6, fg);
      paint.DrawFilledCircle(sx + 1, sy + 10, 2, fg);
      paint.DrawFilledRectangle(sx + 8, sy - 6, sx + 11, sy + 6, fg);
      paint.DrawFilledCircle(sx + 9, sy + 10, 2, fg);
      break;
    case MOOD_WORRIED:
      drawSweatDrop(sx + 4, sy, 4);
      break;
    case MOOD_ANNOYED:
      drawAngerMark(sx + 4, sy, 6);
      break;
    case MOOD_ANGRY:
      drawAngerMark(sx,      sy - 4, 5);
      drawAngerMark(sx + 14, sy + 2, 4);
      break;
    case MOOD_DIZZY:
      break;
    case MOOD_SAD:
      drawSweatDrop(rx + R/2, ey + R + 4, 3);
      break;
    case MOOD_CONFUSED:
      paint.DrawCircle(sx + 4, sy - 2, 5, fg);
      paint.DrawFilledRectangle(sx + 7, sy - 2, sx + 9, sy + 6, fg);
      paint.DrawFilledCircle(sx + 8, sy + 10, 2, fg);
      break;
    case MOOD_TIRED:
      paint.DrawStringAt(sx, sy - 4, "z", &Font8, fg);
      paint.DrawStringAt(sx + 8, sy - 10, "z", &Font12, fg);
      break;
    default:
      break;
  }
}

// ═══════════════════════════════════════════════════════════
//  PET FACE  (landscape 250×122)
// ═══════════════════════════════════════════════════════════

void drawPetFace() {
  const int LX = 78, RX = 172, EY = 52;
  const int R = 32, PR = 14;

  if (_nightMode) {
    /* ── Night sky decorations ───────────────────────── */
    int fg = FG();

    /* crescent moon (top-left) */
    paint.DrawFilledCircle(30, 22, 14, fg);
    paint.DrawFilledCircle(38, 16, 12, BG());  // shadow bite

    /* scattered stars (small sparkles) */
    drawSparkle(8,   8,   3);
    drawSparkle(55,  5,   2);
    drawSparkle(110, 10,  3);
    drawSparkle(210, 6,   2);
    drawSparkle(235, 18,  3);
    drawSparkle(18,  105, 2);
    drawSparkle(240, 100, 2);

    /* tiny star dots */
    paint.DrawFilledCircle(70,  14, 1, fg);
    paint.DrawFilledCircle(150,  8, 1, fg);
    paint.DrawFilledCircle(180, 15, 1, fg);
    paint.DrawFilledCircle(45, 108, 1, fg);
    paint.DrawFilledCircle(200, 105, 1, fg);
  }

  drawEyePair(LX, RX, EY, R, PR);
  drawMoodSymbol(RX, EY, R);
}

// ═══════════════════════════════════════════════════════════
//  SLEEP FACE  (landscape 250×122, adaptive rotation)
// ═══════════════════════════════════════════════════════════

void drawSleepFace() {
  const int LX = 78, RX = 172, EY = 52;
  const int R = 28;
  int fg = FG();

  eyeAsleep(LX, EY, R);
  eyeAsleep(RX, EY, R);

  /* floating zzz */
  if (millis() - _sleepTimer > 800) {
    _sleepTimer = millis();
    _sleepFrame = (_sleepFrame + 1) % 3;
  }
  int bx = 188 + _sleepFrame * 5;
  int by = 36  - _sleepFrame * 3;
  paint.DrawStringAt(bx,      by,      "z", &Font12, fg);
  paint.DrawStringAt(bx + 12, by - 10, "z", &Font16, fg);
  paint.DrawStringAt(bx + 26, by - 22, "z", &Font20, fg);

  /* night mode: add stars around sleeping face */
  if (_nightMode) {
    drawSparkle(20,  18, 3);
    drawSparkle(50,   8, 2);
    drawSparkle(230, 12, 3);
    drawSparkle(15, 100, 2);
    paint.DrawFilledCircle(100, 10, 1, fg);
    paint.DrawFilledCircle(200,  6, 1, fg);
  }
}

// ═══════════════════════════════════════════════════════════
//  TEMP & CALENDAR  (portrait 122×250)
// ═══════════════════════════════════════════════════════════

void drawTempCalPortrait() {
  updateCalendarReadings();
  int fg = FG();
  int tempI = isnan(_tempC) ? 0 : (int)_tempC;
  int tempF = isnan(_tempC) ? 0 : (int)(fabs(_tempC - tempI) * 10.0f);
  int hum = isnan(_humPct) ? 0 : (int)(_humPct + 0.5f);
  if (hum < 0) hum = 0;
  if (hum > 100) hum = 100;

  paint.DrawStringAt(10, 8,  _dayBuf, &Font12, fg);
  paint.DrawStringAt(10, 26, _dateBuf, &Font12, fg);
  paint.DrawHorizontalLine(4, 44, 114, fg);

  paint.DrawStringAt(10, 54, _timeBuf, &Font24, fg);
  paint.DrawHorizontalLine(4, 88, 114, fg);

  paint.DrawStringAt(6, 96, "Temperature", &Font12, fg);
  {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d.%d", tempI, tempF);
    paint.DrawStringAt(6, 114, buf, &Font24, fg);
    int tx = 6 + 17 * (int)strlen(buf);
    paint.DrawCircle(tx + 3, 116, 2, fg);
    paint.DrawStringAt(tx + 8, 114, "C", &Font24, fg);
  }

  int barW = 100;
  paint.DrawRectangle(6, 146, 6 + barW, 158, fg);
  int fill = (int)(barW * (tempI + tempF / 10.0f) / 45.0f);
  if (fill > barW) fill = barW;
  if (fill > 0) paint.DrawFilledRectangle(6, 146, 6 + fill, 158, fg);

  paint.DrawHorizontalLine(4, 168, 114, fg);
  paint.DrawStringAt(6, 176, "Humidity", &Font12, fg);
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", hum);
    paint.DrawStringAt(6, 194, buf, &Font24, fg);
  }

  paint.DrawRectangle(6, 224, 6 + barW, 236, fg);
  int hfill = barW * hum / 100;
  if (hfill > 0) paint.DrawFilledRectangle(6, 224, 6 + hfill, 236, fg);

  paint.DrawStringAt(20, 244, "UniBuddy", &Font8, fg);
}

// ═══════════════════════════════════════════════════════════
//  FOCUS / POMODORO  (landscape 250×122, ROTATE_90)
//
//  Layout (per mockup):
//   ┌───────────────────────────────┐
//   │  [focused eye L] [focused eye R]  │  ← big squint eyes (~y 8..50)
//   │                                  │
//   │  ████████████░░░░░░░░░░░░░░░░░░  │  ← progress bar (y 56..70)
//   │           20:24                   │  ← time below bar (y 76)
//   │  ◉◉◉○ Session 1        FOCUS     │  ← dots + label at very bottom
//   └───────────────────────────────┘
// ═══════════════════════════════════════════════════════════

void drawFocusScreen() {
  const int EL = 78, ER = 172, EY = 26;
  const int R = 22, PR = 9;
  uint8_t blink = getPetBlinkLevel();
  int fg = FG(), bg = BG();

  static const int8_t _drift[] = {0, 1, 2, 1, 0, -1, -2, -1};
  int8_t pdx = _drift[(millis() / 1000) % 8];

  if (blink == 2) {
    eyeBlink(EL, EY, R);
    eyeBlink(ER, EY, R);
  } else {
    int exs[2] = {EL, ER};
    for (int i = 0; i < 2; i++) {
      int cx = exs[i];
      paint.DrawFilledCircle(cx, EY, R, fg);
      paint.DrawFilledCircle(cx, EY, R - 2, bg);
      paint.DrawFilledRectangle(cx - R - 1, EY - R - 1, cx + R + 1, EY - R/3, bg);
      nThickHLine(cx - R, EY - R/3, R * 2, 1);
      paint.DrawFilledCircle(cx + pdx, EY + 2, PR, fg);
      paint.DrawFilledCircle(cx + pdx - PR/4, EY + 1 - PR/4, PR/4 + 1, bg);
    }
  }

  /* --- progress bar --- */
  uint32_t sLeft = pomodoroSecondsLeft();
  float progress = 1.0f - (float)sLeft / (POMODORO_DURATION / 1000.0f);
  if (progress < 0) progress = 0;
  if (progress > 1) progress = 1;
  int barX = 16, barY = 54, barW = 218, barH = 12;
  paint.DrawRectangle(barX, barY, barX + barW, barY + barH, fg);
  int fw = (int)(barW * progress);
  if (fw > 0)
    paint.DrawFilledRectangle(barX + 1, barY + 1,
                              barX + fw, barY + barH - 1, fg);

  /* --- time --- */
  int mn = sLeft / 60, sc = sLeft % 60;
  char timeBuf[8];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", mn, sc);
  paint.DrawStringAt(84, 72, timeBuf, &Font24, fg);

  /* --- paused indicator --- */
  if (isPomPaused()) {
    paint.DrawStringAt(80, 94, "|| PAUSED", &Font12, fg);
  }

  /* --- bottom row --- */
  uint8_t sess = getSessionCount();
  int by = 110;
  {
    char sb[16];
    snprintf(sb, sizeof(sb), "Session %d", sess + 1);
    paint.DrawStringAt(6, by, sb, &Font12, fg);
  }
  for (int i = 0; i < 4; i++) {
    int dotX = 110 + i * 14;
    if ((int)i < (int)(sess % 4))
      paint.DrawFilledCircle(dotX, by + 6, 4, fg);
    else
      paint.DrawCircle(dotX, by + 6, 4, fg);
  }
  paint.DrawStringAt(194, by, "FOCUS", &Font12, fg);
}

// ═══════════════════════════════════════════════════════════
//  BREAK  (landscape 250×122, ROTATE_90)
// ═══════════════════════════════════════════════════════════

void drawBreakScreen() {
  int fg = FG();
  paint.DrawStringAt(52, 4, "BREAK TIME", &Font20, fg);
  paint.DrawHorizontalLine(4, 28, 242, fg);

  uint32_t sLeft = breakSecondsLeft();
  int mn = sLeft / 60, sc = sLeft % 60;
  char timeBuf[8];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", mn, sc);
  paint.DrawStringAt(70, 36, timeBuf, &Font24, fg);

  {
    char buf[24];
    snprintf(buf, sizeof(buf), "Cycle %d done!", getCompletedCycleCount());
    paint.DrawStringAt(60, 72, buf, &Font12, fg);
  }

  eyeHappy(80, 100, 16);
  eyeHappy(170, 100, 16);
}

// ═══════════════════════════════════════════════════════════
//  Render dispatcher
// ═══════════════════════════════════════════════════════════

void renderToBuffer(int mode) {
  paint.Clear(BG());
  switch (mode) {
    case MODE_PET:        drawPetFace();       break;
    case MODE_SLEEP:      drawSleepFace();     break;
    case MODE_TEMPTIME_L:
    case MODE_TEMPTIME_R: drawTempCalPortrait(); break;
    case MODE_POMODORO:   drawFocusScreen();   break;
    case MODE_BREAK:      drawBreakScreen();   break;
    default: break;
  }
}
