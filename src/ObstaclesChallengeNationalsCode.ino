#include <Arduino.h>
#include <ESP32Servo.h>
#include "WonderCam.h"

#define SDA 21
#define SCL 22

#define TRIG_FRENTE 33
#define ECHO_FRENTE 19
#define TRIG_DER 18
#define ECHO_DER 5
#define TRIG_IZQ 32
#define ECHO_IZQ 35
#define PIN_SERVO 17
#define PIN_BOTON 34

WonderCam Cam;
Servo Direccion;

const int PWMA = 14;
const int AIN1 = 12;
const int AIN2  =13;

#define Acentral 80
#define Areversa 51
#define Asalida_izquierda 45
#define Asalida_derecha   135

#define AesquiveRojo 111
#define AreacomodoRojo 51

#define AesquiveVerde 51
#define AreacomodoVerde 111

#define Tcentral 700
#define Tesquive 700
#define Treacomodo 500

#define Tgiro 1600
#define Talto 500
#define Tatras 1000

#define Tatras_salida     300
#define Tadelante_salida  350
#define Tadelante_afuera  1000
#define Tatras_Acomodo    600

#define VELOCIDAD_NORMAL 170
#define VELOCIDAD_GIRO 180
#define VELOCIDAD_ESQUIVE 200

#define Velocidad_salida  190

#define AREA_MINIMA_VERDE 6000
#define AREA_MINIMA_ROJO 3500
#define DISTANCIA_FRENTE 70

#define TOTAL_GIROS 12

bool robotActivo = false;

unsigned long lastUpdate  = 0;
const unsigned long updateInterval  = 100;

int area1 = 0, area2 = 0;
int centroX1 = 0, centroX2 = 0;
int centroY2 = 0;

float ulLecturaFrente = 999;
float ulLecturaDer = 999;
float ulLecturaIzq = 999;

int opcion = 0;

int contadorGiros = 0;

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

void adelante(int velocidad) {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, velocidad);
}

void atras(int velocidad) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, velocidad);
}

void moverServo(int angulo) {
  Direccion.write(angulo);
}

void detenerRobot() {
  moverServo(Acentral);
  analogWrite(PWMA, 0);
}

void esquivarRojo() {
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

void esquivarVerde() {
  detenerRobot();
  delay(500);
  atras(VELOCIDAD_NORMAL);
  delay(500);
  detenerRobot();
  delay(500);
  adelante(VELOCIDAD_ESQUIVE);
  moverServo(AesquiveVerde);
  delay(Tesquive);
  adelante(VELOCIDAD_ESQUIVE);
  moverServo(Acentral);
  delay(Tcentral);
  adelante(VELOCIDAD_ESQUIVE);
  moverServo(AreacomodoVerde);
  delay(Treacomodo);
  moverServo(Acentral);
}

void girarDerecha() {
  moverServo(Acentral + 40);
  adelante(VELOCIDAD_GIRO);
  delay(Tgiro);
  moverServo(Acentral);
  detenerRobot();
  delay(Talto);
  moverServo(Areversa);
  atras(VELOCIDAD_GIRO);
  delay(Tatras);
  detenerRobot();
  delay(500);
  adelante(VELOCIDAD_NORMAL);
  moverServo(Acentral);
  delay(1000);
}

void girarIzquierda() {
  moverServo(Acentral - 40);
  adelante(VELOCIDAD_GIRO);
  delay(Tgiro);
  moverServo(Acentral);
  detenerRobot();
  delay(Talto);
  moverServo(AesquiveRojo);
  atras(VELOCIDAD_GIRO);
  delay(Tatras);
  detenerRobot();
  delay(500);
  adelante(VELOCIDAD_NORMAL);
  moverServo(Acentral);
  delay(1000);
}

void salirEstacionamientoIzquierda() {
  moverServo(Asalida_izquierda);
  adelante(Velocidad_salida);
  delay(Tadelante_salida);
  detenerRobot();
  delay(500);
  moverServo(Asalida_derecha);
  atras(Velocidad_salida);
  delay(Tatras_salida);
  detenerRobot();
  delay(500);
  moverServo(Asalida_izquierda);
  adelante(Velocidad_salida);
  delay(Tadelante_afuera);
  detenerRobot();
  delay(500);
  moverServo(Asalida_derecha);
  adelante(Velocidad_salida);
  delay(Tadelante_afuera);
  detenerRobot();
  delay(500);
  moverServo(Asalida_izquierda);
  atras(Velocidad_salida);
  delay(Tatras_Acomodo);
  detenerRobot();
}

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

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA, SCL);
  Wire.setClock(10000);

  Serial.println("A VER");
  Cam.begin();

  Cam.setLed(true);
  Serial.println("LUUUUUUUZ");

  delay(100);

  if (Cam.changeFunc(APPLICATION_COLORDETECT)) {
    Serial.println("Modo color activado correctamente");
  } else {
    Serial.println("ERROR: Falló al cambiar a modo color");
    // Podrías agregar un led de error aquí
  }
  pinMode(TRIG_FRENTE, OUTPUT); digitalWrite(TRIG_FRENTE, LOW);
  pinMode(ECHO_FRENTE, INPUT);
  pinMode(TRIG_IZQ,    OUTPUT); digitalWrite(TRIG_IZQ,    HIGH);
  pinMode(ECHO_IZQ,    INPUT);
  pinMode(TRIG_DER,    OUTPUT); digitalWrite(TRIG_DER,    HIGH);
  pinMode(ECHO_DER,    INPUT);

  Direccion.attach(PIN_SERVO);
  Direccion.write(Acentral);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  detenerRobot();

  pinMode(PIN_BOTON, INPUT_PULLUP);
  Serial.println("Sistema listo");

}

void loop() {
  Cam.updateResult();

  float distFrente = leerDistancia(TRIG_FRENTE, ECHO_FRENTE);
  float distDer = leerDistancia(TRIG_DER, ECHO_DER);
  float distIzq = leerDistancia(TRIG_IZQ, ECHO_IZQ);

  if (distFrente > 0) ulLecturaFrente = distFrente;
  if (distDer > 0) ulLecturaDer = distDer;
  if (distIzq > 0) ulLecturaIzq = distIzq;

  Serial.print("F: ");
  Serial.println(ulLecturaFrente);

  unsigned long ahora = millis();
  if (ahora - lastUpdate < updateInterval) {
    return;
  }
  lastUpdate = ahora;

  if (!Cam.updateResult()) {
    Serial.println("Error");
    return;
  }

  if (!robotActivo && digitalRead(PIN_BOTON) == LOW) {
    delay(50);
    if (digitalRead(PIN_BOTON) == LOW) {
      Serial.println("Empieza");
      robotActivo = true;
      if (opcion == 0) {
        float Lizq = leerDistancia(TRIG_IZQ, ECHO_IZQ);
        float Lder = leerDistancia(TRIG_DER, ECHO_DER);
        if (Lizq > 0) ulLecturaIzq = Lizq;
        if (Lder > 0) ulLecturaDer = Lder;

        if (ulLecturaDer > ulLecturaIzq) {
          opcion = 1;
          salirEstacionamientoDerecha();
        } else {
          opcion = 2;
          salirEstacionamientoIzquierda();
        }
      }
    }
  }

  if (!robotActivo) {
    detenerRobot();
    return;
  }

  area1 = 0;
  area2 = 0;
  centroX1 = 0;
  centroX2 = 0;
  centroY2 = 0;

  bool color1Detectado = Cam.colorIdDetected(1);
  bool color2Detectado = Cam.colorIdDetected(2);

  if(color1Detectado) {
    WonderCamColorDetectResult C1;
    Serial.print("Primer color");
    if(Cam.colorId(1, &C1)){
      centroX1 = C1.x + (C1.w / 2);
      area1 = C1.w * C1.h;
      Serial.print("ROJO | X="); Serial.print(centroX1);
      Serial.print(" Area="); Serial.println(area1);
    }
  }
  if(color2Detectado) {
    WonderCamColorDetectResult C2;
    Serial.print("Segundo color");
    if(Cam.colorId(2, &C2)){
      centroX2 = C2.x + (C2.w / 2);
      centroY2 = C2.y + (C2.h / 2);
      if (centroY2 > 150) {
        area2 = 0;
      } else {
        area2 = C2.w * C2.h;
      }
      Serial.print("VERDE | X="); Serial.print(centroX2);
      Serial.print(" Area="); Serial.println(area2);
    }
  }

  if (area1 > 0 || area2 > 0) {

    if (area1 >= area2) {
      // ✅ Verde tiene prioridad
      if (area1 >= AREA_MINIMA_VERDE) {
        esquivarVerde();
      } else {
        int angulo = map(centroX1, 0, 320, 51, 111);
        moverServo(constrain(angulo, 51, 111));
        adelante(VELOCIDAD_NORMAL);
      }
    } else {
      // ✅ Rojo tiene prioridad
      if (area2 >= AREA_MINIMA_ROJO) {
        if (centroY2 >  150) {
          area2 = 0;
        } else {
          esquivarRojo();
        }
      } else {
        int angulo = map(centroX2, 0, 320, 51, 111);
        moverServo(constrain(angulo, 51, 111));
        adelante(VELOCIDAD_NORMAL);
      }
    }

  } else {
    // --- Sin colores → sensor frontal ---
    moverServo(Acentral);
    adelante(VELOCIDAD_NORMAL);
    Serial.print("Sin colores | F: ");
    Serial.println(ulLecturaFrente);

    if (ulLecturaFrente < DISTANCIA_FRENTE && (distIzq > 100 || distDer > 100)) {
      if (opcion == 1) {
        girarDerecha();
        contadorGiros++;
      } else if (opcion == 2) {
        girarIzquierda();
        contadorGiros++;
      }
      if (contadorGiros >= TOTAL_GIROS) {
        robotActivo = false;
      }
    }
  }
}
