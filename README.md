Engineering materials
====

This repository contains engineering materials of a self-driven vehicle's model participating in the WRO Future Engineers competition in the season 2022.

## Content

* `t-photos` contains 2 photos of the team (an official one and one funny photo with all team members)
* `v-photos` contains 6 photos of the vehicle (from every side, from top and bottom)
* `video` contains the video.md file with the link to a video where driving demonstration exists
* `schemes` contains one or several schematic diagrams in form of JPEG, PNG or PDF of the electromechanical components illustrating all the elements (electronic components and motors) used in the vehicle and how they connect to each other.
* `src` contains code of control software for all components which were programmed to participate in the competition
* `models` is for the files for models used by 3D printers, laser cutting machines and CNC machines to produce the vehicle elements. If there is nothing to add to this location, the directory can be removed.
* `other` is for other files which can be used to understand how to prepare the vehicle for the competition. It may include documentation how to connect to a SBC/SBM and upload files there, datasets, hardware specifications, communication protocols descriptions etc. If there is nothing to add to this location, the directory can be removed.

## Introduction

_This part must be filled by participants with the technical clarifications about the code: which modules the code consists of, how they are related to the electromechanical components of the vehicle, and what is the process to build/compile/upload the code to the vehicle’s controllers._

## Components used

* "Hiwonder WonderCam V2.0": Es un módulo de visión artificial basado en el SoC Kendryte K210, capaz de ejecutar inferencia en el borde sin conectividad externa. Integra un sensor CMOS de 2 MP, pantalla LCD, iluminación auxiliar e interfaz I2C, y soporta funciones como detección de color, reconocimiento facial, clasificación de objetos y lectura de códigos QR, además de entrenamiento simplificado de modelos. Está orientada a aplicaciones de robótica y sistemas embebidos de baja potencia con integración directa en plataformas como Arduino y micro:bit con un rango de operación de 5V y un consumo de corriente de trabajo de 300mA.
* "DF ROBOT URM37 V5.0": Es un módulo de medición de distancia basado en ultrasonido que integra compensación térmica para mejorar la precisión en entornos variables, con un rango extendido de 2 a 800 cm y resolución de 1 cm. Opera en un rango de 3.3–5.5 V con bajo consumo (<20 mA) y ofrece múltiples interfaces de salida: analógica, PWM, switch y comunicación serial TTL/RS232..
* "Arduino UNO Q": Es una SBC centrada en el MCU STM32U585 (Arm Cortex-M33 a 160 MHz), encargado del control en tiempo real, adquisición de datos y gestión de periféricos con soporte para bajo consumo y capacidades de seguridad (TrustZone). Este se complementa con un MPU Qualcomm Dragonwing QRB2210 para tareas de alto nivel, permitiendo la coexistencia de procesamiento determinista y ejecución de aplicaciones complejas. La plataforma integra 2 GB de RAM LPDDR4, 16 GB eMMC, conectividad Wi-Fi/Bluetooth y múltiples interfaces (UART, I²C, SPI, CAN, USB).
* "Servomotor Digital TD-8125": Es un actuador de posición de alto par basado en control PWM, que integra un motor DC, tren de engranajes metálicos y lazo de realimentación interno para posicionamiento preciso. Opera típicamente en el rango de 3.7–7.2 V, con rotación de ~180° y torque nominal cercano a 20–22 kg·cm.
* "50:1 Micro Metal Gearmotor HPCB 6V": Es un motor DC con escobillas integrado con una caja reductora metálica de relación ≈51:1, diseñado para aplicaciones de robótica compacta y actuadores de precisión. Opera a 6 V con velocidad sin carga de ~650 rpm y consumo típico de 100 mA, alcanzando corrientes de hasta ~1.5 A y un par de bloqueo cercano a 0.74 kg·cm.
* "TB6612FNG Dual Motor Driver Carrier": Es un driver dual para motores DC basado en el circuito integrado TB6612FNG, diseñado para el control bidireccional de hasta dos motores de escobillas mediante señales PWM de hasta ~100 kHz. Opera en un rango de 4.5–13.5 V, con una corriente continua de hasta 1 A por canal (picos mayores), e incorpora lógica de control independiente para cada motor, permitiendo regulación de velocidad y sentido de giro.
* Cables, Jumpers, Alambre
* Tornillería M3 y M5 y baleros M5 por dentro y M16 por fuera.
* PLA, TPU, acrílico.
  
