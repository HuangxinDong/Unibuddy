/*
 * ============================================================
 *  E-Paper Minimal Test — Waveshare 2.13" V4 (Black/White)
 *  Library: epd2in13_V4 (NOT the B three-color version)
 *
 *  Wiring (SPI) — per Waveshare wiki for Arduino UNO:
 *    VCC  → 5V
 *    GND  → GND
 *    DIN  → D11 (MOSI)
 *    CLK  → D13 (SCK)
 *    CS   → D10
 *    DC   → D9
 *    RST  → D8
 *    BUSY → D7
 *
 *  What this does:
 *    1. Init in FULL mode, clear screen
 *    2. Draw text using Display1() (4 strip calls)
 *    3. Put display to sleep
 *
 *  Display1() works in 4-strip cycles:
 *    Call 1: starts 0x24 command, sends strip data
 *    Call 2-3: sends strip data
 *    Call 4: sends strip data + triggers refresh
 *  Each strip = bufwidth(16) × bufheight(63) = 1008 bytes
 *  4 strips × 63 rows = 252 rows ≥ 250 (EPD_HEIGHT)
 * ============================================================
 */

#include <SPI.h>
#include "epd2in13_V4.h"
#include "epdpaint.h"

#define COLORED     0
#define UNCOLORED   1

// Buffer: matches library's bufwidth(16) × bufheight(63) = 1008 bytes
unsigned char image[1050];

Epd epd;

void setup() {
  Serial.begin(115200);
  Serial.println(F("[EPD Test] Starting..."));

  // ── Init in FULL refresh mode ───────────────────────────────
  if (epd.Init(FULL) != 0) {
    Serial.println(F("[EPD Test] Init FAILED! Check wiring:"));
    Serial.println(F("  VCC->5V  GND->GND  DIN->D11  CLK->D13"));
    Serial.println(F("  CS->D10  DC->D9  RST->D8  BUSY->D7"));
    while (true);
  }
  Serial.println(F("[EPD Test] Init OK."));

  // ── Clear entire screen ─────────────────────────────────────
  epd.Clear();
  Serial.println(F("[EPD Test] Screen cleared."));

  // ── Paint object: 128 wide × 63 tall (matches bufwidth*8 × bufheight)
  Paint paint(image, epd.bufwidth * 8, epd.bufheight);

  // ── Strip 1: text ───────────────────────────────────────────
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(2, 4,  "Hello UniBuddy!", &Font16, COLORED);
  paint.DrawStringAt(2, 28, "E-Paper works!", &Font12, COLORED);
  paint.DrawStringAt(2, 46, "epd2in13 V4", &Font12, COLORED);
  epd.Display1(image);
  Serial.println(F("[EPD Test] Strip 1 sent."));

  // ── Strip 2: shapes ────────────────────────────────────────
  paint.Clear(UNCOLORED);
  paint.DrawRectangle(2, 2, 50, 50, COLORED);
  paint.DrawLine(2, 2, 50, 50, COLORED);
  paint.DrawLine(2, 50, 50, 2, COLORED);
  paint.DrawCircle(90, 25, 20, COLORED);
  epd.Display1(image);
  Serial.println(F("[EPD Test] Strip 2 sent."));

  // ── Strip 3: filled shapes ─────────────────────────────────
  paint.Clear(UNCOLORED);
  paint.DrawFilledRectangle(2, 2, 50, 50, COLORED);
  paint.DrawFilledCircle(90, 25, 20, COLORED);
  epd.Display1(image);
  Serial.println(F("[EPD Test] Strip 3 sent."));

  // ── Strip 4: empty (triggers refresh) ──────────────────────
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(2, 20, "Test complete!", &Font16, COLORED);
  epd.Display1(image);   // 4th call → refresh!
  Serial.println(F("[EPD Test] Strip 4 sent -> Refreshing!"));

  Serial.println(F("[EPD Test] Display should now show 4 strips:"));
  Serial.println(F("  1: Hello UniBuddy! / E-Paper works! / epd2in13 V4"));
  Serial.println(F("  2: Rectangle + X / Circle outline"));
  Serial.println(F("  3: Filled rectangle / Filled circle"));
  Serial.println(F("  4: Test complete!"));

  // ── Sleep to protect display ────────────────────────────────
  delay(2000);
  epd.Sleep();
  Serial.println(F("[EPD Test] Display sleeping. Done."));
}

void loop() {
  // E-paper retains image without power — nothing to do.
}
