/*
 * ============================================================
 *  Tap Sensor Test — Raw diagnostics on e-paper + Serial
 *
 *  Tap Sensor: D2 (INPUT_PULLUP)
 *  E-Paper:    Waveshare 2.13" V4 (landscape 250×122)
 *
 *  This sketch tests THREE detection methods simultaneously
 *  so we can figure out which edge/mode actually fires:
 *
 *    1. Interrupt on RISING  (LOW→HIGH)
 *    2. Interrupt on FALLING (HIGH→LOW)
 *    3. Interrupt on CHANGE  (any edge)
 *
 *  Every event is logged to Serial AND displayed on e-paper.
 *  The screen shows:
 *    - Total RISING / FALLING / CHANGE counts
 *    - Current pin state (HIGH/LOW)
 *    - Last event type + timestamp
 *    - Single-tap / double-tap evaluation
 * ============================================================
 */

#include <SPI.h>
#include "epd2in13_V4.h"
#include "epdpaint.h"

// ── Pin ──────────────────────────────────────────────────────
#define TAP_PIN  2

// ── E-Paper ──────────────────────────────────────────────────
Epd epd;
unsigned char framebuf[128 / 8 * 250];  // 4000 bytes
Paint paint(framebuf, 128, 250);

#define COL_BLACK 0
#define COL_WHITE 1

// ── Interrupt counters (volatile!) ───────────────────────────
volatile uint32_t risingCount  = 0;
volatile uint32_t fallingCount = 0;
volatile uint32_t changeCount  = 0;
volatile uint32_t lastRisingMs  = 0;
volatile uint32_t lastFallingMs = 0;
volatile uint32_t lastChangeMs  = 0;

// We'll use CHANGE mode and inspect pin state inside ISR
void tapISR() {
  uint32_t now = millis();
  changeCount++;
  lastChangeMs = now;

  if (digitalRead(TAP_PIN) == HIGH) {
    risingCount++;
    lastRisingMs = now;
  } else {
    fallingCount++;
    lastFallingMs = now;
  }
}

// ── Double-tap detection state ───────────────────────────────
#define DEBOUNCE_MS   80
#define DBL_WINDOW_MS 400

uint32_t _prevChangeCount = 0;
uint32_t _tapTimes[8];       // ring buffer of recent tap timestamps
uint8_t  _tapHead = 0;
uint8_t  _tapTotal = 0;

// Evaluated results
char lastResult[32] = "Waiting...";
uint32_t lastResultMs = 0;
uint32_t singleCount = 0;
uint32_t doubleCount = 0;

// ── Screen update ────────────────────────────────────────────
uint32_t lastScreenMs = 0;
#define SCREEN_INTERVAL 800   // update screen every 800ms max

void drawScreen() {
  paint.SetRotate(ROTATE_270);
  paint.Clear(COL_WHITE);

  // Title
  paint.DrawStringAt(4, 2, "TAP SENSOR TEST", &Font16, COL_BLACK);
  paint.DrawHorizontalLine(4, 20, 242, COL_BLACK);

  char buf[40];

  // Current pin state
  int pinNow = digitalRead(TAP_PIN);
  snprintf(buf, sizeof(buf), "Pin D2 now: %s", pinNow ? "HIGH" : "LOW");
  paint.DrawStringAt(4, 24, buf, &Font12, COL_BLACK);

  // Interrupt counts
  noInterrupts();
  uint32_t r = risingCount;
  uint32_t f = fallingCount;
  uint32_t c = changeCount;
  uint32_t lr = lastRisingMs;
  uint32_t lf = lastFallingMs;
  interrupts();

  snprintf(buf, sizeof(buf), "RISING:  %lu  (%lums ago)",
           (unsigned long)r, (unsigned long)(millis() - lr));
  paint.DrawStringAt(4, 38, buf, &Font12, COL_BLACK);

  snprintf(buf, sizeof(buf), "FALLING: %lu  (%lums ago)",
           (unsigned long)f, (unsigned long)(millis() - lf));
  paint.DrawStringAt(4, 52, buf, &Font12, COL_BLACK);

  snprintf(buf, sizeof(buf), "CHANGE:  %lu total", (unsigned long)c);
  paint.DrawStringAt(4, 66, buf, &Font12, COL_BLACK);

  // Double-tap evaluation results
  paint.DrawHorizontalLine(4, 80, 242, COL_BLACK);

  snprintf(buf, sizeof(buf), "Single: %lu  Double: %lu",
           (unsigned long)singleCount, (unsigned long)doubleCount);
  paint.DrawStringAt(4, 84, buf, &Font12, COL_BLACK);

  snprintf(buf, sizeof(buf), "Last: %s", lastResult);
  paint.DrawStringAt(4, 100, buf, &Font12, COL_BLACK);
}

void refreshScreen() {
  drawScreen();
  epd.Init(PART);
  epd.DisplayPart(framebuf);
  lastScreenMs = millis();
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println(F("[TapTest] Starting..."));

  // Tap sensor pin
  pinMode(TAP_PIN, INPUT_PULLUP);

  // Use CHANGE to capture both edges
  attachInterrupt(digitalPinToInterrupt(TAP_PIN), tapISR, CHANGE);

  // Init e-paper
  if (epd.Init(FULL) != 0) {
    Serial.println(F("[TapTest] EPD init FAILED!"));
    while (true);
  }
  epd.Clear();
  Serial.println(F("[TapTest] EPD ready."));

  // Draw initial screen (full refresh as base)
  drawScreen();
  epd.DisplayPartBaseImage(framebuf);

  Serial.println(F("[TapTest] Ready. Tap the sensor!"));
  Serial.println(F("  RISING  = LOW->HIGH"));
  Serial.println(F("  FALLING = HIGH->LOW"));
  Serial.println(F("  Single tap / Double tap evaluated with 400ms window"));
}

// ── Double-tap evaluator (runs in loop) ──────────────────────
// We use falling edges as the "tap" trigger and debounce them.
// If your sensor works on rising, we test both.

uint8_t  pendingTaps     = 0;
uint32_t firstPendingMs  = 0;
uint32_t lastAcceptedMs  = 0;
uint32_t prevFalling     = 0;
uint32_t prevRising      = 0;

void evaluateTaps() {
  noInterrupts();
  uint32_t fc = fallingCount;
  uint32_t rc = risingCount;
  uint32_t lfm = lastFallingMs;
  uint32_t lrm = lastRisingMs;
  interrupts();

  // Try FALLING edges as tap signal
  if (fc != prevFalling) {
    if (lfm - lastAcceptedMs >= DEBOUNCE_MS) {
      lastAcceptedMs = lfm;
      pendingTaps++;
      if (pendingTaps == 1) firstPendingMs = lfm;
      prevFalling = fc;

      Serial.print(F("[Tap] FALLING edge #"));
      Serial.print(pendingTaps);
      Serial.print(F(" at "));
      Serial.println(lfm);
    } else {
      prevFalling = fc;  // skip bounced edge
    }
  }

  // Also check RISING edges (in case sensor triggers on rising)
  if (rc != prevRising) {
    // Log it but don't double-count if falling already counted
    if (lrm - lastAcceptedMs >= DEBOUNCE_MS && pendingTaps == 0) {
      // Only use rising if no falling was detected recently
      if (lrm - lfm > 200) {  // rising came alone, not paired with falling
        lastAcceptedMs = lrm;
        pendingTaps++;
        if (pendingTaps == 1) firstPendingMs = lrm;
        Serial.print(F("[Tap] RISING edge (standalone) #"));
        Serial.print(pendingTaps);
        Serial.print(F(" at "));
        Serial.println(lrm);
      }
    }
    prevRising = rc;
  }

  // Evaluate after double-tap window expires
  if (pendingTaps > 0 && (millis() - firstPendingMs >= DBL_WINDOW_MS)) {
    if (pendingTaps >= 2) {
      doubleCount++;
      snprintf(lastResult, sizeof(lastResult), "DOUBLE TAP #%lu",
               (unsigned long)doubleCount);
      Serial.print(F(">>> DOUBLE TAP  (total: "));
      Serial.print(doubleCount);
      Serial.println(F(")"));
    } else {
      singleCount++;
      snprintf(lastResult, sizeof(lastResult), "SINGLE TAP #%lu",
               (unsigned long)singleCount);
      Serial.print(F(">>> SINGLE TAP  (total: "));
      Serial.print(singleCount);
      Serial.println(F(")"));
    }
    pendingTaps = 0;
    lastResultMs = millis();
  }
}

// ── Main Loop ────────────────────────────────────────────────
void loop() {
  evaluateTaps();

  // Refresh screen periodically
  if (millis() - lastScreenMs >= SCREEN_INTERVAL) {
    refreshScreen();
  }

  delay(20);  // 50 Hz polling
}
