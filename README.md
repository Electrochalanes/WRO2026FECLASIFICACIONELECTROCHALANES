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

* "Hiwonder WonderCam V2.0": Es un módulo de visión artificial basado en el SoC Kendryte K210, capaz de ejecutar inferencia en el borde sin conectividad externa. Integra un sensor CMOS de 2 MP, pantalla LCD, iluminación auxiliar e interfaz I2C, y soporta funciones como detección de color, reconocimiento facial, clasificación de objetos y lectura de códigos QR, además de entrenamiento simplificado de modelos. Está orientada a aplicaciones de robótica y sistemas embebidos de baja potencia con integración directa en plataformas como Arduino y micro:bit con un rango de operación de 5V y un consumo de corriente de trabajo de 300mA.
  
* "DF ROBOT URM37 V5.0": Es un módulo de medición de distancia basado en ultrasonido que integra compensación térmica para mejorar la precisión en entornos variables, con un rango extendido de 2 a 800 cm y resolución de 1 cm. Opera en un rango de 3.3–5.5 V con bajo consumo (<20 mA) y ofrece múltiples interfaces de salida: analógica, PWM, switch y comunicación serial TTL/RS232..

* "Arduino UNO Q": Es una SBC centrada en el MCU STM32U585 (Arm Cortex-M33 a 160 MHz), encargado del control en tiempo real, adquisición de datos y gestión de periféricos con soporte para bajo consumo y capacidades de seguridad (TrustZone). Este se complementa con un MPU Qualcomm Dragonwing QRB2210 para tareas de alto nivel, permitiendo la coexistencia de procesamiento determinista y ejecución de aplicaciones complejas. La plataforma integra 2 GB de RAM LPDDR4, 16 GB eMMC, conectividad Wi-Fi/Bluetooth y múltiples interfaces (UART, I²C, SPI, CAN, USB).

* "Servomotor Digital TD-8125": Es un actuador de posición de alto par basado en control PWM, que integra un motor DC, tren de engranajes metálicos y lazo de realimentación interno para posicionamiento preciso. Opera típicamente en el rango de 3.7–7.2 V, con rotación de ~180° y torque nominal cercano a 20–22 kg·cm.

* "50:1 Micro Metal Gearmotor HPCB 6V": Es un motor DC con escobillas integrado con una caja reductora metálica de relación ≈51:1, diseñado para aplicaciones de robótica compacta y actuadores de precisión. Opera a 6 V con velocidad sin carga de ~650 rpm y consumo típico de 100 mA, alcanzando corrientes de hasta ~1.5 A y un par de bloqueo cercano a 0.74 kg·cm.

* "TB6612FNG Dual Motor Driver Carrier": Es un driver dual para motores DC basado en el circuito integrado TB6612FNG, diseñado para el control bidireccional de hasta dos motores de escobillas mediante señales PWM de hasta ~100 kHz. Opera en un rango de 4.5–13.5 V, con una corriente continua de hasta 1 A por canal (picos mayores), e incorpora lógica de control independiente para cada motor, permitiendo regulación de velocidad y sentido de giro.

* Cables, Jumpers, Alambre

* Tornillería M3 y M5 y baleros M5 por dentro y M16 por fuera.

* PLA, TPU, acrílico.

* Alimentación: Powerbank de 5v y 3A para el Arduino, bater´´ia LiPO de 7.4V y 400mAh

## Criterio 1: Movilidad y Diseño Mecánico.

* Torque y Velocidad: Torque y velocidad:
Para el vehículo autónomo, se seleccionó el motor DC 50:1 (torque de bloqueo 0.74 kg·cm, 650 rpm) por su equilibrio óptimo entre maniobrabilidad y capacidad de superar obstáculos se eligió para precisión en dirección o guiado, priorizando respuesta sobre consumo energético.

* Selecciones relacionadas al diseño: Torque y velocidad:
Rigidez vs. peso: Uso de PLA para piezas resistentes, TPU para absorción de impactos y maleabilidad para las llantas y acrílico para una base de calidad. Tornillería M3/M5 y baleros M5/M16 garantizan robustez sin exceso de peso.

* ¿Por qué estos componentes?:
  WonderCam V2.0: Inferencia en el borde para decisiones autónomas en tiempo real, cumpliendo reglas WRO sin conexión externa.
  URM37 V5.0: Rango 2–800 cm, resolución 1 cm y compensación térmica, ideal para detección de paredes/obstáculos en entornos cambiantes.
  Arduino UNO Q: Combina bajo consumo (STM32U585) y potencia de aplicación para la robótica.
  TB6612FNG: Menor caída de tensión que L298N, eficiente y compatible con lógica 5V, maneja 1 A continuo por canal (picos 3.2 A) suficiente para el motor.
  Materiales (PLA, TPU, acrílico): Ligereza, resistencia e iteración rápida; el acrílico permite inspección visual en verificaciones técnicas.

* Explicación del diseño: El diseño está inspirado en el robot que utilizamos para el nacional del 2025, cambiamos muchos componentes, pero específicamente hablando del diseño, se mantuvo una estructura similar pero con cambios importantes en partes como el sistema direccional, se implementó un sistema de dirección Ackerman para garantizar que las ruedas delanteras describan arcos de giro con diferentes radios, evitando el deslizamiento lateral y reduciendo la fricción en curvas. Esto mejora la precisión de trayectoria y el desgaste de neumáticos, esencial en un circuito cerrado con curvas cerradas. Las llantas fueron diseñadas en 3D impresas en PLA para la estructura principal y TPU para las gomas, esto fue seleccionado así con el fin de obtener llantas que se adaptaran al diseño que teníamos en mente, permitiéndonos utilizar diferentes arreglos en la parte trasera y delantera del vehículo y fabricar llanta más grande para evitar roses con el suelo de la pista. Decidimos hace un robot compacto con el fin de obtener una mayor maniobrabilidad al momento de realizar maniobras de giro o esquivamiento, la cámara se colocó en una posición alta para permitirle tener una vista más centrada a la pista y a los obstáculos en general, eliminando así interferencias ambientales del entorno externo a la pista. Las capas y pisos fueron diseñado de forma que acceder a los componentes cruciales como el motor y el cerebro del automóvil fueran de fácil acceso o de fácil desarmado y armado para eliminar tiempo crucial en posibles ajustes o reparaciones. El sistema de tracción trasera con tornillo sin fin y reducción en dos etapas maximiza el torque disponible, asegura el frenado automático y simplifica el control, todo ello integrado con una dirección Ackerman delantera que no interfiere con el empuje.

## Criterio 2: Arquitectura de potencia y sensores: 

Se utilizan dos fuentes independientes para aislar la parte ruidosa del motor de tracción de la lógica sensible del Arduino, sensores, cámara y servomotor. La alimentación de la lógica proviene de una powerbank de 5V con capacidad de 3A. Esta powerbank alimenta directamente el Arduino UNO Q, que a su vez distribuye la corriente a la WonderCam V2.0 (300 mA), a los tres sensores ultrasónicos URM37 (60 mA en total), al servomotor TD-8125 (1.5 A en pico) y a la etapa lógica del driver TB6612FNG (menos de 10 mA). Sumando los consumos máximos teóricos se obtienen aproximadamente 3.07 A, una cifra ligeramente superior a los 3A disponibles. Sin embargo, en la práctica los picos del servomotor y del Arduino no ocurren simultáneamente.

Para la tracción se emplea una batería LiPo de 7.4V y 400 mAh exclusiva para el motor DC de tracción. El motor único 50:1 consume hasta 1.5 A en pico, y las pérdidas en la etapa de potencia del TB6612FNG suman unos 0.1 A, totalizando 1.6 A pico. La autonomía estimada es de 15 minutos (0.4 Ah / 1.6 A), más que suficiente para rondas de menos de 3 minutos. 

Se colocan tres sensores ultrasónicos URM37: uno frontal central para detectar muros u obstáculos delante del vehículo, y dos laterales (izquierdo y derecho) para medir la distancia a los muros laterales. Estos valores laterales alimentan un control PID que actúa sobre el sistema de dirección Ackerman, ajustando el ángulo de las ruedas delanteras para mantener el vehículo centrado en el carril. Los sensores laterales se montan a 10 cm del suelo, operando en su rango óptimo de 10 a 50 cm, adecuado para pasillos de 60-80 cm de ancho. La cámara WonderCam V2.0 se ubica en la parte superior del vehículo, orientada en diagonal hacia abajo unos 30 grados respecto a la horizontal. Esta posición permite ver el suelo delante del robot para detectar colores de obstáculos (rojo, verde) y evita sombras proyectadas por la propia estructura. La vista diagonal, combinada con el ultrasonido frontal, cubre tanto el corto alcance (visión) como el largo alcance (sonar).

Los métodos de calibración incluyen, para los sensores ultrasónicos, la compensación térmica de fábrica del URM37 y una calibración en campo midiendo distancias reales de 10, 50 y 100 cm para ajustar un factor lineal por sensor. Para la cámara se utiliza el balance de blancos automático del K210 y se calibran los colores capturando muestras de obstáculos oficiales bajo la iluminación del campo para generar umbrales HSV almacenados en flash.
Se han considerado problemas de ruido e interferencias. El ruido eléctrico generado por el motor DC y el servomotor se mitiga con alimentaciones separadas (batería 7.4V solo para el motor, powerbank 5V para la lógica). Para la cámara, se activa el LED auxiliar de la WonderCam en condiciones de poca luz.

