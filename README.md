# UniBuddy — Your Desk-Sized Study Companion

> A physical Pomodoro pet that lives on your desk, reacts to touch and motion, and keeps you focused — no phone required.

Built on Arduino UNO R4 WiFi with a Waveshare 2.13" e-Paper display, Modulino Movement IMU, tap sensor, servo arm, and a 3D-printed enclosure.

<p align="center">
  <a href="https://www.youtube.com/watch?v=GjLW2NmiDvU">
    <img src="feature_img.png" alt="UniBuddy Demo Video" width="600">
  </a>
  <br>
  <em>▶ Click to watch the demo</em>
</p>

---

## Why UniBuddy?

| Problem | UniBuddy's Answer |
|---|---|
| Phone timers lead to distraction | A dedicated physical device — no screen temptation |
| Productivity tools feel cold | A virtual pet with different mood that reacts to how you study |
| Pomodoro apps need manual input | Tilt to start — flip the device and the timer begins |
| No feedback loop between sessions | Pet mood evolves based on your session count and habits |

---

## Key Features

### Tilt-Driven Modes — No Buttons Needed

Simply rotate UniBuddy to switch between modes. An on-board IMU with EMA filtering, hysteresis, and shake lockout ensures smooth, reliable transitions.

| Orientation | Mode | Description |
|---|---|---|
| Standing upright | **Pet** | Your idle companion — eyes follow, blinks, reacts |
| Flat on desk | **Sleep** | Low-power resting face with Zzz animation |
| Tilted left / right | **Info** | Portrait calendar / temperature display |
| Flipped upright | **Focus** | 25-min Pomodoro timer with progress bar |
| Face-down | **Off** | Display stops updating |

### 14-Mood Virtual Pet

The pet's emotional state changes dynamically based on your interactions:

- **Shake gently** → cute reaction with sparkle eyes
- **Shake repeatedly** → surprised → annoyed → dizzy
- **Complete focus sessions** → happy / interested / cute
- **Study too many sessions** → tired / sad (take a break!)
- **Leave idle too long** → bored

Each mood has a **unique eye style** with pixel-art expressions drawn in real-time on the e-paper display.

### Pomodoro Focus System

- **25-min focus / 5-min break** cycle (15-min break every 4th session)
- **Tap to pause / resume** the timer mid-session
- Animated progress bar + countdown on the focus screen
- Session counter with dot indicators (resets every 4 cycles)
- Break-complete notification with optional **servo arm nudge**

### Physical Interactions

| Input | Action |
|---|---|
| **Single tap** (KY-031) | Pause/resume Pomodoro (focus mode) · Pet reacts (pet mode) |
| **Double tap** | Toggle **Night Mode** (works in any mode) |
| **Shake** | Graduated mood reactions + servo nudge |
| **Button short-press** | Start new Pomodoro when idle |
| **Button long-press** | Reset all sessions (pet mode) |

### Night Mode

Double-tap anywhere to invert the display — black background with white foreground, crescent moon and scattered stars. Every drawing primitive dynamically switches via `FG()`/`BG()` colour abstraction, so all 14 eye styles, mood symbols, and screen layouts render correctly in both modes.

### Servo Arm

An SG90 micro servo provides a physical "nudge" animation when focus sessions complete or when you shake the device. Fully optional — the firmware compiles and runs without `Servo.h`.

---

## Hardware

| Component | Interface | Notes |
|---|---|---|
| Arduino UNO R4 WiFi | — | Main MCU (Renesas RA4M1) |
| Waveshare 2.13" e-Paper V4 | SPI (D7–D11, D13) | 250×122 BW, partial refresh ~1 Hz |
| Modulino Movement (LSM6DSOX) | I²C | 6-axis IMU for tilt + shake |
| KY-031 Tap Sensor | D2 (ISR) | Knock detection via interrupt |
| Push Button | D4 | Debounced, short/long press |
| SG90 Servo | D6 (PWM) | Optional nudge arm |
| 3D-Printed Enclosure | — | STL files in `CAD/` |

### E-Paper SPI Wiring

| Signal | Pin |
|---|---|
| DIN (MOSI) | D11 |
| CLK (SCK) | D13 |
| CS | D10 |
| DC | D9 |
| RST | D8 |
| BUSY | D7 |

---

## Software Architecture

```
UniBuddy/
├── UniBuddy.ino       Main loop & tilt-driven state machine
├── config.h           Pin assignments, timing constants, mode enums
├── input.h            Button + KY-031 tap (ISR) + movement polling
├── tilt.h             IMU EMA filter, tilt classification, shake detection
├── pet.h              14-mood system, animation phases, shake reactions, idle decay
├── pomodoro.h         Focus & break timers with pause/resume
├── behaviour.h        Session counter & EEPROM streak persistence
├── epaper.h           Full rendering engine (all screens, night mode, mood art)
├── servo_arm.h        Servo nudge sequence (auto-detected, optional)
├── calendar.h         RTC + temperature display (requires RTClib)
├── epd2in13_V4.*      Waveshare e-paper driver (bundled)
├── epdpaint.*         Paint class with rotation & framebuffer
├── epdif.*            SPI hardware abstraction
└── font*.c / fonts.h  Bitmap fonts (8/12/16/20/24 px)
```

### State Machine

```
                    tilt flip                timer done              break done
  PET (upright) ──────────────► POMODORO ──────────────► BREAK ──────────────► PET
                                  ▲  tap: pause/resume     │
                                  └────────────────────────┘

  PET ◄──tilt flat──► SLEEP       PET ◄──tilt side──► INFO (calendar/temp)

  Any mode ── double tap ──► toggle Night Mode
  Any mode ── face down ──► OFF (display frozen)
```

### Display Refresh Strategy

| Trigger | Refresh Type | Details |
|---|---|---|
| Mode change / night toggle | **Full refresh** | Clean transition, no ghosting |
| Timer tick (~1 Hz) | **Partial refresh** | Fast update, low flicker |
| Every 30 partial cycles | **Auto full refresh** | Prevents ghost accumulation |

---

## Quick Start

1. Open `UniBuddy/UniBuddy.ino` in **Arduino IDE 2.x**.
2. Install board support: **Arduino UNO R4 Boards** via Board Manager.
3. Install libraries via Library Manager:
   - `Modulino` (Arduino)
   - `RTClib` (Adafruit) — optional, for calendar mode
4. Select **Arduino UNO R4 WiFi** and your serial port.
5. **Compile & Upload**.
6. Open Serial Monitor at **115200 baud** — you should see `=== UniBuddy ===`.

### Test Mode

Uncomment in `config.h` for rapid iteration (10s focus / 3s break / 5s long break):

```cpp
#define TEST_MODE
```

---

## 3D-Printed Enclosure

STL files are provided in the `CAD/` directory:

| File | Description |
|---|---|
| `CASE.stl` | Main body housing all electronics |
| `LID.stl` | Snap-fit lid with e-paper window |

---

## Graceful Fallbacks

The firmware auto-detects optional libraries at compile time via `__has_include`:

| Library | Missing? | Fallback |
|---|---|---|
| `Servo.h` | Nudge disabled | Serial log only |
| `EEPROM.h` | Persistence disabled | RAM-only session counter |
| `RTClib.h` | Calendar disabled | Info screen shows placeholder |

No `#define` toggles needed — it just works.

---

## Roadmap

| Status | Feature |
|---|---|
| ✅ Done | Pomodoro timer (25/5/15) with pause/resume |
| ✅ Done | 14-mood virtual pet with animated eyes |
| ✅ Done | Tilt-driven mode switching (IMU + EMA + hysteresis) |
| ✅ Done | Shake detection with graduated mood reactions |
| ✅ Done | Night mode (full display inversion + moon/stars) |
| ✅ Done | Tap to pause/resume Pomodoro + pet interaction |
| ✅ Done | Session counter with EEPROM persistence |
| ✅ Done | E-paper multi-screen rendering (5 modes) |
| ✅ Done | 3D-printed enclosure (STL provided) |
| ✅ Done | Calendar / temperature info screen (RTC + thermo) |
| 📋 Planned | Streak tracking with day-boundary logic |
| 📋 Planned | Linux MPU integration (AI chat via Gemini API) |

---

**Hack London 2026** — Hardware Track project.
