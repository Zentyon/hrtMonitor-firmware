#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Pantallas TFT (ILI9341 320x240, rotacion 1).
// Lee el perfil (profile.h), la medicion (sensor.h) y los
// tiempos de estado (app.h). No escribe estado.

extern TFT_eSPI tft;

void displayInit();
void clearScreen();
void drawTitle(const char* title);
void drawCheck(int x, int y, uint16_t color);
void drawCross(int x, int y, uint16_t color);
void drawOption(int num, const char* text, int y, bool selected);

void showSplash();
void showSensorError();
void showOnLang();
void showOnAge();
void updateAgeDisplay();
void showOnGender();
void showOnExercise();
void showOnExerciseIntensity();
void showOnConditions();
void showOnConditionType();
void showMenu();
void showConfig();
void showConfigLang();
void showConfigProfile();
void showConfigReset();
void showCalibrate();
void updateCalibrateDisplay(int secondsLeft);
void showCountdown();
void updateCountdownDisplay(int secondsLeft);
void showMeasure();
void updateMeasureDisplay();
void showResults();
