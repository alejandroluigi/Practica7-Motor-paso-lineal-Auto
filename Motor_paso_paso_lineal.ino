#include <Arduino.h>
#include <AccelStepper.h>

// PINES DRV8825
const int PIN_STEP   = 2;
const int PIN_DIR    = 3;
const int PIN_ENABLE = 4;

// FINALES DE CARRERA
const int PIN_SW_INICIO = 5;
const int PIN_SW_FINAL  = 6;

// PARAMETROS MECANICOS
const float AVANCE_HUSILLO_MM = 10.28;
const int PASOS_POR_VUELTA = 200;
const int MICROPASO = 1;

// Calculo de pasos para 100 mm (10 cm)
const long PASOS_10CM = (long)((100.0 / AVANCE_HUSILLO_MM) * PASOS_POR_VUELTA * MICROPASO);

// PARAMETROS DEL MOTOR
const float VELOCIDAD_MAXIMA = 400.0;
const float ACELERACION = 200.0;
const float VELOCIDAD_HOMING = 200.0;

// Tiempo de espera en estaciones
const unsigned long TIEMPO_ESPERA_MS = 1500;

// MOTOR
AccelStepper motor(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);

// ESTADOS
enum EstadoSistema {
  BUSCAR_INICIO,
  AVANZAR_10CM,
  ESPERA_ESTACION,
  RETORNAR_INICIO,
  ESPERA_REINICIO
};

EstadoSistema estadoActual = BUSCAR_INICIO;

unsigned long marcaTiempo = 0;
int numeroPasoActual = 0;

// FINALES DE CARRERA
bool inicioActivado() {
  return digitalRead(PIN_SW_INICIO) == LOW;
}

bool finalActivado() {
  return digitalRead(PIN_SW_FINAL) == LOW;
}

void setup() {

  Serial.begin(115200);

  pinMode(PIN_SW_INICIO, INPUT_PULLUP);
  pinMode(PIN_SW_FINAL, INPUT_PULLUP);
  pinMode(PIN_ENABLE, OUTPUT);

  // LOW habilita DRV8825
  digitalWrite(PIN_ENABLE, LOW);

  motor.setMaxSpeed(VELOCIDAD_MAXIMA);
  motor.setAcceleration(ACELERACION);

  Serial.println(F("=============================="));
  Serial.println(F("SISTEMA DE MOTOR LINEAL"));
  Serial.println(F("=============================="));

  Serial.print(F("Pasos calculados para 10 cm: "));
  Serial.println(PASOS_10CM);

  Serial.println(F("Iniciando busqueda de origen..."));
}

void loop() {

  switch (estadoActual) {

    // HOMING
    case BUSCAR_INICIO:

      if (!inicioActivado()) {
        motor.setSpeed(-VELOCIDAD_HOMING);
        motor.runSpeed();

      } else {
        motor.setSpeed(0);
        motor.stop();
        motor.setCurrentPosition(0);
        numeroPasoActual = 0;
        Serial.println(F("[HOME] Origen detectado."));
        Serial.println(F("[HOME] Posicion: 0 cm"));
        marcaTiempo = millis();
        estadoActual = ESPERA_REINICIO;
      }

      break;

    // AVANCE 10 CM
    case AVANZAR_10CM:

      // Supervisar limite final
      if (finalActivado()) {
        motor.stop();
        Serial.println(F("[LIMITE] Final detectado."));
        Serial.println(F("[RETORNO] Iniciando retorno."));
        estadoActual = RETORNAR_INICIO;
        break;
      }

      if (motor.distanceToGo() != 0) {
        motor.run();

      } else {
        numeroPasoActual++;
        Serial.print(F("[POSICION] Avance completado: "));
        Serial.print(numeroPasoActual * 10);
        Serial.println(F(" cm"));
        marcaTiempo = millis();
        estadoActual = ESPERA_ESTACION;
      }

      break;

    // ESPERA EN ESTACION
    case ESPERA_ESTACION:

      if (millis() - marcaTiempo >= TIEMPO_ESPERA_MS) {

        if (finalActivado()) {
          Serial.println(F("[LIMITE] Final detectado."));
          Serial.println(F("[RETORNO] Iniciando retorno."));
          estadoActual = RETORNAR_INICIO;

        } else {

          motor.move(PASOS_10CM);

          Serial.println(F("[MOVIMIENTO] Avanzando 10 cm."));
          Serial.print(F("[POSICION] Estacion: "));
          Serial.print(numeroPasoActual * 10);
          Serial.println(F(" cm"));

          estadoActual = AVANZAR_10CM;
        }
      }

      break;

    // RETORNO A HOME
    case RETORNAR_INICIO:

      if (!inicioActivado()) {

        if (motor.currentPosition() > 0) {
          motor.moveTo(0);
          motor.run();

        } else {
          motor.setSpeed(-VELOCIDAD_HOMING);
          motor.runSpeed();
        }

      } else {
        motor.stop();
        motor.setCurrentPosition(0);
        numeroPasoActual = 0;
        Serial.println(F("[RETORNO] Origen alcanzado."));
        Serial.println(F("[POSICION] 0 cm"));
        marcaTiempo = millis();
        estadoActual = ESPERA_REINICIO;
      }

      break;

    // ESPERA PARA REINICIAR
    case ESPERA_REINICIO:

      if (millis() - marcaTiempo >= TIEMPO_ESPERA_MS) {
        motor.move(PASOS_10CM);
        Serial.println(F("[CICLO] Iniciando avance."));
        Serial.println(F("[MOVIMIENTO] Avanzando 10 cm."));
        estadoActual = AVANZAR_10CM;
      }

      break;
  }
}
