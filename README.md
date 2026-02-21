# UniBuddy — Pocket Desktop Buddy

An Arduino-based physical study companion that helps university students manage focus time through a **Pomodoro timer**, **virtual pet**, and **servo arm nudges** — all without touching a phone.

## Minimum Runnable Unit

The codebase compiles and runs as a self-contained loop on the MCU side (no Linux/WiFi/AI required):

1. **Power on** → e-paper shows splash screen → enters Idle mode.
2. **Long-press button** → starts a 25-minute Pomodoro focus session.
3. **Timer finishes** → records session (EEPROM), servo arm waves, enters 5-min Break (15-min every 4th session).
4. **Break finishes** → automatically starts next Pomodoro.
5. **Short-press button** → cycles Idle ↔ Stats; skips Break when in Break mode.
6. **Shake device** → triggers servo nudge animation, auto-returns to Idle when done.

All libraries have **graceful fallbacks**: if Servo or EEPROM libraries are missing, the firmware still compiles and runs with Serial output.

---

## Hardware

| Component | Pin | Notes |
|---|---|---|
| Waveshare 2.13" e-Paper V4 (250×122, BW) | D7–D11, D13 | SPI bus (see below) |
| SG90 Servo | D6 | PWM; use separate 5V if jitter occurs |
| Push Button | D2 | `INPUT_PULLUP`, active LOW |
| SW-420 Shake Sensor | D3 | `INPUT_PULLUP`, triggers LOW on vibration |

### E-Paper SPI Wiring

| Signal | Pin | Function |
|---|---|---|
| VCC | 5V | Power |
| GND | GND | Ground |
| DIN | D11 | SPI MOSI |
| CLK | D13 | SPI SCK |
| CS | D10 | Chip Select |
| DC | D9 | Data/Command |
| RST | D8 | Reset |
| BUSY | D7 | Busy flag |

**Board**: Arduino Uno Q (STM32 MCU side, Zephyr toolchain).

**Display**: Landscape orientation (ROTATE\_270), 250 px wide × 122 px tall, full 4000-byte framebuffer in RAM. Partial refresh for timer ticks (~1 Hz); full refresh on mode changes.

---

## Software Dependencies

Install via Arduino IDE Library Manager:

| Library | Required? | Fallback |
|---|---|---|
| `Servo` | Recommended | Stub: nudge logged to Serial |
| `EEPROM` | Recommended | RAM-only: sessions lost on reboot |
| `SPI` | Required | Built-in (Arduino core) |

E-paper driver (`epd2in13_V4`) and paint library are bundled in the sketch folder. No external install needed.

> `Servo` and `EEPROM` are auto-detected via `__has_include`. No manual `#define` needed.

---

## Quick Start

1. Open `UniBuddy/UniBuddy.ino` in Arduino IDE.
2. Select your board and serial port.
3. **Compile & Upload**.
4. Open Serial Monitor at **115200 baud** — you should see `[Buddy] Ready!`.

### Test Mode (fast iteration)

Uncomment this line in `UniBuddy/config.h` to use 10s/3s/5s durations instead of 25m/5m/15m:

```cpp
#define TEST_MODE
```

This lets you verify the full Idle → Focus → Break → next-Focus loop in under 30 seconds.

### E-Paper Landscape Test

A standalone test lives in `EpaperMinTest/`:
1. Open `EpaperMinTest/EpaperMinTest.ino`.
2. Upload — screen should show landscape text + shapes, then demo partial refresh.

---

## Controls

| Input | Action |
|---|---|
| **Long-press button** | Idle → Start Pomodoro / Pomodoro → Pause & return to Idle |
| **Short-press button** | Idle ↔ Stats / Break → Skip break |
| **Shake device** | Trigger servo nudge animation |

---

## Code Structure

```
UniBuddy/
├── UniBuddy.ino      Main loop & state machine (setup/loop/mode switch)
├── config.h           Shared pins, timing, AppMode enum, compile toggles
├── input.h            Button debounce (short/long press) & shake sensor
├── pomodoro.h         Focus & break countdown timers (auto long-break)
├── behaviour.h        Session counter & streak persistence (EEPROM / RAM)
├── pet.h              Pet mood enum, 16×16 PROGMEM sprites, blink animation
├── epaper.h           E-paper UI renderer (full / partial refresh, 5 screens)
├── servo_arm.h        SG90 nudge wave animation (7-step keyframe)
├── epd2in13_V4.*      Waveshare e-paper driver (bundled)
├── epdpaint.*         Paint class (Draw*, rotation, framebuffer)
├── epdif.*            SPI hardware abstraction
└── font*.c / fonts.h  Bitmap fonts (8/12/16/20/24 px)

EpaperMinTest/
└── EpaperMinTest.ino  Standalone landscape e-paper test
```

### State Machine

```
         long-press              timer done           break done
  IDLE ──────────► POMODORO ──────────► BREAK ──────────► POMODORO
   ▲  short-press     │                  │ short-press       │
   └──────────────────┘                  └───────► IDLE ◄────┘
   ▲                                                   │
   └──── NUDGE (auto-return when servo sequence ends) ◄┘
         (shake triggers from any mode)

   IDLE ◄──short-press──► STATS
```

### Display Refresh Strategy

| Trigger | Refresh Type | Function |
|---|---|---|
| Mode change (Idle→Focus, etc.) | Full refresh | `fullRefresh()` → `DisplayPartBaseImage()` |
| Timer tick (1 Hz) | Partial refresh | `partialRefresh()` → `DisplayPart()` |
| Every 30 partial refreshes | Auto-full refresh | Prevents ghosting accumulation |

### Architecture (Arduino Uno Q)

```
┌─────────────────────────────────────────────────┐
│              Arduino Uno Q                      │
│  ┌────────────────────┐  ┌───────────────────┐  │
│  │  MCU (STM32)       │  │  MPU (Linux)      │  │
│  │  - E-paper render  │  │  - Python logic   │  │
│  │  - Servo PWM       │  │  - Gemini AI API  │  │
│  │  - Button/Shake    │◄─│  - WiFi (future)  │  │
│  │  - EEPROM save     │  │                   │  │
│  └────────┬───────────┘  └───────────────────┘  │
│           │   SPI / PWM / GPIO                  │
│  ┌────────┴──────────────────────────────────┐  │
│  │  HW: E-Paper │ Servo │ Button │ SW-420   │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

> **MVP scope**: Only the MCU side is implemented. The Linux MPU side (AI chat, WiFi sync) is a future extension.

---

## Serial Protocol (Future — Linux MPU ↔ MCU)

| Direction | Message | Meaning |
|---|---|---|
| MCU → Linux | `SESSION_DONE` | Completed a 25-min pomodoro |
| MCU → Linux | `SHAKE` | User shook the device |
| MCU → Linux | `BOOT` | Device powered on |
| Linux → MCU | `SET_MOOD:2` | Override pet mood (0–4) |
| Linux → MCU | `MSG:Study time!` | Display message on e-paper for 3s |
| Linux → MCU | `STREAK:5` | Update streak display |

---

## What Was Fixed

| Issue | Fix |
|---|---|
| `getSessionCount()` defined in both `pomodoro.h` and `behaviour.h` | Renamed pomodoro-internal counter to `getCompletedCycleCount()` |
| Pomodoro finish didn't start Break timer | Added `startBreak()` call on transition |
| Nudge mode never auto-exited | Returns to Idle when servo sequence completes |
| `setPetMood()` called every render frame, breaking animation | Moved to state transitions only |
| Compilation fails without Servo/EEPROM libs | Added `__has_include` guards with Serial fallback stubs |
| No fast-test option | Added `TEST_MODE` macro (10s focus / 3s break) |
| Unused `_nudgeReps` variable | Removed |
| Break skip left stale timer state | Pet mood properly restored on skip |
| OLED → E-Paper migration | Replaced Adafruit SSD1306 with Waveshare 2.13" V4 driver |
| Servo pin conflict (D9 = e-paper DC) | Moved servo to D6 |
| Waveshare Paint rotation off-by-one | Fixed `width-y` → `width-1-y` in ROTATE\_90/180/270 |
| E-paper refresh too frequent | Event-driven: full refresh on mode change, partial on timer tick |

---

## Feature Roadmap

| Priority | Feature | Status |
|---|---|---|
| P0 | Pomodoro Timer (25/5/15) | Done |
| P0 | Virtual Pet (pixel art moods) | Done |
| P0 | Servo Arm Nudge | Done |
| P0 | Button + Shake Input | Done |
| P0 | Session Counter (EEPROM) | Done |
| P0 | E-Paper Landscape Display | Done |
| P1 | Streak Tracker | Partial (data stored, no day-boundary logic) |
| P1 | Stats Screen | Done |
| P1 | Burnout / Low Energy Mode | Not started |
| P1 | AI Conversational Chat (Linux side) | Not started |
| P2 | Dorm Room Sync (ESP32) | Future |
| P2 | Note Jotting / Calendar | Future |
| P2 | Vibration Motor | Future |
| P2 | Mobile App / AR / Computer Vision | Future |

---

## License

Hack London 2025 — Hardware Track project.
