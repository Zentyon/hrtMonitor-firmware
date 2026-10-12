#pragma once
#include <Arduino.h>

// Maquina de estados + setup/loop de la app.
// Los handlers son internos de app.cpp; aquí solo vive
// el estado compartido con sensor/display.

enum State {
    ST_SPLASH,
    ST_ON_LANG,
    ST_ON_AGE,
    ST_ON_GENDER,
    ST_ON_EXERCISE,
    ST_ON_EXERCISE_INTENSITY,
    ST_ON_CONDITIONS,
    ST_ON_CONDITION_TYPE,
    ST_MENU,
    ST_CONFIG,
    ST_CONFIG_LANG,
    ST_CONFIG_PROFILE,
    ST_CONFIG_RESET,
    ST_CALIBRATE,
    ST_COUNTDOWN,
    ST_MEASURE,
    ST_RESULTS
};

extern State state;

extern unsigned long stateStart;
extern unsigned long measureStart;

extern bool isCalibrating;
extern float currentAlphaDC;

#define COUNTDOWN_DURATION 3000    // 3 segundos
#define CALIBRATE_DURATION 5000    // 5 segundos
#define MEASURE_DURATION 90000     // 90 segundos (1:30)

void appSetup();
void appLoop();
