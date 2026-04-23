// ============================================================
//  WRO Future Engineers 2026 — Electrochalanes
//  Sketch principal integrado — Desafío Abierto
//  Placa: Arduino UNO Q (Arduino_RouterBridge)
// ============================================================
//
//  DIAGRAMA DE PINES
//  ─────────────────
//  Motor driver
//    PWMA  → pin 5   (velocidad, PWM 10-bit con analogWriteResolution)
//    AIN1  → pin 3   (dirección del motor)
//    AIN2  → pin 2   (dirección del motor)
//
//  Servo de dirección
//    señal → pin 9
//
//  Sensor URM37 FRONTAL   (detecta pared para girar)
//    TRIG  → pin 8
//    ECHO  → pin 14 (A0)
//
//  Sensor URM37 IZQUIERDO (PID lateral)
//    TRIG  → pin 10
//    ECHO  → pin 15 (A1)
//
//  Sensor URM37 DERECHO   (PID lateral)
//    TRIG  → pin 12
//    ECHO  → pin 16 (A2)
//
//  WonderCam
//    I2C   → SDA/SCL (conexión estándar)
//
//  UI
//    LED   → pin 4
//    BTN   → pin 7   (INPUT_PULLUP, activo en LOW)
//
//  MÁQUINA DE ESTADOS
//  ──────────────────
//  ESPERA → (botón) → CORRIENDO → (pared frontal) → GIRANDO
//         → (giro completo) → CORRIENDO → ... × 3 vueltas
//         → COMPLETADO (motor off, LED parpadea)
//
//  LÓGICA DE SENTIDO
//  ─────────────────
//  Primera detección de color con WonderCam:
//    Color ID 1 (naranja) → giro a la DERECHA (sentido antihorario)
//    Color ID 2 (azul)    → giro a la IZQUIERDA (sentido horario)
//  El sentido se fija en la primera vuelta y se repite.
// ============================================================

#include <Arduino_RouterBridge.h>
#include <Servo.h>
#include "WonderCam.h"


// ── Pines ────────────────────────────────────────────────────
#define PIN_PWMA      5
#define PIN_AIN1      3
#define PIN_AIN2      2
#define PIN_SERVO     9
//#define PIN_LED       4
#define PIN_BTN       7

#define TRIG_F        8     // Frontal
#define ECHO_F       14
#define TRIG_IZQ     10     // Izquierdo
#define ECHO_IZQ     15
#define TRIG_DER     12     // Derecho
#define ECHO_DER     16

// ── Parámetros de servo ──────────────────────────────────────
const int SERVO_CENTRO = 81;
const int SERVO_MAX_IZQ = 40;   // giro máximo izquierda
const int SERVO_MAX_DER = 138;  // giro máximo derecha

// ── Parámetros del motor ─────────────────────────────────────
// Resolución 10-bit → rango 0-1023
const int VEL_CRUCERO   = 680;  // velocidad normal en recta
const int VEL_GIRO      = 550;  // velocidad reducida al girar
const int VEL_CERO      = 0;

// ── Parámetros PID (centrado en carril) ──────────────────────
double Kp = 1.2;
double Ki = 0.04;
double Kd = 0.15;

double pid_error          = 0;
double pid_error_anterior = 0;
double pid_suma           = 0;
double pid_salida         = 0;

const unsigned long PID_INTERVALO_MS = 50;   // 20 Hz
unsigned long pid_t_previo = 0;

// ── Parámetros de detección de pared (giro) ──────────────────
const int DIST_PARED_GIRO  = 55;  // cm — inicia secuencia de giro
const int DIST_LATERAL_MIN = 10;  // cm — límite de seguridad lateral

// ── Parámetros de giro ────────────────────────────────────────
const unsigned long T_GIRO_MS      = 900;   // duración del giro (ajustar en pista)
const unsigned long T_ENDEREZAR_MS = 300;   // tiempo para enderezar tras el giro

// ── Contador de vueltas ───────────────────────────────────────
// El circuito WRO tiene 4 esquinas por vuelta → 3 vueltas = 12 esquinas
const int TOTAL_ESQUINAS = 12;
int esquinas_completadas = 0;

// ── Posición de inicio (se mide al arrancar) ──────────────────
// Guardamos las lecturas de los 3 sensores justo antes de soltar el botón.
// Al final de las 3 vueltas el carro busca volver a estas distancias.
float pos_frontal_ini = -1.0;
float pos_izq_ini     = -1.0;
float pos_der_ini     = -1.0;

// Tolerancia en cm para considerar que el carro está en posición.
// ±5 cm es suficiente para WRO; reducir si el juez es más estricto.
const float TOLERANCIA_PARADA_CM = 5.0;

// Velocidad reducida durante la fase de aproximación final
const int VEL_APROXIMACION = 480;

// ── Sentido de giro ───────────────────────────────────────────
// 0 = no definido, 1 = derecha (naranja), -1 = izquierda (azul)
int sentido_giro = 0;
bool sentido_fijado = false;

// ── Máquina de estados ────────────────────────────────────────
enum Estado { ESPERA, CORRIENDO, GIRANDO, ENDEREZANDO, APROXIMACION, COMPLETADO };
Estado estado_actual = ESPERA;

unsigned long t_giro_inicio    = 0;
unsigned long t_enderezo_inicio = 0;

// ── Objetos ───────────────────────────────────────────────────
Servo servo_dir;
WonderCam cam;

// ════════════════════════════════════════════════════════════
//  FUNCIONES AUXILIARES
// ════════════════════════════════════════════════════════════

// Leer un sensor URM37 V5 de forma individual (secuencial)
// Protocolo correcto URM37 v5.0:
//   TRIG reposa en HIGH → baja a LOW 100µs → pulseIn en LOW
// Retorna distancia en cm, o -1 si no hay eco válido
float leerURM37(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(100);         // URM37 v5.0 requiere 100µs en LOW para disparar

  unsigned long duracion = pulseIn(echoPin, LOW, 50000);
  digitalWrite(trigPin, HIGH);    // Volver al estado de reposo

  if (duracion == 0 || duracion >= 50000) return -1.0;
  return (float)duracion / 50.0;  // Cada 50µs = 1 cm (fórmula específica URM37)
}

// Mover motor hacia adelante con velocidad dada (0-1023)
void motorAdelante(int velocidad) {
  velocidad = constrain(velocidad, 0, 1023);
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, HIGH);
  analogWrite(PIN_PWMA, velocidad);
}

// Detener motor
void motorStop() {
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  analogWrite(PIN_PWMA, 0);
}

// Aplicar ángulo al servo con límites de seguridad
void aplicarServo(int angulo) {
  angulo = constrain(angulo, SERVO_MAX_IZQ, SERVO_MAX_DER);
  servo_dir.write(angulo);
}

// Parpadear LED n veces
void parpadearLED(int n, int ms_on = 150, int ms_off = 150) {
  for (int i = 0; i < n; i++) {
    digitalWrite(PIN_LED, HIGH); delay(ms_on);
    digitalWrite(PIN_LED, LOW);  delay(ms_off);
  }
}

// Consultar WonderCam y actualizar sentido_giro si aún no está fijado
// Retorna true si detectó un color y actualizó el sentido
bool detectarColorCamara() {
  cam.updateResult();

  bool colorNaranja = cam.colorIdDetected(1);  // ID 1 = naranja
  bool colorAzul    = cam.colorIdDetected(2);  // ID 2 = azul

  if (!colorNaranja && !colorAzul) return false;

  if (!sentido_fijado) {
    if (colorNaranja) {
      sentido_giro  = 1;   // derecha
      sentido_fijado = true;
      LOGLN("CAM: Naranja detectado → giro DERECHA fijado");
    } else if (colorAzul) {
      sentido_giro  = -1;  // izquierda
      sentido_fijado = true;
      LOGLN("CAM: Azul detectado → giro IZQUIERDA fijado");
    }
  }
  return true;
}

// Calcular y aplicar PID lateral
// error = dist_izq - dist_der → si >0 el carro está más cerca de la izq, gira a la der
void actualizarPID(float dist_izq, float dist_der) {
  unsigned long ahora = millis();
  double dt = (ahora - pid_t_previo) / 1000.0;
  if (dt < (PID_INTERVALO_MS / 1000.0)) return;
  pid_t_previo = ahora;

  // Ignorar lecturas inválidas
  if (dist_izq < 0 || dist_der < 0) return;

  pid_error = dist_izq - dist_der;   // setpoint implícito = 0 (centrado)

  double P = Kp * pid_error;

  pid_suma = constrain(pid_suma + pid_error * dt, -80.0, 80.0);
  double I = Ki * pid_suma;

  double D = Kd * ((pid_error - pid_error_anterior) / dt);
  pid_error_anterior = pid_error;

  pid_salida = constrain(P + I + D, -(SERVO_CENTRO - SERVO_MAX_IZQ),
                                      (SERVO_MAX_DER - SERVO_CENTRO));

  aplicarServo((int)(SERVO_CENTRO + pid_salida));

  LOG("PID | izq="); LOG(dist_izq);
  LOG(" der="); LOG(dist_der);
  LOG(" err="); LOG(pid_error);
  LOG(" sal="); LOGLN((int)pid_salida);
}

// Iniciar secuencia de giro (se llama al detectar pared)
void iniciarGiro() {
  estado_actual  = GIRANDO;
  t_giro_inicio  = millis();
  motorAdelante(VEL_GIRO);

  if (sentido_giro == 1) {
    aplicarServo(SERVO_MAX_DER);
    LOGLN("GIRO → DERECHA");
  } else if (sentido_giro == -1) {
    aplicarServo(SERVO_MAX_IZQ);
    LOGLN("GIRO → IZQUIERDA");
  } else {
    // Sentido aún no detectado: usar sensores laterales como respaldo
    // El lado con más espacio = el lado hacia donde girar
    float izq = leerURM37(TRIG_IZQ, ECHO_IZQ);
    float der = leerURM37(TRIG_DER, ECHO_DER);
    if (izq > der) {
      aplicarServo(SERVO_MAX_IZQ);
      LOGLN("GIRO respaldo → IZQUIERDA (más espacio)");
    } else {
      aplicarServo(SERVO_MAX_DER);
      LOGLN("GIRO respaldo → DERECHA (más espacio)");
    }
  }
}

// ════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════
void setup() {
  Bridge.begin();
#ifdef DEBUG
  Monitor.begin(9600);
#endif

  // Pines de UI
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);

  // Pines del motor
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  motorStop();

  // Resolución PWM 10-bit para el motor
  analogWriteResolution(10);

  // Pines de sensores
  pinMode(TRIG_F,   OUTPUT); pinMode(ECHO_F,   INPUT);
  pinMode(TRIG_IZQ, OUTPUT); pinMode(ECHO_IZQ, INPUT);
  pinMode(TRIG_DER, OUTPUT); pinMode(ECHO_DER, INPUT);

  // Estado inicial de TRIG en HIGH (URM37 V5 inicia en reposo alto)
  digitalWrite(TRIG_F,   HIGH);
  digitalWrite(TRIG_IZQ, HIGH);
  digitalWrite(TRIG_DER, HIGH);

  // Servo
  servo_dir.attach(PIN_SERVO);
  aplicarServo(SERVO_CENTRO);

  // WonderCam
  cam.begin();
  if (cam.changeFunc(APPLICATION_COLORDETECT)) {
    LOGLN("CAM: Modo detección de color activado");
  } else {
    LOGLN("CAM: ERROR al activar detección de color");
  }

  // Señal de listo: 2 parpadeos
  parpadearLED(2);
  LOGLN("Sistema listo. Esperando botón de inicio...");

  // ── Esperar botón de inicio ──────────────────────────────
  while (digitalRead(PIN_BTN) == HIGH) {
    digitalWrite(PIN_LED, HIGH);  // LED encendido mientras espera
    delay(20);
  }

  // ── Medir posición de inicio (3 lecturas promediadas por sensor) ──
  // Se hace con el carro quieto para máxima precisión.
  // Si algún sensor no lee, pos_*_ini queda en -1 y esa coordenada
  // no se usará como criterio de parada.
  LOGLN("Midiendo posicion de inicio...");
  float sf = 0, si = 0, sd = 0;
  const int N_MUESTRAS = 3;
  for (int i = 0; i < N_MUESTRAS; i++) {
    float f = leerURM37(TRIG_F,   ECHO_F);   delay(30);
    float l = leerURM37(TRIG_IZQ, ECHO_IZQ); delay(30);
    float r = leerURM37(TRIG_DER, ECHO_DER); delay(30);
    if (f > 0) sf += f;
    if (l > 0) si += l;
    if (r > 0) sd += r;
  }
  pos_frontal_ini = (sf > 0) ? sf / N_MUESTRAS : -1.0;
  pos_izq_ini     = (si > 0) ? si / N_MUESTRAS : -1.0;
  pos_der_ini     = (sd > 0) ? sd / N_MUESTRAS : -1.0;

  LOG("Pos inicio — F:"); LOG(pos_frontal_ini);
  LOG(" I:");             LOG(pos_izq_ini);
  LOG(" D:");             LOGLN(pos_der_ini);

  // Confirmar inicio: 3 parpadeos rápidos
  parpadearLED(3, 80, 80);
  digitalWrite(PIN_LED, HIGH);

  LOGLN("¡Inicio!");
  estado_actual = CORRIENDO;
  pid_t_previo  = millis();
  motorAdelante(VEL_CRUCERO);
}

// ════════════════════════════════════════════════════════════
//  LOOP
// ════════════════════════════════════════════════════════════
void loop() {

  // ── ESTADO: COMPLETADO ──────────────────────────────────
  if (estado_actual == COMPLETADO) {
    motorStop();
    aplicarServo(SERVO_CENTRO);
    // Parpadeo continuo de celebración
    parpadearLED(1, 200, 200);
    return;
  }

  // ── Leer los 3 sensores de forma SECUENCIAL ─────────────
  // Cada lectura espera su propio eco antes de disparar el siguiente
  float dist_frontal = leerURM37(TRIG_F,   ECHO_F);
  delay(5);   // pequeña separación entre disparos
  float dist_izq     = leerURM37(TRIG_IZQ, ECHO_IZQ);
  delay(5);
  float dist_der     = leerURM37(TRIG_DER, ECHO_DER);
  delay(5);

  LOG("F="); LOG(dist_frontal);
  LOG(" I="); LOG(dist_izq);
  LOG(" D="); LOGLN(dist_der);

  // ── ESTADO: CORRIENDO ────────────────────────────────────
  if (estado_actual == CORRIENDO) {

    // 1. Intentar detectar color con la cámara
    detectarColorCamara();

    // 2. PID lateral para mantener el carril
    actualizarPID(dist_izq, dist_der);

    // 3. Verificar si hay pared frontal para girar
    if (dist_frontal > 0 && dist_frontal < DIST_PARED_GIRO) {
      LOGLN("Pared detectada → iniciando giro");
      iniciarGiro();
    }
  }

  // ── ESTADO: GIRANDO ──────────────────────────────────────
  else if (estado_actual == GIRANDO) {
    unsigned long t_transcurrido = millis() - t_giro_inicio;

    if (t_transcurrido >= T_GIRO_MS) {
      // Giro completado → enderezar
      estado_actual      = ENDEREZANDO;
      t_enderezo_inicio  = millis();
      aplicarServo(SERVO_CENTRO);
      motorAdelante(VEL_CRUCERO);

      esquinas_completadas++;
      LOG("Esquinas: "); LOGLN(esquinas_completadas);

      if (esquinas_completadas >= TOTAL_ESQUINAS) {
        // 3 vueltas completadas → buscar posición de inicio
        LOGLN("¡3 VUELTAS! Entrando en APROXIMACION...");
        estado_actual = APROXIMACION;
        motorAdelante(VEL_APROXIMACION);  // velocidad reducida para parada precisa
        pid_suma = 0;
      }
    }
  }

  // ── ESTADO: ENDEREZANDO ──────────────────────────────────
  else if (estado_actual == ENDEREZANDO) {
    if (millis() - t_enderezo_inicio >= T_ENDEREZAR_MS) {
      estado_actual = CORRIENDO;
      pid_suma      = 0;
      LOGLN("Volviendo a CORRIENDO");
    }
  }

  // ── ESTADO: APROXIMACION ─────────────────────────────────
  // El carro avanza despacio con PID activo y compara las lecturas
  // actuales contra las guardadas al inicio.
  // Solo usa como criterio los sensores que tuvieron lectura válida (>0).
  // Condición de parada: TODOS los sensores válidos dentro de tolerancia.
  else if (estado_actual == APROXIMACION) {

    // PID lateral sigue activo para mantenerse centrado
    actualizarPID(dist_izq, dist_der);

    bool frontal_ok = true;
    bool izq_ok     = true;
    bool der_ok     = true;

    if (pos_frontal_ini > 0 && dist_frontal > 0) {
      frontal_ok = abs(dist_frontal - pos_frontal_ini) <= TOLERANCIA_PARADA_CM;
    }
    if (pos_izq_ini > 0 && dist_izq > 0) {
      izq_ok = abs(dist_izq - pos_izq_ini) <= TOLERANCIA_PARADA_CM;
    }
    if (pos_der_ini > 0 && dist_der > 0) {
      der_ok = abs(dist_der - pos_der_ini) <= TOLERANCIA_PARADA_CM;
    }

    LOG("APROX | F_err:"); LOG(dist_frontal - pos_frontal_ini);
    LOG(" I_err:"); LOG(dist_izq - pos_izq_ini);
    LOG(" D_err:"); LOGLN(dist_der - pos_der_ini);

    if (frontal_ok && izq_ok && der_ok) {
      LOGLN("Posicion de inicio alcanzada. COMPLETADO.");
      estado_actual = COMPLETADO;
    }
  }
}
