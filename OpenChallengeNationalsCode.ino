#include <ESP32Servo.h>

#define TRIG_FRENTE 33
#define ECHO_FRENTE 19
#define TRIG_DER 18
#define ECHO_DER 5
#define TRIG_IZQ 32
#define ECHO_IZQ 35
#define PIN_SERVO 21
#define PIN_BOTON 34

Servo Direccion;

const int PWMA = 14;
const int AIN1 = 12;
const int AIN2  =13;

#define CENTRO         81
#define VEL_NORMAL     180
#define VEL_GIRO       210
#define DISTANCIA_GIRO 80
#define TOTAL_GIROS    12
#define TGIRO          1400
#define TFinal         2400
#define TFront         1000

bool robotActivo      = false;
float ulLecturaFrente = 999;
float ulLecturaDer    = 999;
float ulLecturaIzq    = 999;

double setpoint_Der = 40.0;
double setpoint_Izq = 40.0;

double Kp = 10.0;
double Ki = 0.05;
double Kd = 5.0;

double error          = 0;
double error_anterior = 0;
double suma_errores   = 0;

unsigned long tiempoPrevio = 0;
double dt = 0;

int contadorGiros = 0;
int opcion        = 0; // 0=sin definir, 1=derecha, 2=izquierda
int opcion_2      = 0;

float leerDistancia(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long duracion = pulseIn(echo, HIGH, 30000);
  if (duracion == 0) return -1;
  return duracion / 58.0;
}

void moverServo(int angulo) {
  Direccion.write(constrain(angulo, 0, 180));
}

void adelante(int velocidad) {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, velocidad);
}

void detenerRobot() {
  moverServo(CENTRO);
  analogWrite(PWMA, 0);
  error = 0; error_anterior = 0; suma_errores = 0;
}

void girarIzquierda() {
  moverServo(CENTRO - 40);
  adelante(VEL_GIRO);
  delay(TGIRO);
  moverServo(CENTRO);
  adelante(VEL_NORMAL);
  delay(TFront);
  ulLecturaFrente = 999;
  error = 0; error_anterior = 0; suma_errores = 0;
  tiempoPrevio = millis();
  Serial.println("GIRANDO IZQUIERDA");
}

void girarDerecha() {
  moverServo(CENTRO + 40);
  adelante(VEL_GIRO);
  delay(TGIRO);
  moverServo(CENTRO);
  adelante(VEL_NORMAL);
  delay(TFront);
  ulLecturaFrente = 999;
  error = 0; error_anterior = 0; suma_errores = 0;
  tiempoPrevio = millis();
  Serial.println("GIRANDO DERECHA");
}

void PIDderecho() {
  error = ulLecturaDer - setpoint_Der;

  double P = Kp * error;
  double D = Kd * ((error - error_anterior) / dt);
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

  Serial.print("PID_DER | Err: "); Serial.print(error);
  Serial.print(" | Srv: "); Serial.println(anguloServo);
}

void PIDizquierdo() {
  error = -ulLecturaIzq + setpoint_Izq;

  double P = Kp * error;
  double D = Kd * ((error - error_anterior) / dt);
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

  Serial.print("PID_IZQ | Err: "); Serial.print(error);
  Serial.print(" | Srv: "); Serial.println(anguloServo);
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_FRENTE, OUTPUT); digitalWrite(TRIG_FRENTE, LOW);
  pinMode(ECHO_FRENTE, INPUT);
  pinMode(TRIG_IZQ,    OUTPUT); digitalWrite(TRIG_IZQ,    HIGH);
  pinMode(ECHO_IZQ,    INPUT);
  pinMode(TRIG_DER,    OUTPUT); digitalWrite(TRIG_DER,    HIGH);
  pinMode(ECHO_DER,    INPUT);

  Direccion.attach(PIN_SERVO);
  Direccion.write(CENTRO);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  detenerRobot();

  pinMode(PIN_BOTON, INPUT_PULLUP);
}

void loop() {
  float distFrente = leerDistancia(TRIG_FRENTE, ECHO_FRENTE);
  float distDer    = leerDistancia(TRIG_DER, ECHO_DER);
  float distIzq    = leerDistancia(TRIG_IZQ, ECHO_IZQ);

  if (distFrente > 0) ulLecturaFrente = distFrente;
  if (distDer    > 0) ulLecturaDer    = distDer;
  if (distIzq    > 0) ulLecturaIzq    = distIzq;

  Serial.print("F: "); Serial.print(ulLecturaFrente);
  Serial.print(" | I: "); Serial.print(ulLecturaIzq);
  Serial.print(" | D: "); Serial.println(ulLecturaDer);

  if (!robotActivo && digitalRead(PIN_BOTON) == LOW) {
    delay(50);
    if (digitalRead(PIN_BOTON) == LOW) {
      // ✅ Dos setpoints independientes
      if (opcion_2 == 0) {
        setpoint_Der = (ulLecturaDer < 400) ? ulLecturaDer : 40.0;
        setpoint_Izq = (ulLecturaIzq < 400) ? ulLecturaIzq : 40.0;

        if (setpoint_Der > setpoint_Izq) {
          opcion_2 = 1;
        } else {
          opcion_2 = 2;
        }
        Serial.print("SP_Der: "); Serial.print(setpoint_Der);
        Serial.print(" | SP_Izq: "); Serial.println(setpoint_Izq);
        tiempoPrevio = millis();
        robotActivo  = true;
      }
    }
  }

  if (!robotActivo) {
    detenerRobot();
    return;
  }

  unsigned long ahora = millis();
  dt = (ahora - tiempoPrevio) / 1000.0;
  if (dt < 0.001) dt = 0.001;
  tiempoPrevio = ahora;

  // --- Giro ---
  if (ulLecturaFrente < DISTANCIA_GIRO && (distIzq > 100 || distDer > 100)) {

    if (opcion == 0) {
      // ✅ Primer giro — determinar dirección
      detenerRobot();
      delay(1500);

      // Releer sensores tras detenerse
      float lIzq = leerDistancia(TRIG_IZQ, ECHO_IZQ);
      float lDer = leerDistancia(TRIG_DER, ECHO_DER);
      if (lIzq > 0) ulLecturaIzq = lIzq;
      if (lDer > 0) ulLecturaDer = lDer;

      if (ulLecturaIzq > ulLecturaDer) {
        opcion = 1; // ✅ Más espacio a la izquierda → girar derecha
        girarIzquierda();
      } else {
        opcion = 2; // ✅ Más espacio a la derecha → girar izquierda
        girarDerecha();
      }

    } else if (opcion == 1) {
      girarIzquierda();   // ✅ Mantener dirección
    } else if (opcion == 2) {
      girarDerecha(); // ✅ Mantener dirección
    }

    contadorGiros++;
    Serial.print("Giro #"); Serial.print(contadorGiros);
    Serial.print("/"); Serial.println(TOTAL_GIROS);

    if (contadorGiros >= TOTAL_GIROS) {
      adelante(VEL_NORMAL);
      delay(TFinal);
      detenerRobot();
      robotActivo = false;
      Serial.println("=== 3 VUELTAS COMPLETADAS ===");
    }
    return;
  }

  // --- PID según opción ---
  if (opcion_2 == 1) {
    PIDizquierdo();
  } else if (opcion_2 == 2) {
    PIDderecho();
  } else {
    // Antes del primer giro → recto
    moverServo(CENTRO);
    adelante(VEL_NORMAL);
  }

}
