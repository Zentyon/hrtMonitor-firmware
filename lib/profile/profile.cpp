#include <Arduino.h>
#include <Preferences.h>
#include "profile.h"

static Preferences prefs;

char lang = 'E';       // 'E'=ESP, 'I'=ENG
int age = 0;
char gender = ' ';     // 'M'=masculino, 'F'=femenino
int exerciseType = 0;   // 0=Ninguno, 1=Deporte, 2=Otro(intensidad)
int sportSel = 0;       // 0-7 indice de tableSport
int exerciseLevel = 0;  // 0=Baja, 1=Media, 2=Alta (cuando exerciseType==2)
int conditionSel = 0;  // 0=Ninguna, 1-7=Enfermedad
bool conditions = false; // derivado de conditionSel
char ageInput[4] = {0};
int ageIdx = 0;

// Hoja1: Edad x Genero (en reposo) [fila][0]=Mujer, [1]=Hombre
const Range tableAge[9][2] = {
    {{100,160},{100,160}},  // 0-1
    {{90,150},{90,150}},    // 1-3
    {{80,140},{80,140}},    // 4-5
    {{70,120},{70,120}},    // 6-12
    {{60,100},{60,100}},    // 13-17
    {{60,95},{55,90}},      // 18-25
    {{60,95},{55,90}},      // 26-35
    {{60,100},{60,95}},     // 36-45
    {{60,100},{60,95}}      // 46-55
};

// Hoja3: Actividad [intensidad-1][0]=Mujer, [1]=Hombre
const Range tableExercise[3][2] = {
    {{52,72},{48,68}},   // 1: Baja (promedio suave)
    {{58,78},{55,75}},   // 2: Media (Gimnasio 4-6 dias)
    {{43,58},{40,55}}    // 3: Alta (Alto rendimiento)
};

// Hoja4: Enfermedad [idx-1][0]=Mujer, [1]=Hombre
const Range tableDisease[7][2] = {
    {{70,100},{65,95}},   // 1: Diabetes
    {{60,95},{60,90}},    // 2: Hipertension
    {{85,115},{80,110}},  // 3: Hipertiroidismo
    {{55,75},{50,70}},    // 4: Hipotiroidismo
    {{80,105},{75,100}},  // 5: Anemia
    {{65,100},{60,95}},   // 6: Asma controlada
    {{75,100},{70,95}}    // 7: Insuf. cardiaca
};

// Hoja3: Deporte/Actividad (sin alto rendimiento) [0]=Mujer, [1]=Hombre
const Range tableSport[8][2] = {
    {{53,73},{50,70}},    // 1: Correr (Running)
    {{48,68},{45,65}},    // 2: Ciclismo (Cycling)
    {{48,68},{45,65}},    // 3: Natacion (Swimming)
    {{53,73},{50,70}},    // 4: Futbol (Soccer)
    {{53,73},{50,70}},    // 5: Basquetbol (Basketball)
    {{58,78},{55,75}},    // 6: Gimnasio (Gym 4-6 dias)
    {{48,68},{45,65}},    // 7: Atletismo (Athletics)
    {{53,73},{50,70}}     // 8: Artes marciales (Martial arts)
};

// Nombres de deportes [idioma][indice 0-7]
const char* sportNames[2][8] = {
    {"Correr","Ciclismo","Natacion","Futbol","Basquetbol","Gimnasio","Atletismo","Artes marciales"},
    {"Running","Cycling","Swimming","Soccer","Basketball","Gym","Athletics","Martial arts"}
};

int ageToRow(int a) {
    if (a <= 1)  return 0;
    if (a <= 3)  return 1;
    if (a <= 5)  return 2;
    if (a <= 12) return 3;
    if (a <= 17) return 4;
    if (a <= 25) return 5;
    if (a <= 35) return 6;
    if (a <= 45) return 7;
    return 8;
}

void computeRange(int &lo, int &hi)
{
    int g = (gender == 'M') ? 1 : 0;

    if (conditionSel > 0)
    {
        Range r = tableDisease[conditionSel - 1][g];
        lo = r.lo; hi = r.hi;
    }
    else if (exerciseType == 1)
    {
        Range r = tableSport[sportSel][g];
        lo = r.lo; hi = r.hi;
    }
    else if (exerciseType == 2)
    {
        Range r = tableExercise[exerciseLevel][g];
        lo = r.lo; hi = r.hi;
    }
    else
    {
        int row = ageToRow(age);
        lo = tableAge[row][g].lo;
        hi = tableAge[row][g].hi;
    }
}

void saveProfile()
{
    prefs.begin("profile", false);
    prefs.putBool("configured", true);
    prefs.putChar("lang", lang);
    prefs.putUInt("age", age);
    prefs.putChar("gender", gender);
    prefs.putUInt("exerciseType", exerciseType);
    prefs.putUInt("sportSel", sportSel);
    prefs.putUInt("exerciseLevel", exerciseLevel);
    prefs.putUInt("conditionSel", conditionSel);
    prefs.end();
}

bool loadProfile()
{
    prefs.begin("profile", true);
    bool configured = prefs.getBool("configured", false);
    if (configured)
    {
        lang = prefs.getChar("lang", 'E');
        age = prefs.getUInt("age", 0);
        gender = prefs.getChar("gender", ' ');
        exerciseType = prefs.getUInt("exerciseType", 0);
        sportSel = prefs.getUInt("sportSel", 0);
        exerciseLevel = prefs.getUInt("exerciseLevel", 0);
        conditionSel = prefs.getUInt("conditionSel", 0);
        conditions = (conditionSel > 0);
    }
    prefs.end();
    return configured;
}

void resetProfile()
{
    prefs.begin("profile", false);
    prefs.clear();
    prefs.end();
}
