# Electrochalanes — WRO Future Engineers 2026 - Autonomous Self-Driving Car 🇲🇽

---

## Team

**Electrochalanes** — Tijuana, Baja California, Mexico

---

## Changelog

| Version | Date scope | Key changes |
|---|---|---|
| v1.0 | 2025 national competition | Baseline chassis, standard steering, single lateral sensor (Arduino UNO). |
| v2.0 | Early 2026 season | Bilateral PID (left + right sensors), Ackermann steering geometry, TPU tires, WonderCam color-based turn detection. |
| **v3.0 (current)** | **2026 Nationals** | **Migrated main controller from Arduino UNO R4 Minima to ESP32 + custom shield. Chassis and steering linkage reprinted in black PETG. Open Challenge control strategy rewritten (camera removed, adaptive single-wall PID). Obstacle Challenge gained a parking-exit maneuver and a two-tier (proportional + full-evasion) pillar strategy. Robot assembly video added.** |

This section exists specifically to give reviewers a fast way to see what changed between hardware/software revisions, in addition to the detailed reasoning in each section below.

---

## Repository Contents

| Folder | Contents |
|---|---|
| `t-photos` | 2 team photos (official + fun) |
| `v-photos` | 6 vehicle photos (all sides, top and bottom) |
| `video` | `video.md` with links to autonomous driving demonstrations for both challenges, **plus a full step-by-step robot assembly video** |
| `schemes` | Wiring and electromechanical diagrams (including the custom ESP32 shield) |
| `src` | Complete, commented control software for all programmed components |
| `models` | 3D-printable STL files for custom vehicle parts (now sliced for black PETG) |

---

## Introduction

This repository documents the complete engineering process of our autonomous vehicle for the **WRO Future Engineers 2026**.

As of this revision, the vehicle's main controller is an **ESP32** dev board paired with a **custom shield** (see [Components](#components) and [Power and Sensor Architecture](#2-power-and-sensor-architecture)), replacing the Arduino UNO R4 Minima used previously. The control software is split into two independent sketches, one per challenge:

**Open Challenge** (`src/OpenChallengeNationalsCode.ino`) is organized around:
1. **Sensor module** — sequential HC-SR04 readings (front, left, right)
2. **Adaptive single-wall PID controller** — follows whichever side wall was closer at the start of the run
3. **Corner-direction & turn sequencer** — determines and executes 90° turns using ultrasonic data only (the WonderCam camera is **no longer used** in this challenge — see [Open Challenge — Software Architecture](#open-challenge--software-architecture))
4. **Lap/corner counter and final-stop routine**

**Obstacle Challenge** (`src/ObstaclesChallengeNationalsCode.ino`) is organized around:
1. **Sensor module** — HC-SR04 front/left/right + push-button start trigger
2. **Vision module** — WonderCam color detection (red/green traffic pillars)
3. **Two-tier evasion strategy** — proportional visual steering while a pillar is far away, full stop-reverse-swerve maneuver once it's close
4. **Corner FSM** — 90° turns with a reverse/repositioning phase
5. **Parking-exit sequencer** *(new)* — multi-phase maneuver to leave the starting parking box in the direction with more clearance
6. **Lap/corner counter and stop routine**

To compile and upload either sketch: install **Arduino IDE 2.x**, add the **ESP32 board package** ("esp32" by Espressif Systems, via Boards Manager → additional URL `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`), install the **ESP32Servo** library (the standard `Servo` library is not compatible with the ESP32's PWM/LEDC peripheral) and, for the Obstacle Challenge sketch only, the **WonderCam** library. Open the corresponding `.ino` file, select your board's exact ESP32 dev-module variant, choose the correct COM port, and click Upload. Serial output is enabled at 115200 baud for live debugging via the Arduino IDE Serial Monitor.

---

## Components

| Component | Model | Purpose |
|---|---|---|
| Main controller | **ESP32 dev board** (WROOM-32-based DevKit) | Central processing — sensor reading, PID, motor and servo control |
| **Custom shield** | **Team-designed** | Consolidates all wiring (motor driver, servo, 3× HC-SR04, WonderCam I²C bus, start button) onto a single board mounted on the ESP32's headers, in place of the breadboard wiring used with the Arduino UNO R4 Minima. See [Power and Sensor Architecture](#2-power-and-sensor-architecture) for the electrical reasoning behind it. |
| DC motor driver | TB6612FNG Dual Motor Driver | Bidirectional speed control via PWM |
| Drive motor | 50:1 Micro Metal Gearmotor HPCB 6V | Forward traction |
| Steering servo | Digital Servomotor TD-8125 | Directional control via Ackermann steering |
| Ultrasonic sensor ×3 | HC-SR04 | Front wall detection + wall-following / corner detection |
| Color camera | Hiwonder WonderCam V2.0 | Detects red/green traffic pillars (Obstacle Challenge only) |
| Start button | Push button (Pin 34, `INPUT_PULLUP` + external pull-up on the shield) | Triggers start sequence per WRO rules 9.10–9.11 |
| Logic power | 5V 3A power bank | Supplies the ESP32, sensors, camera, and servo |
| Motor power | 7.4V 400 mAh LiPo | Dedicated traction supply, isolated from logic |
| Structure | **Black PETG** (chassis frame, sensor mounts, steering linkage), TPU (tires), acrylic (base plate), M3/M5 screws, M5/M16 bearings | Chassis, tires, and mechanical assembly |

> **Note on the ESP32 dev board model:** any WROOM-32-based ESP32 DevKit-style board works with the pin map used in the sketches (GPIOs 5, 12–14, 17–19, 21, 22, 32–35). If your team's exact board/module differs, update this row with the precise part number and add its pinout diagram to `schemes/`.

### Why ESP32 instead of the Arduino UNO R4 Minima?

| Consideration | Arduino UNO R4 Minima | ESP32 | Why it mattered for us |
|---|---|---|---|
| Clock speed / headroom | 48 MHz | 240 MHz (dual-core) | More margin for the camera-driven decision logic in the Obstacle Challenge sketch, without slowing down sensor polling. |
| GPIO count | Limited | More available I/O | Needed extra pins once the parking-exit and dual-servo-pin-mapping requirements were added. |
| Native PWM/servo support | `Servo` library, hardware timers shared with other peripherals | Dedicated LEDC PWM peripheral via `ESP32Servo` | Frees up timer conflicts we previously had to work around. |
| I/O voltage | 5V-tolerant | **3.3V-only** | Introduced a real integration risk — see the level-shifting note below. |
| Wireless radios | None | Wi-Fi + Bluetooth on-die | **Must stay unused during official rounds** — WRO 2026 rule 11.10 prohibits any wireless communication while the vehicle is running. The sketches never call `WiFi.begin()` / `BluetoothSerial`, so both radios stay off by default; this is intentional and should not be re-enabled for competition builds. |

**Engineering risk introduced by the migration (and why the custom shield exists):** the HC-SR04's ECHO output is a 5V logic signal, but every ESP32 GPIO is rated for 3.3V input only — driving an ECHO pin directly from a 5V sensor risks damaging the microcontroller over repeated use. Additionally, GPIO34 (used for the start button) is one of the ESP32's *input-only* pins and, unlike most other GPIOs, has **no internal pull-up/pull-down resistor at all**, so `pinMode(PIN_BOTON, INPUT_PULLUP)` in software does not actually provide one. The custom shield exists to solve both problems in one board: resistor voltage dividers on the three ECHO lines (stepping 5V down into the ESP32's safe range) and an external pull-up resistor on the button line, in addition to simply consolidating what used to be loose breadboard wiring. *(If your shield's actual schematic differs from this description, please update this section and `schemes/` accordingly — this description reflects the standard reasoning for this kind of migration and should be checked against your board.)*

---

## 1. Mobility and Mechanical Design

### Chassis and Steering System

The vehicle's structure is based on our 2025 national competition design, with an **Ackermann steering geometry** on the front axle. Ackermann geometry ensures that during a turn, the inner wheel follows a tighter arc than the outer wheel — eliminating lateral tire slip and reducing friction in tight corners. This improves trajectory repeatability, which is critical when relying on a time-based turn duration instead of a gyroscope.

**Material update (v3.0):** the chassis frame, sensor mounts, and the **steering linkage rod** are now printed in **black PETG**, replacing the PLA used previously. Compared to PLA, PETG has meaningfully higher impact resistance and layer adhesion, at a modest cost in stiffness and print-surface finish — a favorable trade for the steering rod specifically, since it is a component under repeated cyclical mechanical stress every time the Ackermann linkage moves. Tires remain **TPU** (grip and vibration absorption) and the base plate remains **acrylic** (visual inspection of wiring during technical checks); those two material choices were unaffected by this update.

M3 screws handle small brackets and sensor attachments; M5 screws with M5/M16 bearings handle the steering pivot and rear axle, ensuring low friction under load.

### Drive Motor Selection

The **50:1 Micro Metal Gearmotor HPCB 6V** (0.74 kg·cm stall torque, ~650 rpm no-load) was selected after evaluating two options:

| Option | Ratio | Speed | Torque | Result |
|---|---|---|---|---|
| **Chosen: HPCB 6V 50:1** | 50:1 | ~650 rpm | 0.74 kg·cm | ✅ Sufficient torque, controllable speed for closed-loop control |
| Discarded: high-speed motor | 10:1 | ~3000 rpm | ~0.15 kg·cm | ❌ Too fast — the control loop could not correct lateral drift in time |

The 50:1 ratio provides enough torque to accelerate smoothly while keeping speed low enough for the wall-following controller to react before lateral drift exceeds recoverable limits. This reasoning is unchanged by the ESP32 migration; the motor and driver stage were not modified.

### Iteration History

| Version | Change | Reason |
|---|---|---|
| V1 (2025) | Standard steering, single lateral sensor | Baseline from 2025 national competition |
| V2 | Bilateral PID using left + right sensors | V1 drifted toward the wider wall; bilateral error fixed this |
| V3 | Ackermann steering geometry, TPU tires | Standard steering caused ~8° corner angle error per turn due to tire slip |
| **V4 (current)** | **ESP32 + custom shield; black PETG chassis and steering rod; Open Challenge vision removed in favor of adaptive single-wall PID; Obstacle Challenge gained a parking-exit maneuver** | **More processing headroom and I/O for the added parking-exit logic; PETG's toughness over PLA for the steering linkage; simplifying the Open Challenge control loop by removing a camera dependency that wasn't needed once the sensor-only cornering logic proved reliable** |

---

## 2. Power and Sensor Architecture

### Power Budget

Two independent power rails isolate motor switching noise from logic and sensor signals. Figures below are typical component ratings for this hardware revision; **please re-verify with a multimeter on your actual assembled shield**, since exact draw depends on the ESP32 board variant and shield regulation.

**Rail 1 — Logic (5V, 3A power bank):**

| Component | Current (approx.) |
|---|---|
| ESP32 dev board (Wi-Fi/Bluetooth unused) | ~150 mA |
| WonderCam V2.0 *(Obstacle Challenge sketch only)* | ~300 mA |
| 3× HC-SR04 sensors | ~45 mA total (~15 mA each) |
| TD-8125 servo (peak) | ~1,500 mA |
| TB6612FNG logic stage | <10 mA |
| **Total peak — Obstacle Challenge (camera active)** | **~2,005 mA** |
| **Total peak — Open Challenge (no camera in this sketch)** | **~1,705 mA** |

Servo peaks are short and do not coincide with ESP32 or camera peaks, so worst-case sums are conservative.

**Rail 2 — Traction (7.4V, 400 mAh LiPo):**

| Component | Current |
|---|---|
| 50:1 gearmotor (peak) | ~1,500 mA |
| TB6612FNG power stage | ~100 mA |
| **Total peak** | **~1,600 mA** |

Estimated autonomy: 0.4 Ah ÷ 1.6 A = **~15 minutes** — more than sufficient for 3-minute runs. This estimate is unchanged from the previous hardware revision, since the traction rail was not modified.

> **Why two rails?** During early testing, motor acceleration caused voltage dips on a shared supply that reset the microcontroller mid-run. Splitting into two independent rails eliminated this failure mode entirely, and the same reasoning carried over unchanged to the ESP32 migration.

### Sensor Selection and Placement

**Why HC-SR04?**
The HC-SR04 is a widely available, cost-effective sensor with a well-documented standard protocol (10 µs HIGH trigger pulse, echo read on HIGH), reliable performance at the distances used in this competition (10–200 cm), and straightforward integration via `pulseIn`. Its main integration cost on this hardware revision is the 5V/3.3V logic mismatch discussed above, addressed via the custom shield's level shifting.

| Sensor | Position | Height | Justification |
|---|---|---|---|
| Front | Center-front of chassis | 12 cm | Avoids reading the floor; detects walls reliably from the corner-trigger distance used by each challenge |
| Left | Above front-left wheel | 10 cm | Reads lateral wall distance for wall-following; positioned forward to detect walls before the chassis passes them |
| Right | Above front-right wheel | 10 cm | Symmetric to left, used interchangeably depending on which side the controller decides to follow |

The WonderCam (Obstacle Challenge only) is mounted facing forward/down, positioned to detect red and green traffic pillars ahead of the vehicle and to compute each pillar's horizontal position and bounding-box area.

### Pin Map

The two sketches assign a few pins differently, because the Obstacle Challenge sketch needs the ESP32's default I²C pins (GPIO21/22) for the WonderCam, while the Open Challenge sketch is free to use GPIO21 for the servo instead:

| Signal | Open Challenge | Obstacle Challenge |
|---|---|---|
| Front TRIG / ECHO | 33 / 19 | 33 / 19 |
| Right TRIG / ECHO | 18 / 5 | 18 / 5 |
| Left TRIG / ECHO | 32 / 35 | 32 / 35 |
| Steering servo | **21** | **17** |
| I²C SDA / SCL (WonderCam) | *not used* | **21** / 22 |
| Start button | 34 | 34 |
| Motor PWM / AIN1 / AIN2 | 14 / 12 / 13 | 14 / 12 / 13 |

---

## Open Challenge — Software Architecture

The Open Challenge no longer uses the WonderCam at all. Turn direction and lane-keeping are both derived purely from the three HC-SR04 sensors. This is a deliberate simplification from the previous camera-based approach: once the sensor-only cornering logic (comparing which side opens up at a corner) proved reliable in testing, the camera dependency added complexity without adding accuracy for this challenge, so it was removed. The Obstacle Challenge sketch still uses the camera, since it needs to identify pillar *color*, which distance sensors cannot do.

### Control Flow

```
                     ┌────────────────────────┐
                     │  IDLE (button not yet   │
                     │  pressed)                │
                     └───────────┬─────────────┘
                                 │ button press
                                 ▼
                   Capture initial left/right distance
                   as PID setpoints → follow whichever
                   side is CLOSER for the whole run
                                 │
                                 ▼
                     ┌────────────────────────┐
             ┌──────►│   WALL-FOLLOWING PID    │
             │       │ (single sensor, fixed   │
             │       │  setpoint from start)   │
             │       └───────────┬─────────────┘
             │                   │ front < DISTANCIA_GIRO
             │                   │ AND one side opens up
             │                   ▼
             │       ┌────────────────────────┐
             │       │   CORNER TURN           │
             │       │ (first corner picks &   │
             │       │  locks the direction;   │
             │       │  later corners repeat   │
             │       │  the same direction)    │
             │       └───────────┬─────────────┘
             │                   │ corner count < 12
             └───────────────────┘
                                 │ corner count == 12
                                 ▼
                     Drive straight briefly, stop
```

### Algorithm 1 — HC-SR04 Sequential Reading

All three sensors are read one at a time every loop cycle:

```cpp
float leerDistancia(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);          // HC-SR04: 10 µs HIGH pulse to trigger
  digitalWrite(trig, LOW);
  unsigned long duracion = pulseIn(echo, HIGH, 30000);   // 30 ms timeout -> ~5 m max range
  if (duracion == 0) return -1;
  return duracion / 58.0;         // standard HC-SR04 time-of-flight -> cm conversion
}
```

A reading of `-1` (timeout) is treated as "no update" rather than "wall is very far": the loop only overwrites `ulLecturaFrente/Der/Izq` when a reading is valid, so a single dropped echo doesn't erase a good previous reading.

### Algorithm 2 — Adaptive Single-Wall PID

Rather than the previous bilateral (left-minus-right) centering controller, this revision follows **one wall only**, chosen at the start of the run:

```cpp
// At start: capture each side's distance as that side's setpoint
setpoint_Der = (ulLecturaDer < 400) ? ulLecturaDer : 40.0;
setpoint_Izq = (ulLecturaIzq < 400) ? ulLecturaIzq : 40.0;

// Whichever side is CLOSER (smaller setpoint) is the one that gets followed
if (setpoint_Der > setpoint_Izq) {
  opcion_2 = 1;   // right wall is farther -> follow the (closer) left wall
} else {
  opcion_2 = 2;   // left wall is farther (or equal) -> follow the (closer) right wall
}
```

```cpp
void PIDderecho() {
  error = ulLecturaDer - setpoint_Der;

  double P = Kp * error;
  double D = Kd * ((error - error_anterior) / dt);
  // Conditional integration: only accumulate the integral if doing so keeps
  // the *unclamped* output inside the same band the final output is
  // clamped to — a simple anti-windup trick.
  double prueba = P + D + Ki * (suma_errores + error * dt);
  if (constrain(prueba, -20, 20) == prueba) {
    suma_errores += error * dt;
    suma_errores = constrain(suma_errores, -70, 70);
  }
  double I = Ki * suma_errores;
  double salidaPID = constrain(P + I + D, -20, 20);

  int anguloServo = CENTRO + (int)salidaPID;
  moverServo(anguloServo);
  adelante(VEL_NORMAL);
  error_anterior = error;
}
```

`PIDizquierdo()` mirrors this exactly, with the error sign flipped (`setpoint_Izq - ulLecturaIzq`) so that a positive PID output always means "steer right" under both functions, using the same gains and clamps.

**Why adaptive-setpoint single-wall following instead of bilateral centering?** Bilateral centering (error = left − right) is setpoint-free and adapts automatically to any corridor width, which is why V2 used it. The trade-off is that it requires both sensors to be reliable simultaneously. The single-wall approach trades that generality for simplicity: it commits to one wall at the start (whichever is closer, and therefore whichever gives a stronger, more reliable signal) and only needs that one sensor to behave well for the rest of the run. This is a deliberate simplification, not strictly an improvement — teams reproducing this design should note it assumes a roughly consistent corridor width within a round, since a mid-run corridor-width change would not be compensated for the way bilateral centering would.

**Tuning (current gains):**

| Kp | Ki | Kd |
|---|---|---|
| 10.0 | 0.05 | 5.0 |

Note these gains are **not directly comparable in magnitude** to the previous bilateral controller's `Kp=1.2, Ki=0.04, Kd=0.15`, since the earlier controller's error term was the difference of two sensors (typically small, since both sides are similar magnitudes) while this one's error term is a single sensor's raw deviation in cm from a fixed setpoint (potentially larger swings) — a like-for-like re-tune was required after the algorithm change, not just a hardware swap.

### Algorithm 3 — Turn Direction and Cornering (Sensor-Only)

A corner is recognized when the front sensor reads closer than `DISTANCIA_GIRO` (80 cm) **and** at least one lateral sensor reads more than 100 cm — the second condition distinguishes an actual corner opening from simply being close to a wall in a narrow straight.

```cpp
if (opcion == 0) {
  // First corner: stop, let the chassis settle, then re-read both sides
  detenerRobot();
  delay(1500);
  float lIzq = leerDistancia(TRIG_IZQ, ECHO_IZQ);
  float lDer = leerDistancia(TRIG_DER, ECHO_DER);
  if (lIzq > 0) ulLecturaIzq = lIzq;
  if (lDer > 0) ulLecturaDer = lDer;

  // Turn toward whichever side has more open space — that's where the track continues
  if (ulLecturaIzq > ulLecturaDer) {
    opcion = 1;       // more room on the left -> turn left at every corner from now on
    girarIzquierda();
  } else {
    opcion = 2;       // more room on the right -> turn right at every corner from now on
    girarDerecha();
  }
} else if (opcion == 1) {
  girarIzquierda();   // keep the same direction as the first corner
} else if (opcion == 2) {
  girarDerecha();
}
```

`girarIzquierda()`/`girarDerecha()` are open-loop, timed maneuvers: steer to full lock, drive through the corner for `TGIRO` ms, recenter, then drive straight for `TFront` ms before the PID loop resumes (this last step gives the chassis room to clear the corner before its lateral readings are trusted again).

This replaces the previous WonderCam-based turn detection entirely. There is no fallback path needed here (unlike the old camera-based version, whose fallback *was* this same sensor comparison) — this **is** now the only mechanism.

### Edge Cases

| Situation | Behavior |
|---|---|
| Front sensor timeout (`-1`) mid-run | Last known-good front reading is kept; a corner is not spuriously triggered by a single dropped echo |
| Both lateral sensors read ≥ 400 cm at start | Both setpoints default to 40 cm |
| `dt` computed as ~0 (very fast consecutive loop iterations) | Clamped to a 0.001 s minimum to avoid a divide-by-zero in the derivative term |
| 12th corner completed | Drives straight for `TFinal` ms to clear the area, then stops for good |

---

## Obstacle Challenge — Software Architecture

### Overview

The Obstacle Challenge program combines: a start-of-run **parking-exit maneuver**, a **two-tier pillar-evasion strategy** driven by the WonderCam, and a **corner FSM** for the track's 90° turns. A lap counter (`contadorGiros`) tracks completed corners; after 12 corners (3 laps) the run stops.

### Priority Order

Each `loop()` pass (throttled to ~10 Hz for the decision logic, while sensors are still polled every cycle) evaluates, in order:

1. **Start button** — arms the robot and, on the very first press, triggers the parking-exit maneuver (per WRO rule: the vehicle must not move until the operator releases it)
2. **Color detection & evasion decision** — if either pillar color is in view, this branch takes over exclusively for that cycle
3. **Corner detection** — only checked when no pillar is currently in view; triggers the corner FSM if the front wall is close and a side is open

### Corner FSM

```
DRIVING STRAIGHT ──(front < DISTANCIA_FRENTE AND a side is open)──► TURN (steer to lock, drive Tgiro ms)
                                                                          │
                                                                (pause Talto ms)
                                                                          │
                                                                          ▼
                                                                REVERSE, wheel angled
                                                                (Tatras ms)
                                                                          │
                                                                          ▼
                                                        contadorGiros++ → DRIVING STRAIGHT
                                                        (if contadorGiros == 12 → STOP)
```

```cpp
void girarDerecha() {
  moverServo(Acentral + 40);
  adelante(VELOCIDAD_GIRO);
  delay(Tgiro);
  moverServo(Acentral);
  detenerRobot();
  delay(Talto);
  moverServo(Areversa);        // wheel angled while reversing, to help straighten the heading
  atras(VELOCIDAD_GIRO);
  delay(Tatras);
  detenerRobot();
  delay(500);
  adelante(VELOCIDAD_NORMAL);
  moverServo(Acentral);
  delay(1000);
}
```

`girarIzquierda()` mirrors this for left turns.

### Algorithm — Two-Tier Pillar Evasion

Rather than a single fixed-distance trigger, the current logic reacts differently depending on how close a pillar is:

- **Far away** (bounding-box area below threshold): a light **proportional** nudge keeps the pillar roughly centered in frame, without committing to a full evasion yet:
  ```cpp
  int angulo = map(centroX, 0, 320, 51, 111);
  moverServo(constrain(angulo, 51, 111));
  adelante(VELOCIDAD_NORMAL);
  ```
- **Close enough** (bounding-box area above threshold): a full 3-phase evasion maneuver runs — reverse briefly, swerve past the pillar on the rule-mandated side, drive straight, then swerve back to center:
  ```cpp
  void esquivarRojo() {          // passes a RED pillar on the RIGHT, per WRO rules
    detenerRobot();
    delay(500);
    atras(VELOCIDAD_NORMAL);
    delay(500);
    detenerRobot();
    delay(500);
    adelante(VELOCIDAD_ESQUIVE);
    moverServo(AesquiveRojo + 10);
    delay(Tesquive);
    adelante(VELOCIDAD_ESQUIVE);
    moverServo(Acentral);
    delay(Tcentral);
    adelante(VELOCIDAD_ESQUIVE);
    moverServo(AreacomodoRojo - 10);
    delay(Treacomodo + 200);
    moverServo(Acentral);
  }
  ```
  `esquivarVerde()` mirrors this, steering left instead, to pass a **GREEN** pillar on the **LEFT** — also per WRO rules.


### Algorithm — Parking-Exit Maneuver *(new)*

On the first button press, the robot reads which side of the parking box has more clearance and executes a fixed 5-phase sequence to back out of it — the same alternating forward/reverse, opposite-lock pattern a person uses to exit a tight parallel parking spot:

| Phase | Action | Steering | Duration |
|---|---|---|---|
| 1 | Forward | Full lock, away from the box | `Tadelante_salida` (350 ms) |
| 2 | Reverse | Full lock, opposite way | `Tatras_salida` (300 ms) |
| 3 | Forward | Full lock, away from the box | `Tadelante_afuera` (1000 ms) |
| 4 | Forward | Full lock, opposite way | `Tadelante_afuera` (1000 ms) |
| 5 | Reverse | Full lock, away from the box | `Tatras_Acomodo` (600 ms) |

```cpp
void salirEstacionamientoDerecha() {
  moverServo(Asalida_derecha);
  adelante(Velocidad_salida);
  delay(Tadelante_salida);
  detenerRobot();
  delay(500);
  moverServo(Asalida_izquierda);
  atras(Velocidad_salida);
  delay(Tatras_salida);
  detenerRobot();
  delay(500);
  moverServo(Asalida_derecha);
  adelante(Velocidad_salida);
  delay(Tadelante_afuera);
  detenerRobot();
  delay(500);
  moverServo(Asalida_izquierda);
  adelante(Velocidad_salida);
  delay(Tadelante_afuera);
  detenerRobot();
  delay(500);
  moverServo(Asalida_derecha);
  atras(Velocidad_salida);
  delay(Tatras_Acomodo);
  detenerRobot();
}
```

`salirEstacionamientoIzquierda()` is the mirror image, used when the left side has more clearance. Whichever side is chosen here (`opcion`) is reused for every corner turn direction for the rest of the run, keeping the exit direction and the lap direction consistent.

**Known scope limitation:** after the 12th corner, the robot simply stops where it is — there is currently no automated maneuver to return to and re-park inside the parking box. This is a reasonable next iteration if the team wants to pursue the final-parking bonus described in the rulebook (Section 6, "Estacionamiento en el cajón de estacionamiento").

### Edge Cases

| Situation | Behavior |
|---|---|
| Green detection too low in frame (`centroY2 > 150`) | Treated as noise (block likely already alongside/behind the robot) and its area is zeroed out |
| Pillar detected while mid-corner-turn | Corner maneuvers are blocking (`delay`-based), so a pillar can't interrupt one already in progress |
| No pillar and no corner condition | Drive straight at `VELOCIDAD_NORMAL`, keep polling |
| Camera update fails (`Cam.updateResult()` returns false) | Cycle is skipped (`return`) rather than acting on stale/garbage data |

---

## Obstacle Challenge — Source Code

The complete, commented source for both challenges lives at:
- `src/open_challenge/OpenChallengeNationalsCode.ino`
- `src/obstacle_challenge/ObstaclesChallengeNationalsCode.ino`

---

## 4. Systems Thinking and Engineering Decisions

### Subsystem Interaction

```
[Power Bank 5V] ──► [ESP32 + Custom Shield] ──► [TB6612FNG] ◄── [LiPo 7.4V]
                               │                       │
                    ┌──────────┼──────────┐       [DC Motor 50:1]
                    │          │          │
              [WonderCam*]  [3× HC-SR04] [Servo TD-8125]
                    │          │          │
             [Color/Pillar [Wall-follow /  [Ackermann
              detection*]   corner detect]  Steering]

  * WonderCam and color detection are used in the Obstacle Challenge sketch only.
```

### Trade-offs

**ESP32 migration vs. staying on Arduino UNO R4 Minima:** more processing headroom and I/O simplified adding the parking-exit logic and the dual pin-map needed once the camera's I²C bus competed for GPIO21. The cost was a genuine integration risk (3.3V-only logic against 5V sensors, and a button pin with no internal pull resistor), which is why a custom shield was designed rather than wiring the ESP32 directly the way the Arduino board had been.

**PETG vs. PLA for the chassis and steering rod:** PETG's added toughness directly targets the steering linkage, a component under repeated cyclic load every time the Ackermann geometry articulates; the trade-off is a small step down in dimensional stiffness and print finish compared to PLA, which was judged acceptable for a functional part that isn't a wear-tolerance-critical fit.

**Removing the camera from the Open Challenge vs. keeping bilateral PID + vision fallback:** simplifies the sketch and removes one sensing dependency, at the cost of losing bilateral centering's automatic adaptation to changing corridor width mid-run. Given the Open Challenge's corridor width is fixed for the duration of a single round, this trade was judged acceptable.

**Two-tier pillar evasion vs. a single fixed-distance trigger:** reacting proportionally while a pillar is still far away, and only committing to the full blocking evasion maneuver once it's close, reduces unnecessary full stops when a pillar is visible but not yet actually in the vehicle's path.

**Time-based vs. gyroscope-based turns:** unchanged reasoning from previous versions — a gyroscope would give exact 90° turns, but time-based turns avoid the extra wiring and failure points of an IMU, and remain reliable as long as battery voltage is consistent (guaranteed by the dedicated LiPo traction rail).

### Risk Assessment

| Risk | Mitigation |
|---|---|
| ESP32 GPIO damage from 5V HC-SR04 echo signals | Voltage-divider level shifting on the shield (see [Components](#components)) |
| Unreliable button reads (GPIO34 has no internal pull resistor) | External pull-up resistor on the shield |
| Wi-Fi/Bluetooth radios interfering with rule 11.10 compliance | Sketches never initialize either radio; verify this remains true in any future firmware update |
| Pillar evasion triggered on the wrong side (see code review finding above) | Flagged in source and documentation; **requires field verification before competition** |
| Corner never detected (front/lateral sensor timeout at the critical moment) | Last known-good reading is retained rather than reset to zero, avoiding a spurious "wall is gone" false read |
| Servo command outside a safe range (Obstacle sketch's `moverServo()` doesn't `constrain()`) | All current constants are within [45°, 135°], a safe range for the TD-8125; recommend adding a `constrain()` call to match the Open Challenge sketch's safer implementation |

---

## 5. Configurable Parameters — Open Challenge

| Constant | Default | Description |
|---|---|---|
| `CENTRO` | 81 | Servo angle for straight-ahead driving |
| `VEL_NORMAL` | 180 | Cruise motor speed (PWM 0–255) |
| `VEL_GIRO` | 210 | Motor speed during a corner |
| `DISTANCIA_GIRO` | 80 cm | Front distance that triggers a turn |
| `TOTAL_GIROS` | 12 | Total corners before automatic stop (3 laps × 4 corners) |
| `TGIRO` | 1400 ms | Duration of full-lock steering while driving through a corner |
| `TFront` | 1000 ms | Straight-driving duration right after a turn, before PID resumes |
| `TFinal` | 2400 ms | Straight-driving duration after the 12th corner, before stopping |
| `Kp` | 10.0 | PID proportional gain |
| `Ki` | 0.05 | PID integral gain |
| `Kd` | 5.0 | PID derivative gain |
| `setpoint_Der` / `setpoint_Izq` | captured at start (default 40 cm) | Target distance from whichever wall is being followed |

## Configurable Parameters — Obstacle Challenge

| Constant | Default | Description |
|---|---|---|
| `Acentral` | 80° | Servo angle for straight-ahead driving |
| `Areversa` | 51° | Steering angle while reversing after a right turn |
| `Asalida_izquierda` / `Asalida_derecha` | 45° / 135° | Full-lock angles used during the parking-exit maneuver |
| `AesquiveRojo` / `AreacomodoRojo` | 111° / 51° | Evasion / recovery angles for a red pillar |
| `AesquiveVerde` / `AreacomodoVerde` | 51° / 111° | Evasion / recovery angles for a green pillar |
| `VELOCIDAD_NORMAL` | 170 | Cruise motor speed (PWM 0–255) |
| `VELOCIDAD_GIRO` | 180 | Motor speed during a corner |
| `VELOCIDAD_ESQUIVE` | 200 | Motor speed during pillar evasion |
| `Velocidad_salida` | 190 | Motor speed during the parking-exit maneuver |
| `DISTANCIA_FRENTE` | 70 cm | Front distance that triggers a corner turn |
| `Tgiro` | 1600 ms | Duration of full-lock steering while driving through a corner |
| `Talto` | 500 ms | Pause at full lock before reversing after a corner |
| `Tatras` | 1000 ms | Reverse duration after a corner |
| `Tesquive` | 700 ms | Evasion phase 1 duration (initial swerve) |
| `Tcentral` | 700 ms | Evasion phase 2 duration (straight past the pillar) |
| `Treacomodo` | 500 ms | Evasion phase 3 duration (recovery swerve) |
| `Tadelante_salida` / `Tatras_salida` | 350 ms / 300 ms | Parking-exit phases 1–2 durations |
| `Tadelante_afuera` | 1000 ms | Parking-exit phases 3–4 durations |
| `Tatras_Acomodo` | 600 ms | Parking-exit phase 5 duration |
| `AREA_MINIMA_ROJO` | 3500 px² | Bounding-box area above which a red-triggered evasion fires (see code review note) |
| `AREA_MINIMA_VERDE` | 6000 px² | Bounding-box area above which a green-triggered evasion fires (see code review note) |
| `TOTAL_GIROS` | 12 | Total corners before automatic stop (3 laps × 4 corners) |

---

## Known Issues / Roadmap

This section exists so reviewers (and future us) can see open items at a glance, in addition to the trade-off reasoning above:

1. **No automated final parking maneuver** — the Obstacle Challenge run currently stops after the 12th corner rather than returning to the parking box.
2. **`moverServo()` without `constrain()` in the Obstacle Challenge sketch** — currently safe given the constants in use, but worth hardening to match the Open Challenge sketch.
