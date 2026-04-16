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


