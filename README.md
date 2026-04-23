Engineering materials
====

This repository contains engineering materials of a self-driven vehicle's model participating in the WRO Future Engineers competition in the season 2026.

## Content

* `t-photos` contains 2 photos of the team (an official one and one funny photo with all team members)
* `v-photos` contains 6 photos of the vehicle (from every side, from top and bottom)
* `video` contains the video.md file with the link to a video where driving demonstration exists
* `schemes` contains one or several schematic diagrams in form of JPEG, PNG or PDF of the electromechanical components illustrating all the elements (electronic components and motors) used in the vehicle and how they connect to each other.
* `src` contains code of control software for all components which were programmed to participate in the competition
* `models` is for the files for models used by 3D printers, laser cutting machines and CNC machines to produce the vehicle elements. If there is nothing to add to this location, the directory can be removed.


## Introduction

_This part must be filled by participants with the technical clarifications about the code: which modules the code consists of, how they are related to the electromechanical components of the vehicle, and what is the process to build/compile/upload the code to the vehicle’s controllers._

## Components used

* "Hiwonder WonderCam V2.0": A machine vision module based on the Kendryte K210 SoC, capable of running edge inference without external connectivity. It integrates a 2 MP CMOS sensor, LCD display, auxiliary lighting, and an I2C interface. It supports functions such as color detection, facial recognition, object classification, and QR code reading, as well as simplified model training. It is intended for robotics and low-power embedded systems, with direct integration into platforms such as Arduino and micro:bit, operating at 5V with a working current of 300 mA.
  
* "DF ROBOT URM37 V5.0": An ultrasonic distance measurement module with built-in temperature compensation to improve accuracy in variable environments. It has an extended range of 2 to 800 cm and 1 cm resolution. It operates between 3.3–5.5 V with low power consumption (<20 mA) and provides multiple output interfaces: analog, PWM, switch, and TTL/RS232 serial communication.

* "Arduino UNO Q": A single-board computer centered on the STM32U585 MCU (Arm Cortex-M33 at 160 MHz), responsible for real-time control, data acquisition, and peripheral management, with low-power support. It is complemented by a Qualcomm Dragonwing QRB2210 MPU for high-level tasks, enabling coexistence of deterministic processing and complex application execution. The platform includes 2 GB LPDDR4 RAM, 16 GB eMMC storage, Wi-Fi/Bluetooth connectivity, and multiple interfaces (UART, I²C, SPI, CAN, USB).

* "Digital Servomotor TD-8125": A high-torque position actuator based on PWM control, integrating a DC motor, metal gear train, and internal feedback loop for precise positioning. It typically operates between 3.7–7.2 V, with ~180° rotation and a nominal torque of approximately 20–22 kg·cm.

* "50:1 Micro Metal Gearmotor HPCB 6V": A brushed DC motor integrated with a metal gearbox with a ratio of approximately 51:1, designed for compact robotics and precision actuators. It operates at 6 V with a no-load speed of ~650 rpm and a typical current of 100 mA, reaching up to ~1.5 A and a stall torque of approximately 0.74 kg·cm.

* "TB6612FNG Dual Motor Driver Carrier": A dual DC motor driver based on the TB6612FNG integrated circuit, designed for bidirectional control of up to two brushed motors using PWM signals up to ~100 kHz. It operates between 4.5–13.5 V, with a continuous current of up to 1 A per channel (higher peaks), and includes independent control logic for each motor, enabling speed and direction regulation.

* Wires, jumpers, and cable

* M3 and M5 screws, and bearings (M5 inner, M16 outer)

* PLA, TPU, acrylic

* Power supply: 5V 3A power bank for the Arduino, and a 7.4V 400 mAh LiPo battery.

## Mobility and Mechanical Design

* Torque and Speed:
For the autonomous vehicle, the 50:1 DC motor (0.74 kg·cm stall torque, 650 rpm) was selected due to its optimal balance between maneuverability and obstacle-handling capability. It was chosen for steering precision and responsiveness, prioritizing control over energy consumption.
Design-related selections:

* Design-related selections:
 Rigidity vs. weight: PLA is used for structural strength, TPU for impact absorption and flexibility in tires, and acrylic for a high-quality base. M3/M5 screws and M5/M16 bearings ensure robustness without excessive weight.

* Why these components?
-WonderCam V2.0: Edge inference enables real-time autonomous decisions, complying with WRO rules without external connectivity.
-URM37 V5.0: 2–800 cm range, 1 cm resolution, and thermal compensation, ideal for wall/obstacle detection in changing environments.
-Arduino UNO Q: Combines low power consumption (STM32U585) with high application capability for robotics.
-TB6612FNG: Lower voltage drop than L298N, efficient and compatible with 5V logic, supports 1 A continuous per channel (up to 3.2 A peak), sufficient for the motor.
-Materials (PLA, TPU, acrylic): Lightweight, durable, and allow rapid iteration; acrylic enables visual inspection during technical checks.

* Design explanation:
The design is inspired by the robot used in the 2025 national competition. While many components were changed, the general structure was maintained with significant improvements, particularly in the steering system. An Ackermann steering system was implemented to ensure that the front wheels follow turning arcs with different radii, preventing lateral slipping and reducing friction in curves. This improves trajectory accuracy and tire wear, essential in a closed circuit with tight turns.

## Power and Sensor Architecture

Two independent power sources are used to isolate the noisy traction motor from the sensitive logic components (Arduino, sensors, camera, and servo motor).

The logic is powered by a 5V, 3A power bank. This directly supplies the Arduino UNO Q, which distributes power to the WonderCam V2.0 (300 mA), three URM37 ultrasonic sensors (60 mA total), the TD-8125 servo motor (1.5 A peak), and the TB6612FNG logic stage (less than 10 mA). The theoretical maximum consumption is approximately 3.07 A, slightly above the available 3A; however, in practice, servo and Arduino peaks do not occur simultaneously.

For traction, a 7.4V 400 mAh LiPo battery is used exclusively for the DC motor. The 50:1 motor consumes up to 1.5 A peak, and the TB6612FNG adds about 0.1 A, totaling 1.6 A peak. Estimated autonomy is 15 minutes (0.4 Ah / 1.6 A), more than sufficient for runs under 3 minutes.

Three URM37 ultrasonic sensors are installed: one front-facing sensor to detect walls or obstacles, and two lateral sensors (left and right) to measure distance to side walls. These lateral values feed a PID controller that adjusts the Ackermann steering system to keep the vehicle centered.

## Pin Mapping

```
DC Motor Driver
  PWMA  → Pin 5   (10-bit PWM speed)
  AIN1  → Pin 3   (direction control)
  AIN2  → Pin 2   (direction control)

Steering Servo
  Signal → Pin 9

URM37 FRONT sensor   (wall detection for turns)
  TRIG  → Pin 8
  ECHO  → Pin 14 (A0)

URM37 LEFT sensor    (PID lane centering)
  TRIG  → Pin 10
  ECHO  → Pin 15 (A1)

URM37 RIGHT sensor   (PID lane centering)
  TRIG  → Pin 12
  ECHO  → Pin 16 (A2)

WonderCam
  I2C   → SDA / SCL (standard)

Start button
  Signal → Pin 7  (INPUT_PULLUP, active LOW)
```

---

## Software Architecture (Open Challenge)

The program is structured as a **finite state machine (FSM)** with six states. Every iteration of `loop()` reads the three sensors sequentially, then executes the logic corresponding to the current state.

### States

```
IDLE ──(button press)──► RUNNING ──(wall < 55 cm)──► TURNING
                             ▲                            │
                             └──(straighten time)── STRAIGHTENING
                                                         │
                                    (after 12 corners) ──►  APPROACH
                                                              │
                                         (sensors match start)──► DONE
```

| State | Description |
|---|---|
| `IDLE` | Motor stopped, waiting for start button. Sensors read and store starting position (3-sample average). |
| `RUNNING` | Vehicle drives forward at cruise speed. PID controller keeps it centered. Camera polls for color. |
| `TURNING` | Servo locked to max angle for `T_GIRO_MS` ms at reduced speed. Direction set by camera or lateral fallback. |
| `STRAIGHTENING` | Servo returns to center for `T_ENDEREZAR_MS` ms before resuming `RUNNING`. Corner counter increments. |
| `APPROACH` | After 12 corners (3 laps), vehicle slows down and compares live sensor readings to stored start values. |
| `DONE` | Motor stops, servo centers. LED blinks continuously. |

---

## Key Algorithms

### 1. URM37 V5.0 Sequential Reading

The three sensors are fired **one at a time** with a 5 ms gap between them to avoid cross-echo interference. Each reading follows the URM37 V5.0 protocol:

```cpp
float leerURM37(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(100);       // URM37 V5.0 needs 100µs LOW to trigger
  unsigned long duration = pulseIn(echoPin, LOW, 50000);
  digitalWrite(trigPin, HIGH);  // return to idle state (HIGH)
  if (duration == 0 || duration >= 50000) return -1.0;
  return (float)duration / 50.0; // URM37 formula: 50µs = 1 cm
}
```

**Why sequential?** Simultaneous triggering causes the echo pulses to cross between sensors, producing incorrect or zero readings. Sequential firing eliminates this interference entirely.

### 2. Bilateral PID Lane Centering

The error is computed as the **difference between left and right lateral distances**. When the car is perfectly centered, `error = 0` regardless of corridor width (the track can be 1000 mm or 600 mm wide in the Open Challenge).

```
error = dist_left − dist_right
```

- `error > 0` → car is closer to the left wall → steer right
- `error < 0` → car is closer to the right wall → steer left
- `error = 0` → car is centered → drive straight

The PID output is added to the servo center angle:

```
servo_angle = SERVO_CENTER + constrain(P + I + D, -MAX_LEFT_OFFSET, +MAX_RIGHT_OFFSET)
```

The integral term uses **anti-windup clamping** (±80 units) and is reset to zero after each turn to prevent accumulated error from previous straight sections affecting the next one.

**Tunable parameters:**

| Parameter | Default | Effect |
|---|---|---|
| `Kp` | 1.2 | Main proportional gain. Increase if centering feels loose. |
| `Ki` | 0.04 | Corrects persistent drift. Decrease if oscillation appears. |
| `Kd` | 0.15 | Damps rapid changes. Increase if the car over-corrects. |

### 3. Turning Direction via WonderCam

The WonderCam is pre-programmed (via its internal flash) to detect two color IDs:
- **Color ID 1 = Orange** → turn **RIGHT**
- **Color ID 2 = Blue** → turn **LEFT**

The direction is latched on the **first detection** and reused for all subsequent 11 corners. Camera polling is active only during the `RUNNING` state to avoid false detections mid-turn.

**Fallback (if no color is detected before the first wall):** The car reads both lateral sensors and turns toward the side with more free space.

### 4. Starting Position Memory & APPROACH State

When the start button is pressed (vehicle stationary), the program takes **3 averaged readings** from each sensor and stores them as the reference starting position:

```
pos_frontal_ini, pos_izq_ini, pos_der_ini
```

After completing the 12th corner, the vehicle enters `APPROACH` mode: it reduces speed to `VEL_APROXIMACION = 480` and continuously compares live readings against the stored values. When all valid sensors are within tolerance simultaneously, it enters `DONE` and stops the motor.

Tolerances are differentiated by sensor reliability:
- **Lateral sensors** (main criterion): ±6 cm
- **Front sensor** (secondary): ±12 cm — discarded if the initial reading exceeded 120 cm (the sensor was looking down a long corridor, not a wall)

---

## Configurable Parameters

All tunable constants are declared at the top of the sketch for easy access during track testing:

| Constant | Default | Description |
|---|---|---|
| `SERVO_CENTRO` | 81 | Servo angle for straight-ahead driving |
| `SERVO_MAX_IZQ` | 40 | Maximum left steering angle |
| `SERVO_MAX_DER` | 138 | Maximum right steering angle |
| `VEL_CRUCERO` | 680 | Straight-line speed (0–1023, 10-bit PWM) |
| `VEL_GIRO` | 550 | Speed during a corner |
| `VEL_APROXIMACION` | 480 | Speed during final approach to start |
| `DIST_PARED_GIRO` | 55 cm | Front distance that triggers a turn |
| `DIST_LATERAL_MIN` | 10 cm | Emergency wall safety threshold |
| `T_GIRO_MS` | 900 ms | Time the servo stays at full lock during a turn |
| `T_ENDEREZAR_MS` | 300 ms | Time to straighten after a turn |
| `TOLERANCIA_LATERAL_CM` | 6.0 cm | Stop tolerance for lateral sensors |
| `TOLERANCIA_FRONTAL_CM` | 12.0 cm | Stop tolerance for front sensor |
| `FRONTAL_DESCARTE_CM` | 120 cm | Threshold above which front sensor is ignored for stopping |

---

## How to Compile and Upload

1. Install **Arduino IDE 2.x**.
2. Install the **Arduino UNO Q** board package (`Arduino_RouterBridge` library).
3. Install the **WonderCam** library from its manufacturer or include the `.h` file in the sketch folder.
4. Install the **Servo** library (included with Arduino IDE by default).
5. Open `WRO_FutureEngineers_Electrochalanes.ino`.
6. Select board: **Arduino UNO Q**.
7. Select the correct COM port.
8. Click **Upload**.

**Debug mode:** Uncomment `#define DEBUG` at the top of the sketch to enable Serial Monitor output (9600 baud) showing sensor readings and PID values in real time. Comment it out for competition to save CPU cycles.

---

## Engineering Decisions & Trade-offs

**Why URM37 V5.0 instead of HC-SR04?**
The URM37 uses a different trigger protocol (100 µs LOW pulse, echo measured on LOW) compared to the standard HC-SR04. It provides more stable readings at short distances and does not require a separate HIGH trigger pulse, simplifying the timing logic.

**Why a bilateral PID instead of a single-sensor PID?**
Using only one lateral sensor makes the setpoint depend on the track width, which varies between rounds (600–1000 mm). A bilateral error (`left − right`) implicitly sets the setpoint to 0 regardless of corridor width, making the controller robust to track variations.

**Why a time-based turn instead of angle-based?**
The Arduino UNO Q does not have a gyroscope or encoder in our build. A fixed-duration turn (`T_GIRO_MS`) was chosen as the simplest reliable solution. The value is calibrated on the actual track to produce a consistent 90° turn.

**Why differentiated tolerances for the stopping condition?**
The front sensor's reading at the start position depends on whether the car is facing a wall (short distance, reliable) or looking down a long corridor (large distance, changes with inner wall configuration between rounds). Using a fixed tolerance for both cases would produce false stops or missed stops depending on the starting zone assigned by the judge.

---



