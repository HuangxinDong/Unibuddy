/*
 * ============================================================
 *  E-Paper Landscape Test — Waveshare 2.13" V4 (Black/White)
 *  Library: epd2in13_V4
 *
 *  Wiring (SPI):
 *    VCC→5V  GND→GND  DIN→D11  CLK→D13
 *    CS→D10  DC→D9  RST→D8  BUSY→D7
 *
 *  What this does:
 *    1. Init FULL, clear screen
 *    2. Draw landscape UI using full 4000-byte framebuffer
 *       with Paint(128, 250) + ROTATE_270
 *    3. Demo partial refresh (counter update)
 *    4. Sleep
 *
 *  Landscape coordinate system:
 *    x: 0 → 249 (left to right)
 *    y: 0 → 121 (top to bottom)
 * ============================================================
 */

#include <SPI.h>
#include "epd2in13_V4.h"
#include "epdpaint.h"

#define COL_BLACK  0
#define COL_WHITE  1

// Full framebuffer: (128/8) × 250 = 4000 bytes
unsigned char framebuf[128 / 8 * 250];
Paint paint(framebuf, 128, 250);    // physical width, height
Epd epd;

void setup() {
  Serial.begin(115200);
  Serial.println(F("[Test] Landscape E-Paper Test"));

  // Init
  if (epd.Init(FULL) != 0) {
    Serial.println(F("[Test] Init FAILED!"));
    while (true);
  }
  epd.Clear();
  Serial.println(F("[Test] Init OK, cleared."));

  // Landscape rotation
  paint.SetRotate(ROTATE_270);

  // ── Draw full-screen landscape UI ───────────────────────
  paint.Clear(COL_WHITE);

  // Title
  paint.DrawStringAt(30, 8, "UniBuddy", &Font24, COL_BLACK);

  // Divider
  paint.DrawHorizontalLine(10, 36, 230, COL_BLACK);

  // Info text
  paint.DrawStringAt(10, 44, "Landscape 250x122", &Font16, COL_BLACK);
  paint.DrawStringAt(10, 66, "E-Paper V4 Test OK", &Font12, COL_BLACK);

  // Shapes
  paint.DrawRectangle(10, 86, 60, 116, COL_BLACK);             // outlined rect
  paint.DrawFilledRectangle(70, 86, 120, 116, COL_BLACK);      // filled rect
  paint.DrawCircle(160, 101, 15, COL_BLACK);                    // circle
  paint.DrawFilledCircle(210, 101, 15, COL_BLACK);              // filled circle

  // Border around entire screen
  paint.DrawRectangle(0, 0, 249, 121, COL_BLACK);

  // Send to display (full refresh) + set as partial base
  epd.DisplayPartBaseImage(framebuf);
  Serial.println(F("[Test] Full refresh done."));

  delay(2000);

  // ── Partial refresh demo: update counter ────────────────
  Serial.println(F("[Test] Partial refresh demo..."));
  for (int i = 1; i <= 5; i++) {
    epd.Init(PART);

    // Erase only the counter area (white rectangle)
    paint.DrawFilledRectangle(10, 44, 230, 80, COL_WHITE);

    // Draw updated text
    char buf[32];
    snprintf(buf, sizeof(buf), "Partial #%d of 5", i);
    paint.DrawStringAt(10, 44, buf, &Font16, COL_BLACK);
    paint.DrawStringAt(10, 66, "No full flash!", &Font12, COL_BLACK);

    epd.DisplayPart(framebuf);
    Serial.print(F("[Test] Partial ")); Serial.println(i);
    delay(2000);
  }

  // ── Sleep ───────────────────────────────────────────────
  epd.Sleep();
  Serial.println(F("[Test] Done. Display sleeping."));
}

void loop() {
  // E-paper retains image without power
}
