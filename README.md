Engineering materials
====

# Electrochalanes — WRO Future Engineers 2026 - Autonomous Self-Driving Car 🇲🇽

---

## Team

**Electrochalanes** — Tijuana, Baja California, Mexico

---

## Repository Contents

| Folder | Contents |
|---|---|
| `t-photos` | 2 team photos (official + fun) |
| `v-photos` | 6 vehicle photos (all sides, top and bottom) |
| `video` | `video.md` with links to autonomous driving demonstrations |
| `schemes` | Wiring and electromechanical diagrams  |
| `src` | Complete control software for all programmed components |
| `models` | 3D-printable STL files for custom vehicle parts |

---

## Introduction

This repository documents the complete engineering process of our autonomous vehicle for the **WRO Future Engineers 2026**. 

The open challenge software is organized into a single Arduino sketch that integrates six modules:

1. **Sensor module** — Sequential URM37 ultrasonic readings (front, left, right)
2. **Vision module** — WonderCam color detection for turning direction
3. **PID controller** — Bilateral lane centering using left/right sensor error
4. **State machine** — Six-state FSM controlling overall vehicle behavior
5. **Turn sequencer** — Time-based 90° cornering with camera/sensor fallback
6. **Stop locator** — Sensor-based return to starting position after 3 laps

To compile and upload: install **Arduino IDE 2.x**, add the **Arduino UNO Q** board package, install the **WonderCam** and **Servo** libraries, open the `.ino` file, select board **Arduino UNO Q**, choose the correct COM port, and click Upload. For real-time debugging, uncomment `#define DEBUG` to enable Serial Monitor output at 9600 baud.

---

## Components

| Component | Model | Purpose |
|---|---|---|
| Main controller | Arduino UNO Q | Central processing — sensor reading, PID, motor and servo control |
| DC motor driver | TB6612FNG Dual Motor Driver | Bidirectional speed control via 10-bit PWM |
| Drive motor | 50:1 Micro Metal Gearmotor HPCB 6V | Forward traction |
| Steering servo | Digital Servomotor TD-8125 | Directional control via Ackermann steering |
| Ultrasonic sensor ×3 | DFRobot URM37 V5.0 | Front wall detection + bilateral lane centering |
| Color camera | Hiwonder WonderCam V2.0 | Detects orange/blue floor lines to decide turning direction |
| Start button | Push button (Pin 7, INPUT_PULLUP) | Triggers start sequence per WRO rules 9.10–9.11 |
| Logic power | 5V 3A power bank | Supplies Arduino, sensors, camera, and servo |
| Motor power | 7.4V 400 mAh LiPo | Dedicated traction supply, isolated from logic |
| Structure | PLA, TPU, acrylic, M3/M5 screws, M5/M16 bearings | Chassis, tires, and mechanical assembly |

---

## 1. Mobility and Mechanical Design

### Chassis and Steering System

The vehicle's structure is based on our 2025 national competition design, substantially revised with a new **Ackermann steering geometry** for the front axle. Ackermann geometry ensures that during a turn, the inner wheel follows a tighter arc than the outer wheel — eliminating lateral tire slip and reducing friction in tight corners. This improves trajectory repeatability, which is critical when relying on a time-based turn duration (`T_GIRO_MS`) instead of a gyroscope.

The chassis uses three materials selected for specific mechanical roles:
- **PLA** for structural rigidity of the main frame and sensor mounts
- **TPU** for the tires, providing grip and vibration absorption without brittleness
- **Acrylic** for the base plate, enabling visual inspection of wiring during technical checks

M3 screws handle small brackets and sensor attachments; M5 screws with M5/M16 bearings handle the steering pivot and rear axle, ensuring low friction under load.

### Drive Motor Selection

The **50:1 Micro Metal Gearmotor HPCB 6V** (0.74 kg·cm stall torque, ~650 rpm no-load) was selected after evaluating two options:

| Option | Ratio | Speed | Torque | Result |
|---|---|---|---|---|
| **Chosen: HPCB 6V 50:1** | 50:1 | ~650 rpm | 0.74 kg·cm |  Sufficient torque, controllable speed for PID |
| Discarded: high-speed motor | 10:1 | ~3000 rpm | ~0.15 kg·cm |  Too fast — PID could not correct lateral drift in time |

The 50:1 ratio provides enough torque to accelerate smoothly while keeping speed low enough for the PID controller to react before lateral drift exceeds recoverable limits.

### Iteration History

| Version | Change | Reason |
|---|---|---|
| V1 (2025) | Standard steering, single lateral sensor | Baseline from 2025 national competition |
| V2 | Bilateral PID using left + right sensors | V1 drifted toward the wider wall; bilateral error fixed this |
| V3 (current) | Ackermann steering geometry, TPU tires | Standard steering caused ~8° corner angle error per turn due to tire slip |

---

## 2. Power and Sensor Architecture

### Power Budget

Two independent power rails isolate motor switching noise from logic and sensor signals:

**Rail 1 — Logic (5V, 3A power bank):**

| Component | Current |
|---|---|
| Arduino UNO Q | ~200 mA |
| WonderCam V2.0 | ~300 mA |
| 3× URM37 sensors | ~60 mA total |
| TD-8125 servo (peak) | ~1,500 mA |
| TB6612FNG logic stage | <10 mA |
| **Total peak** | **~2,070 mA** |

Servo peaks are short and do not coincide with Arduino or camera peaks. Measured current during full operation stays consistently below 2.5 A.

**Rail 2 — Traction (7.4V, 400 mAh LiPo):**

| Component | Current |
|---|---|
| 50:1 gearmotor (peak) | ~1,500 mA |
| TB6612FNG power stage | ~100 mA |
| **Total peak** | **~1,600 mA** |

Estimated autonomy: 0.4 Ah ÷ 1.6 A = **~15 minutes** — more than sufficient for 3-minute runs.

> **Why two rails?** During V1 testing, motor acceleration caused voltage dips on the shared supply that reset the Arduino mid-run. Splitting into two independent rails eliminated this failure mode entirely.

### Sensor Selection and Placement

**Why URM37 V5.0 over HC-SR04?**
The URM37 includes built-in temperature compensation for consistent accuracy in variable-temperature venues. Its trigger protocol (100 µs LOW pulse, echo read on LOW) avoids the double-pulse timing of HC-SR04, simplifying the driver code and reducing timing errors.

| Sensor | Position | Height | Justification |
|---|---|---|---|
| Front | Center-front of chassis | 12 cm | Avoids reading the floor; detects 100 mm-high walls reliably from 55 cm |
| Left | Above front-left wheel | 10 cm | Reads lateral wall distance for PID; positioned forward to detect walls before the chassis passes them |
| Right | Above front-right wheel | 10 cm | Symmetric to left for balanced bilateral PID |

The WonderCam is mounted facing down at ~30°, positioned to see orange/blue turn lines on the track floor when the vehicle is 40–60 cm from the corner entrance.

---

## 3. Software Architecture and Control Strategy

### State Machine

The program runs as a **finite state machine (FSM)** with six states. Every `loop()` iteration reads all three sensors sequentially, then executes the current state's logic.

```
IDLE ──(button press)──► RUNNING ──(front < 55 cm)──► TURNING
                             ▲                              │
                             └───── STRAIGHTENING ◄─────────┘
                                          │
                              (corners == 12, 3 laps done)
                                          │
                                      APPROACH
                                          │
                              (sensors within tolerance)
                                          │
                                        DONE
```

| State | Behavior |
|---|---|
| `IDLE` | Motor off, waiting for button. Takes 3-sample averaged baseline from all sensors. |
| `RUNNING` | Drives at cruise speed. PID keeps car centered. Camera polls for color. Checks front sensor every cycle. |
| `TURNING` | Servo locked at max angle for `T_GIRO_MS` ms at reduced speed. |
| `STRAIGHTENING` | Servo returns to center for `T_ENDEREZAR_MS` ms. Corner counter increments. Integral resets. |
| `APPROACH` | After 12 corners, slows and compares live readings to stored start values. |
| `DONE` | Motor stops, servo centers, LED blinks. |

### Algorithm 1 — URM37 Sequential Reading

All three sensors are fired **one at a time** with 5 ms separation to prevent cross-echo interference — a failure discovered in V1 testing where simultaneous triggering caused random zero readings.

```cpp
float leerURM37(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(100);         // URM37 V5.0: 100µs LOW to trigger
  unsigned long duration = pulseIn(echoPin, LOW, 50000);
  digitalWrite(trigPin, HIGH);    // return to idle (HIGH)
  if (duration == 0 || duration >= 50000) return -1.0;
  return (float)duration / 50.0;  // 50µs = 1 cm (URM37-specific formula)
}
```

The 50,000 µs timeout caps range at 100 cm and prevents `pulseIn` from blocking when no wall is present.

### Algorithm 2 — Bilateral PID Lane Centering

The error is the **difference between left and right lateral distances**, implicitly targeting 0 (centered) regardless of corridor width — essential when the Open Challenge alternates between 600 mm and 1000 mm corridors determined randomly before each round.

```
error = dist_left − dist_right

servo_angle = SERVO_CENTER + constrain(P + I + D, −41°, +57°)
```

The integral uses **anti-windup clamping** (±80 units) and **resets after every turn** to prevent previous straight-section error from affecting the next.

**Tuning iterations:**

| Kp | Ki | Kd | Observed behavior |
|---|---|---|---|
| 0.8 | 0.04 | 0.10 | Loose — 3 full sections to correct a 5 cm lateral offset |
| **1.2** | **0.04** | **0.15** | ** Current — corrects within 1 section, no oscillation** |
| 1.8 | 0.04 | 0.15 | Overshoot — side-to-side oscillation in long straights |

### Algorithm 3 — Turning Direction via WonderCam

The WonderCam is pre-loaded via its internal flash with two color profiles:
- **Color ID 1 = Orange** → turn **RIGHT**
- **Color ID 2 = Blue** → turn **LEFT**

The direction is **latched on the first detection** and reused for all 11 remaining corners to prevent re-detection errors. Camera polling is restricted to the `RUNNING` state only.

**Fallback:** If no color is detected before the first wall, the vehicle reads both lateral sensors and turns toward the side with more free space (the wider gap indicates the inside of the turn).

### Algorithm 4 — Start Position Memory and APPROACH State

At startup, with the vehicle stationary, 3 averaged readings per sensor are stored as the reference:

```
pos_frontal_ini, pos_izq_ini, pos_der_ini
```

After the 12th corner, `APPROACH` state reduces speed to 480 PWM and compares live readings against stored values each cycle. Tolerances are differentiated by sensor reliability:

| Sensor | Tolerance | Reasoning |
|---|---|---|
| Left + Right (primary) | ±6 cm | Always faces a wall — reading is stable and repeatable across starting zones |
| Front (secondary) | ±12 cm | May face a long corridor in lateral starting zones; wider tolerance prevents false positives |
| Front initial > 120 cm | Discarded | If initial reading was large, sensor was looking down a corridor — that value will not repeat reliably |

### Edge Cases

| Situation | Behavior |
|---|---|
| Both lateral sensors return -1 | PID skips correction cycle; car continues straight |
| Front sensor returns -1 | Wall check skipped; no spurious turn triggered |
| No color before first wall | Fallback: turn toward the side with more lateral space |
| Color detected again after first lock | Ignored; direction stays latched |

---

## 4. Systems Thinking and Engineering Decisions

### Subsystem Interaction

```
[Power Bank 5V] ──► [Arduino UNO Q] ──► [TB6612FNG] ◄── [LiPo 7.4V]
                          │                   │
               ┌──────────┼──────────┐   [DC Motor 50:1]
               │          │          │
         [WonderCam]  [3× URM37] [Servo TD-8125]
               │          │          │
          [Vision]    [PID +     [Ackermann
          [Module]  Wall detect]  Steering]
```

All sensor data enters the FSM every loop cycle. The FSM decides servo angle and motor speed based on current state and sensor values.

### Trade-offs

**Time-based vs. gyroscope-based turns:**
A gyroscope would give exact 90° turns. We chose time-based (`T_GIRO_MS = 900 ms`) because adding an external IMU increases wiring complexity and failure points. The time-based approach is reliable when battery voltage is consistent — guaranteed by the dedicated LiPo traction rail.

**Bilateral vs. single-sensor PID:**
A single lateral sensor requires a fixed setpoint that depends on corridor width. Since corridor width varies randomly between rounds (600–1000 mm), a fixed setpoint produces wall-hugging in narrow corridors and drift in wide ones. The bilateral approach adapts automatically to any width.

**Separate vs. shared power rails:**
During V1 testing, motor acceleration caused voltage dips on the shared supply that reset the Arduino mid-run. Two independent rails eliminated this failure mode.

**Continuous vs. restricted camera polling:**
Initially the camera polled throughout the entire run. This caused false detections from adjacent section floor lines visible from mid-straight. Restricting polling to `RUNNING` state and requiring front sensor below 70 cm before accepting a detection eliminated false triggers.

### Risk Assessment

| Risk | Mitigation |
|---|---|
| Camera misses color before first wall | Lateral fallback: turn toward the more open side |
| Sensor returns -1 at critical moment | PID skips cycle safely; no crash triggered |
| APPROACH never matches | Lateral ±6 cm tolerance covers zone variation; front discarded if > 120 cm |
| Servo center drift | `SERVO_CENTRO = 81` verified on actual track; recalibrate if vehicle is modified |

---

## 5. Configurable Parameters

| Constant | Default | Description |
|---|---|---|
| `SERVO_CENTRO` | 81 | Servo angle for straight-ahead driving |
| `SERVO_MAX_IZQ` | 40 | Maximum left steering angle |
| `SERVO_MAX_DER` | 138 | Maximum right steering angle |
| `VEL_CRUCERO` | 680 | Cruise speed (0–1023, 10-bit PWM) |
| `VEL_GIRO` | 550 | Speed during a corner |
| `VEL_APROXIMACION` | 480 | Speed during final approach |
| `DIST_PARED_GIRO` | 55 cm | Front distance that triggers a turn |
| `DIST_LATERAL_MIN` | 10 cm | Emergency lateral safety threshold |
| `T_GIRO_MS` | 900 ms | Duration of full-lock servo during a turn |
| `T_ENDEREZAR_MS` | 300 ms | Duration of straightening phase after a turn |
| `Kp` | 1.2 | PID proportional gain |
| `Ki` | 0.04 | PID integral gain |
| `Kd` | 0.15 | PID derivative gain |
| `TOLERANCIA_LATERAL_CM` | 6.0 cm | Stop tolerance for lateral sensors |
| `TOLERANCIA_FRONTAL_CM` | 12.0 cm | Stop tolerance for front sensor |
| `FRONTAL_DESCARTE_CM` | 120 cm | Threshold above which front sensor is ignored for stopping |

---



