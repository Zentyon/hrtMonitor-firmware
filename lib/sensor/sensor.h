#pragma once
#include <Arduino.h>

// Filtro PPG + deteccion de latido (MAX30102).
// processSensor() debe llamarse en cada iteracion del loop
// durante COUNTDOWN / CALIBRATE / MEASURE.

#define RATE_SIZE 8

extern byte rates[RATE_SIZE];
extern byte rateSpot;

extern long lastBeat;
extern float bpm;
extern int avgBpm;
extern int maxBpm;
extern int minBpm;
extern int bpmCount;

extern bool normInit;
extern bool schmittState;
extern bool fingerPresent;

// Ganancia DC del filtro (app la ajusta durante warmup).
extern const float ALPHA_DC;

void sensorInit();
void processSensor();
