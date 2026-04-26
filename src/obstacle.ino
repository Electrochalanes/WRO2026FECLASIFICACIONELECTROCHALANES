/*
 * ============================================================
 * WRO Future Engineers 2026 — Obstacle Challenge
 * Team: Electrochalanes | Tijuana, Baja California, Mexico
 * ============================================================
 *
 * DESCRIPTION:
 *   Autonomous vehicle controller for the WRO 2026 Obstacle Challenge.
 *   The robot navigates a closed track completing exactly 3 laps (12 corners).
 *   It detects red and green obstacle blocks via a WonderCam color sensor
 *   and executes evasive maneuvers accordingly:
 *     - RED block  → evade to the right, reposition to the left
 *     - GREEN block → evade to the left, reposition to the right
 *   Front and left ultrasonic sensors detect corners and trigger 90° turns.
 *   After 12 turns the robot stops automatically (WRO rule compliance).
 *
 * HARDWARE:
 *   - Arduino UNO Q (main controller)
 *   - TB6612FNG motor driver (PWM speed control)
 *   - 50:1 Micro Metal Gearmotor HPCB 6V (traction)
 *   - TD-8125 digital servo (Ackermann steering, Pin 9)
 *   - DFRobot URM37 V5.0 ultrasonic × 2 (front: Pin 8/A0, left: Pin 10/A1)
 *   - Hiwonder WonderCam V2.0 (color detection, Color ID 1=Red, ID 2=Green)
 *   - Push button on Pin 11 (INPUT_PULLUP, WRO start trigger)
 *
 * CONFORMANCE:
 *   - Robot starts only after button press (WRO rules 9.10–9.11)
 *   - Stops automatically after exactly 3 laps / 12 corners
 *   - No external radio control during run
 * ============================================================
 */

#include <Arduino.h>
#include "WonderCam.h"
#include <Arduino_RouterBridge.h>
#include <Servo.h>

// ─── PIN DEFINITIONS ────────────────────────────────────────
#define PIN_BOTON      11   // Start button (INPUT_PULLUP — LOW when pressed)
#define URTRIG_FRENTE   8   // Front ultrasonic trigger pin
#define URECHO_FRENTE  A0   // Front ultrasonic echo pin
#define URTRIG_IZQ     10   // Left ultrasonic trigger pin
#define URECHO_IZQ     A1   // Left ultrasonic echo pin

// ─── CORNER DETECTION THRESHOLD ─────────────────────────────
// Front wall closer than this (cm) + left sensor open → trigger corner turn
#define UMBRAL_FRENTE    34.0

// ─── CORNER TURN PARAMETERS ─────────────────────────────────
#define VEL_GIRO         110  // Motor speed during 90° corner turn (0–255 PWM)
#define DURACION_GIRO_MS 1200 // Duration (ms) of full-lock servo during corner
#define TIEMPO_RETROCESO 500  // Duration (ms) of reverse after corner completes

// ─── MOTOR DRIVER PINS (TB6612FNG) ──────────────────────────
const int PWMA = 5;  // PWM speed control pin
const int AIN1 = 3;  // Direction control pin A
const int AIN2 = 2;  // Direction control pin B

// ─── PERIPHERAL OBJECTS ─────────────────────────────────────
WonderCam wc;        // Color detection camera (I²C via RouterBridge)
Servo direccion;     // Steering servo

// ─── COLOR DETECTION VARIABLES ──────────────────────────────
// Stores bounding-box center and area for each detected color
int centroX1 = 0, centroY1 = 0;  // Red block center (pixels, 0–320)
int centroX2 = 0, centroY2 = 0;  // Green block center
int area1 = 0, area2 = 0;         // Pixel area of each detected block

// ─── SERVO AND MOTION CONSTANTS ─────────────────────────────
#define CENTRO_SERVO      79   // Servo angle for straight-ahead driving (°)
#define VEL_NORMAL        100  // Cruise motor speed (PWM 0–255)

// ─── EVASION PARAMETERS ─────────────────────────────────────
#define AREA_MINIMA       5000  // Minimum block area (px²) to trigger evasion
                                 // Prevents reacting to distant or partial detections
#define ANGULO_ESQ_ROJO   122   // Servo angle when evading a RED block (right)
#define ANGULO_ESQ_VERDE   40   // Servo angle when evading a GREEN block (left)
#define TIEMPO_ESQUIVE    600   // Phase 1: duration of evasion turn (ms)
#define TIEMPO_RECTO      400   // Phase 2: straight segment between evasion and reposition (ms)
#define TIEMPO_REACOMODO  500   // Phase 3: reposition turn duration (ms)

// ─── ROBOT STATE ─────────────────────────────────────────────
bool robotActivo = false;  // False until button pressed; prevents motion before start

// ─── EVASION STATE MACHINE ───────────────────────────────────
// Three-phase evasion: turn away → brief straight → turn back to center
enum EstadoEsquive { LIBRE, GIRANDO_ESQ, RECTO_ESQ, REACOMODO };
EstadoEsquive estadoEsquive = LIBRE;
unsigned long tiempoInicioEsquive = 0;  // Timestamp when current evasion phase started
int anguloReacomodo = CENTRO_SERVO;      // Servo angle used in the reposition phase

// ─── CORNER STATE MACHINE ────────────────────────────────────
// Two-phase corner: full-lock turn → brief reverse to prevent wall contact
enum EstadoGiro { SIGUIENDO, GIRANDO, RETROCEDIENDO };
EstadoGiro estadoGiro = SIGUIENDO;
unsigned long tiempoInicioGiro = 0;  // Timestamp when corner turn started

// ─── LAP COUNTER ─────────────────────────────────────────────
int contadorGiros = 0;     // Counts completed 90° corners
#define TOTAL_GIROS 12     // 4 corners × 3 laps = 12 total; robot stops after this


// ════════════════════════════════════════════════════════════
// MOTOR HELPERS
// ════════════════════════════════════════════════════════════

/** Stop motor and center servo — called on completion or idle */
void detenerRobot() {
  analogWrite(PWMA, 0);
  direccion.write(CENTRO_SERVO);
}

/** Drive forward at given PWM speed (0–255) */
void adelante(int velocidad) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, velocidad);
}

/** Drive backward at given PWM speed (0–255) */
void atras(int velocidad) {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, velocidad);
}


// ════════════════════════════════════════════════════════════
// ULTRASONIC SENSOR READING
// ════════════════════════════════════════════════════════════

/**
 * Read distance from a URM37 V5.0 ultrasonic sensor.
 *
 * Protocol: pull TRIG LOW for 100 µs → sensor fires pulse → ECHO goes LOW
 * for a duration proportional to distance. Formula: distance (cm) = duration / 50.
 *
 * Returns -1 if no echo received within 50,000 µs (out of range or obstruction).
 * The 50 ms timeout caps the useful range at ~100 cm and prevents blocking.
 */
float leerDistancia(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(100);                          // URM37 trigger pulse
  unsigned long duracion = pulseIn(echo, LOW, 50000);
  if (duracion == 0 || duracion >= 50000) return -1;
  return duracion / 50.0;                          // URM37-specific conversion
}


// ════════════════════════════════════════════════════════════
// SETUP
// ════════════════════════════════════════════════════════════

void setup() {
  Bridge.begin();         // Initialize Arduino_RouterBridge (required for WonderCam)
  Monitor.begin(9600);    // Serial monitor for real-time debugging
  wc.begin();             // Initialize WonderCam

  // Activate color detection mode (Color ID 1 = Red, Color ID 2 = Green)
  if (wc.changeFunc(APPLICATION_COLORDETECT)) {
    Monitor.println("Modo COLOR activado");
  } else {
    Monitor.println("ERROR: No se pudo activar modo COLOR");
  }

  // Steering servo — centered at startup
  direccion.attach(9);
  direccion.write(CENTRO_SERVO);

  // Ultrasonic sensors — URM37 idle state is TRIG=HIGH
  pinMode(URTRIG_FRENTE, OUTPUT); digitalWrite(URTRIG_FRENTE, HIGH);
  pinMode(URECHO_FRENTE, INPUT);
  pinMode(URTRIG_IZQ,    OUTPUT); digitalWrite(URTRIG_IZQ,    HIGH);
  pinMode(URECHO_IZQ,    INPUT);

  // Motor driver pins
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  detenerRobot();

  // Start button — INPUT_PULLUP: LOW when pressed (WRO rules 9.10–9.11)
  pinMode(PIN_BOTON, INPUT_PULLUP);
  Monitor.println("Presiona el pulsador para iniciar...");
}


// ════════════════════════════════════════════════════════════
// MAIN LOOP
// ════════════════════════════════════════════════════════════

void loop() {

  // ── 1. BUTTON CHECK ──────────────────────────────────────
  // Robot remains stopped until operator presses the start button.
  // 50 ms debounce prevents false triggers from switch bounce.
  if (!robotActivo && digitalRead(PIN_BOTON) == LOW) {
    delay(50);
    if (digitalRead(PIN_BOTON) == LOW) {
      robotActivo   = true;
      estadoEsquive = LIBRE;
      estadoGiro    = SIGUIENDO;
      contadorGiros = 0;
      Monitor.println("¡Robot iniciado!");
    }
  }

  if (!robotActivo) {
    detenerRobot();
    return;
  }

  // ── 2. CORNER FSM — GIRANDO ──────────────────────────────
  // Executes 90° full-lock turn at corner. Takes priority over everything else.
  // Servo was set to full-left before entering this state (CENTRO_SERVO - 60).
  if (estadoGiro == GIRANDO) {
    adelante(VEL_GIRO);
    if (millis() - tiempoInicioGiro >= DURACION_GIRO_MS) {
      direccion.write(CENTRO_SERVO);   // Re-center before reversing
      detenerRobot();
      tiempoInicioGiro = millis();
      estadoGiro = RETROCEDIENDO;
      Monitor.println("Giro completado → RETROCEDIENDO");
    }
    return;
  }

  // ── 3. CORNER FSM — RETROCEDIENDO ───────────────────────
  // Brief reverse to avoid clipping the corner wall after the turn.
  // After reversing, increments corner counter and resumes normal driving.
  // If 12 corners completed, stops the robot (3 laps done).
  if (estadoGiro == RETROCEDIENDO) {
    atras(VEL_NORMAL);
    if (millis() - tiempoInicioGiro >= TIEMPO_RETROCESO) {
      detenerRobot();

      contadorGiros++;
      Monitor.print("Giro #"); Monitor.print(contadorGiros);
      Monitor.print("/"); Monitor.println(TOTAL_GIROS);

      if (contadorGiros >= TOTAL_GIROS) {
        // 3 laps completed — stop permanently (WRO automatic stop rule)
        detenerRobot();
        robotActivo = false;
        Monitor.println("=== 3 VUELTAS COMPLETADAS ===");
      } else {
        estadoGiro = SIGUIENDO;
      }
    }
    return;
  }

  // ── 4. EVASION FSM ───────────────────────────────────────
  // Three-phase evasion sequence runs to completion before normal driving resumes.
  switch (estadoEsquive) {

    case GIRANDO_ESQ:
      // Phase 1: steer hard away from the block while driving forward
      adelante(VEL_NORMAL);
      if (millis() - tiempoInicioEsquive >= TIEMPO_ESQUIVE) {
        direccion.write(CENTRO_SERVO);       // Straighten before brief straight
        tiempoInicioEsquive = millis();
        estadoEsquive = RECTO_ESQ;
        Monitor.println("Esquive: recto intermedio...");
      }
      return;

    case RECTO_ESQ:
      // Phase 2: short straight segment to separate from the block laterally
      adelante(VEL_NORMAL);
      if (millis() - tiempoInicioEsquive >= TIEMPO_RECTO) {
        direccion.write(anguloReacomodo);    // Begin reposition turn
        tiempoInicioEsquive = millis();
        estadoEsquive = REACOMODO;
        Monitor.println("Reacomodando...");
      }
      return;

    case REACOMODO:
      // Phase 3: steer back toward center lane after passing the block
      adelante(VEL_NORMAL);
      if (millis() - tiempoInicioEsquive >= TIEMPO_REACOMODO) {
        direccion.write(CENTRO_SERVO);       // Return to straight
        estadoEsquive = LIBRE;
        Monitor.println("Reacomodo completo → LIBRE");
      }
      return;

    case LIBRE:
    default:
      break;  // No evasion in progress — fall through to sensor reading
  }

  // ── 5. SENSOR READING ────────────────────────────────────
  float distFrente = leerDistancia(URTRIG_FRENTE, URECHO_FRENTE);
  float distIzq    = leerDistancia(URTRIG_IZQ,    URECHO_IZQ);

  // ── 6. CORNER DETECTION ──────────────────────────────────
  // Conditions: front wall within threshold AND left sensor reads open space.
  // Left > 100 cm indicates the robot is at a corner (open corridor on the left).
  // Servo is pre-set to full-left (CENTRO_SERVO - 60) before entering GIRANDO.
  if (distFrente > 0 && distFrente < UMBRAL_FRENTE && distIzq > 100) {
    tiempoInicioGiro = millis();
    direccion.write(CENTRO_SERVO - 60);  // Full-left lock for corner turn
    adelante(VEL_GIRO);
    estadoGiro = GIRANDO;
    Monitor.println("<<< GIRANDO ESQUINA");
    return;
  }

  // ── 7. COLOR DETECTION ───────────────────────────────────
  // Reset detection variables before each camera query
  area1 = 0; area2 = 0;
  centroX1 = 0; centroX2 = 0;
  centroY1 = 0; centroY2 = 0;

  wc.updateResult();  // Fetch latest frame data from WonderCam

  // Color ID 1 = Red block
  bool Color1Detected = wc.colorIdDetected(1);
  // Color ID 2 = Green block
  bool Color2Detected = wc.colorIdDetected(2);

  if (Color1Detected) {
    WonderCamColorDetectResult r1;
    if (wc.colorId(1, &r1)) {
      centroX1 = r1.x + (r1.w / 2);   // Bounding box center X (0=left, 320=right)
      centroY1 = r1.y + (r1.h / 2);   // Bounding box center Y
      area1    = r1.w * r1.h;           // Area used to determine proximity
      Monitor.print("ROJO | X="); Monitor.print(centroX1);
      Monitor.print(" Area="); Monitor.println(area1);
    }
  }

  if (Color2Detected) {
    WonderCamColorDetectResult r2;
    if (wc.colorId(2, &r2)) {
      centroX2 = r2.x + (r2.w / 2);
      centroY2 = r2.y + (r2.h / 2);
      area2    = r2.w * r2.h;
      Monitor.print("VERDE | X="); Monitor.print(centroX2);
      Monitor.print(" Area="); Monitor.println(area2);
    }
  }

  // ── 8. DECISION LOGIC ────────────────────────────────────
  /*
   * Priority logic for block response:
   *
   * a) RED block dominant and large enough  → full evasion right, reposition left
   * b) RED block visible but small          → soft tracking (follow X-center)
   * c) GREEN block dominant and large enough → full evasion left, reposition right
   * d) GREEN block visible but small         → soft tracking
   * e) No blocks detected                   → drive straight
   *
   * "Dominant" means its area exceeds the other color's area AND exceeds AREA_MINIMA.
   * Soft tracking uses map() to convert the block's X position to a servo angle,
   * keeping the block centered in the camera frame while remaining in the correct lane.
   */

  if (area1 > area2 && area1 > AREA_MINIMA) {
    // ── RED block is close and dominant → evade RIGHT ──
    // ANGULO_ESQ_ROJO steers right to pass block on its left side (red = left lane)
    Monitor.println("ROJO → esquivar");
    direccion.write(ANGULO_ESQ_ROJO);
    anguloReacomodo = ANGULO_ESQ_VERDE;  // After evasion, reposition using green angle
    tiempoInicioEsquive = millis();
    estadoEsquive = GIRANDO_ESQ;

  } else if (area1 > 0) {
    // ── RED block visible but small → soft tracking ──
    // map() inverts X (320→0° means: block on right → steer left)
    int angulo = map(centroX1, 0, 320, 180, 0);
    angulo = constrain(angulo, 0, 180);
    direccion.write(angulo);
    adelante(VEL_NORMAL);
    Monitor.print("Siguiendo ROJO | Servo="); Monitor.println(angulo);

  } else if (area2 > area1 && area2 > AREA_MINIMA) {
    // ── GREEN block is close and dominant → evade LEFT ──
    // ANGULO_ESQ_VERDE steers left to pass block on its right side (green = right lane)
    Monitor.println("VERDE → esquivar");
    direccion.write(ANGULO_ESQ_VERDE);
    anguloReacomodo = ANGULO_ESQ_ROJO;  // After evasion, reposition using red angle
    tiempoInicioEsquive = millis();
    estadoEsquive = GIRANDO_ESQ;

  } else if (area2 > 0) {
    // ── GREEN block visible but small → soft tracking ──
    // map() keeps original direction (0→0° means: block on left → steer left)
    int angulo = map(centroX2, 0, 320, 0, 180);
    angulo = constrain(angulo, 0, 180);
    direccion.write(angulo);
    adelante(VEL_NORMAL);
    Monitor.print("Siguiendo VERDE | Servo="); Monitor.println(angulo);

  } else {
    // ── No blocks detected → drive straight ──
    direccion.write(CENTRO_SERVO);
    adelante(VEL_NORMAL);
    Monitor.println("Sin bloque — recto");
  }

  delay(50);  // 50 ms loop period → ~20 Hz sensor/camera update rate
}
