#pragma once
#include <Arduino.h>

// Filtro PPG + deteccion de latido (MAX30102) + SpO2 estimado.
// El muestreo corre en una task dedicada (core 0); el loop solo lee
// los valores compartidos via sensorReadMeasure() / sensorBeatLost().
// El estado compartido se publica bajo un portMUX (critica corta, solo
// calculo; las lecturas I2C quedan fuera).

#define RATE_SIZE 16

// Estado compartido. Los campos que escribe la task del sensor son
// volatile porque los lee el core 1 (app/display). Para leer varios
// campos de forma coherente usar sensorReadMeasure().
extern volatile byte rates[RATE_SIZE];
extern volatile byte rateSpot;

extern volatile long lastBeat;
extern volatile float bpm;
extern volatile int avgBpm;
extern volatile int maxBpm;
extern volatile int minBpm;
extern volatile int bpmCount;

extern volatile bool normInit;
extern volatile bool schmittState;
extern volatile bool wristPresent;

// true si el MAX30102 respondio en sensorInit(). En false el equipo
// arranca en modo demo (sin lectura PPG) en vez de colgarse.
extern volatile bool sensorAvailable;

// SpO2 estimado por ratio-de-ratios (rojo/IR). SOLO valido si
// spo2Valid. Curva de Maxim por defecto: VALIDAR contra oximetro
// de referencia antes de uso real (ver sensor.cpp).
extern volatile int spo2;          // 0-100
extern volatile bool spo2Valid;
extern volatile float perfusion;   // indice de perfusion IR (rmsAC/DC)

// Ganancia DC del filtro (app la ajusta durante warmup).
extern const float ALPHA_DC;

struct SensorMeasure {
    int  avgBpm;
    int  maxBpm;
    int  minBpm;
    int  spo2;         // valido solo si spo2Valid
    bool spo2Valid;
    bool wristPresent;
};

// Devuelve true si el sensor quedo operativo. Nunca bloquea:
// reintenta a 400kHz y luego a 100kHz antes de rendirse.
bool sensorInit();
void processSensor();

// Copia coherente (atomica frente a la task) de los valores de medicion.
void sensorReadMeasure(SensorMeasure &m);

// true si hay sensor pero pasaron >3 s sin latido (o ninguno aun).
// Sirve para mostrar "coloque la muñequera" en display.
bool sensorBeatLost();

// Reinicia TODO el estado por-medicion (buffer de BPM, SpO2, filtros).
// sensorBeginMeasurement() ademas deja el filtro en modo calibracion.
void sensorBeginMeasurement();
// Cierra la calibracion: limpia acumuladores y arranca la medicion real.
void sensorEndCalibration();
