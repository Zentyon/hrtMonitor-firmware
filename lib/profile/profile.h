#pragma once

struct Range { int lo; int hi; };

// Perfil de usuario (persistido en Preferences)
extern char lang;       // 'E'=ESP, 'I'=ENG
extern int age;
extern char gender;     // 'M'=masculino, 'F'=femenino
extern int exerciseType;   // 0=Ninguno, 1=Deporte, 2=Otro(intensidad)
extern int sportSel;       // 0-7 indice de tableSport
extern int exerciseLevel;  // 0=Baja, 1=Media, 2=Alta (cuando exerciseType==2)
extern int conditionSel;   // 0=Ninguna, 1-7=Enfermedad
extern bool conditions;    // derivado de conditionSel
extern char ageInput[4];
extern int ageIdx;

// Tablas de rango BPM (reposo / actividad / enfermedad / deporte)
extern const Range tableAge[9][2];
extern const Range tableExercise[3][2];
extern const Range tableDisease[7][2];
extern const Range tableSport[8][2];
extern const char* sportNames[2][8];

int ageToRow(int a);
void computeRange(int &lo, int &hi);
void saveProfile();
bool loadProfile();
void resetProfile();
