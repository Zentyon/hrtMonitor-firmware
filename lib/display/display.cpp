#include <Arduino.h>
#include "display.h"
#include "profile.h"
#include "sensor.h"
#include "app.h"
#include "pinout.h"

TFT_eSPI tft;

void displayInit()
{
    tft.init();
    tft.setRotation(1);
    clearScreen();
}

void clearScreen()
{
    tft.fillScreen(TFT_BLACK);
}

void drawTitle(const char* title)
{
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(title, 30, 10);
    tft.drawFastHLine(10, 28, 300, TFT_DARKGREY);
}

// Palomita (check) dibujada con líneas
void drawCheck(int x, int y, uint16_t color)
{
    tft.drawLine(x, y + 6, x + 4, y + 10, color);
    tft.drawLine(x + 4, y + 10, x + 11, y, color);
}

// Tache (cross) dibujado con líneas
void drawCross(int x, int y, uint16_t color)
{
    tft.drawLine(x, y, x + 9, y + 9, color);
    tft.drawLine(x + 9, y, x, y + 9, color);
}

void drawOption(int num, const char* text, int y, bool selected)
{
    tft.setTextSize(2);
    if (selected)
    {
        tft.setTextColor(TFT_BLACK, TFT_CYAN);
        tft.drawString("> " + String(num) + ". " + String(text), 20, y);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
    }
    else
    {
        tft.setTextColor(TFT_WHITE);
        tft.drawString("  " + String(num) + ". " + String(text), 20, y);
    }
}

void showSplash()
{
    clearScreen();
    tft.setTextSize(3);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("HRT Monitor", 50, 60);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("v1.0", 150, 100);
}

void showOnLang()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Idioma" : "Language");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("1. Espanol", 40, 50);
    tft.drawString("2. English", 40, 80);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString(lang == 'E' ? "Presione 1 o 2" : "Press 1 or 2", 100, 140);
}

void showOnAge()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Edad" : "Age");
    tft.setTextSize(3);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("_ _ _", 80, 60);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString(lang == 'E' ? "0-9: numero  A: ok  C: borrar" : "0-9: number  A: ok  C: del", 40, 120);
}

void updateAgeDisplay()
{
    tft.fillRect(80, 55, 120, 40, TFT_BLACK);
    tft.setTextSize(3);
    tft.setTextColor(TFT_CYAN);
    String disp = "";
    for (int i = 0; i < ageIdx; i++)
        disp += String(ageInput[i]) + " ";
    for (int i = ageIdx; i < 3; i++)
        disp += "_ ";
    tft.drawString(disp, 80, 60);
}

void showOnGender()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Genero" : "Gender");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(lang == 'E' ? "* ♀ Femenino" : "* ♀ Female", 40, 60);
    tft.drawString(lang == 'E' ? "# ♂ Masculino" : "# ♂ Male", 40, 90);
}

void showOnExercise()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Actividad fisica" : "Physical activity");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(lang == 'E' ? "0 Ninguno" : "0 None", 30, 34);
    tft.drawString("1 " + String(sportNames[lang=='E'?0:1][0]), 30, 48);
    tft.drawString("2 " + String(sportNames[lang=='E'?0:1][1]), 30, 62);
    tft.drawString("3 " + String(sportNames[lang=='E'?0:1][2]), 170, 48);
    tft.drawString("4 " + String(sportNames[lang=='E'?0:1][3]), 170, 62);
    tft.drawString("5 " + String(sportNames[lang=='E'?0:1][4]), 30, 76);
    tft.drawString("6 " + String(sportNames[lang=='E'?0:1][5]), 170, 76);
    tft.drawString("7 " + String(sportNames[lang=='E'?0:1][6]), 30, 90);
    tft.drawString("8 " + String(sportNames[lang=='E'?0:1][7]), 170, 90);
    tft.drawString(lang == 'E' ? "9 Otro" : "9 Other", 30, 104);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("B. " + String(lang == 'E' ? "Volver" : "Back"), 120, 150);
}

void showOnExerciseIntensity()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Intensidad" : "Intensity");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(lang == 'E' ? "1. Baja" : "1. Low", 40, 55);
    tft.drawString(lang == 'E' ? "2. Media" : "2. Medium", 40, 85);
    tft.drawString(lang == 'E' ? "3. Alta" : "3. High", 40, 115);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("B. " + String(lang == 'E' ? "Volver" : "Back"), 120, 160);
}

void showOnConditions()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Enf. Cardiacas" : "Heart Cond.");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("A", 40, 70);
    drawCheck(58, 70, TFT_GREEN);
    tft.drawString(lang == 'E' ? " Si" : " Yes", 72, 70);
    tft.drawString("B", 40, 100);
    drawCross(58, 100, TFT_RED);
    tft.drawString(lang == 'E' ? " No" : " No", 72, 100);
}

void showOnConditionType()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Tipo Enfermedad" : "Condition Type");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    const char* dn[2][7] = {
        {"1. Diabetes","2. Hipertension","3. Hipertiroidismo","4. Hipotiroidismo","5. Anemia","6. Asma controlada","7. Insuf. cardiaca"},
        {"1. Diabetes","2. Hypertension","3. Hyperthyroidism","4. Hypothyroidism","5. Anemia","6. Controlled asthma","7. Heart failure"}
    };
    int li = (lang == 'E') ? 0 : 1;
    tft.drawString(dn[li][0], 30, 38);
    tft.drawString(dn[li][1], 30, 55);
    tft.drawString(dn[li][2], 30, 72);
    tft.drawString(dn[li][3], 30, 89);
    tft.drawString(dn[li][4], 30, 106);
    tft.drawString(dn[li][5], 30, 123);
    tft.drawString(dn[li][6], 30, 140);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("B. " + String(lang == 'E' ? "Volver" : "Back"), 120, 170);
}

void showMenu()
{
    clearScreen();
    drawTitle("HRT Monitor");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(lang == 'E' ? "1. Medir" : "1. Measure", 40, 60);
    tft.drawString(lang == 'E' ? "2. Config" : "2. Settings", 40, 90);
}

void showConfig()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Configuracion" : "Settings");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(lang == 'E' ? "1. Idioma" : "1. Language", 40, 50);
    tft.drawString(lang == 'E' ? "2. Mi perfil" : "2. My profile", 40, 75);
    tft.drawString(lang == 'E' ? "3. Reset" : "3. Reset", 40, 100);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("B. " + String(lang == 'E' ? "Volver" : "Back"), 120, 140);
}

void showConfigLang()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Idioma" : "Language");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(lang == 'E' ? "1. Espanol" : "1. Spanish", 40, 60);
    tft.drawString("2. English", 40, 90);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("B. " + String(lang == 'E' ? "Volver" : "Back"), 120, 140);
}

void showConfigProfile()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Mi Perfil" : "My Profile");
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);

    String langStr = (lang == 'E') ? "Espanol" : "English";
    tft.drawString((lang == 'E' ? "Idioma: " : "Language: ") + langStr, 20, 45);
    tft.drawString((lang == 'E' ? "Edad: " : "Age: ") + String(age), 20, 70);

    String genStr;
    if (gender == 'M')      genStr = (lang == 'E' ? "Masculino" : "Male");
    else if (gender == 'F') genStr = (lang == 'E' ? "Femenino" : "Female");
    else                    genStr = (lang == 'E' ? "Sin definir" : "Not set");
    tft.drawString((lang == 'E' ? "Genero: " : "Gender: ") + genStr, 20, 95);

    String exDisplay;
    if (exerciseType == 0)
        exDisplay = (lang == 'E' ? "Ninguno" : "None");
    else if (exerciseType == 1)
        exDisplay = String(sportNames[lang == 'E' ? 0 : 1][sportSel]);
    else
    {
        String lvl[] = {"Baja", "Media", "Alta"};
        String lvlEn[] = {"Low", "Medium", "High"};
        exDisplay = (lang == 'E' ? "Otro: " : "Other: ") + (lang == 'E' ? lvl[exerciseLevel] : lvlEn[exerciseLevel]);
    }
    tft.drawString((lang == 'E' ? "Actividad: " : "Activity: ") + exDisplay, 20, 120);

    const char* condNames[2][8] = {
        {"", "Diabetes", "Hipertension", "Hipertiroidismo", "Hipotiroidismo", "Anemia", "Asma controlada", "Insuf. cardiaca"},
        {"", "Diabetes", "Hypertension", "Hyperthyroidism", "Hypothyroidism", "Anemia", "Controlled asthma", "Heart failure"}
    };
    int ci = (lang == 'E') ? 0 : 1;
    String condStr = (conditionSel > 0) ? String(condNames[ci][conditionSel]) : (lang == 'E' ? "Ninguna" : "None");
    tft.drawString((lang == 'E' ? "Enf. Card.: " : "Heart Cond: ") + condStr, 20, 145);

    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("A. " + String(lang == 'E' ? "Volver" : "Back"), 120, 170);
}

void showConfigReset()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Reset" : "Reset");
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.drawString(lang == 'E' ? "Borrar todos" : "Delete all", 40, 60);
    tft.drawString(lang == 'E' ? "los datos?" : "data?", 40, 85);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("A", 40, 120);
    drawCheck(58, 120, TFT_GREEN);
    tft.drawString(lang == 'E' ? " Confirmar" : " Confirm", 72, 120);
    tft.drawString("B", 40, 145);
    drawCross(58, 145, TFT_RED);
    tft.drawString(lang == 'E' ? " Cancelar" : " Cancel", 72, 145);
}

void showCalibrate()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Calibrando..." : "Calibrating...");
    tft.setTextSize(4);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("5", 150, 70);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString(lang == 'E' ? "Mantenga la muñequera" : "Keep wristband on", 70, 130);
    tft.drawString(lang == 'E' ? "en la muñeca" : "on wrist", 100, 145);
}

void updateCalibrateDisplay(int secondsLeft)
{
    tft.fillRect(135, 65, 50, 45, TFT_BLACK);
    tft.setTextSize(4);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(String(secondsLeft), 135, 70);
}

void showCountdown()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Preparando..." : "Preparing...");
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(lang == 'E' ? "Coloque la muñequera" : "Place wristband", 50, 60);
    tft.drawString(lang == 'E' ? "en la muñeca" : "on wrist", 70, 85);
    tft.setTextSize(5);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("3", 150, 110);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString(lang == 'E' ? "Mantenga firme" : "Keep still", 80, 170);
}

void updateCountdownDisplay(int secondsLeft)
{
    tft.fillRect(130, 105, 60, 55, TFT_BLACK);
    tft.setTextSize(5);
    tft.setTextColor(TFT_CYAN);
    tft.drawString(String(secondsLeft), 150, 110);
}

void showMeasure()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Midiendo..." : "Measuring...");
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("Prom  Max  Min", 20, 35);
}

void updateMeasureDisplay()
{
    unsigned long elapsed = millis() - measureStart;
    if (elapsed >= MEASURE_DURATION) return;

    int remaining = (MEASURE_DURATION - elapsed) / 1000;
    int mins = remaining / 60;
    int secs = remaining % 60;

    // BPM grande
    tft.fillRect(20, 50, 280, 50, TFT_BLACK);
    tft.setTextSize(4);
    if (fingerPresent)
    {
        tft.setTextColor(TFT_CYAN);
        tft.drawString(String(avgBpm) + " BPM", 40, 60);
    }
    else
    {
        tft.setTextColor(TFT_DARKGREY);
        tft.drawString("--", 120, 60);
    }

    // Tiempo restante
    tft.fillRect(240, 35, 80, 15, TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    String timeStr = String(mins) + ":" + (secs < 10 ? "0" : "") + String(secs);
    tft.drawString(timeStr, 260, 35);

    // Barra de progreso
    int barWidth = map(elapsed, 0, MEASURE_DURATION, 0, 280);
    tft.drawRect(20, 110, 280, 12, TFT_DARKGREY);
    tft.fillRect(20, 110, barWidth, 12, TFT_CYAN);

    // Stats
    tft.fillRect(20, 130, 280, 40, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("P:" + String(avgBpm) + " X:" + String(maxBpm) + " N:" + String(minBpm), 20, 135);

    // LEDs - Rango dinámico según perfil
    if (fingerPresent && avgBpm > 0)
    {
        int rLo, rHi;
        computeRange(rLo, rHi);
        int band = (rHi - rLo) * 15 / 100;
        if (band < 8) band = 8;

        digitalWrite(PIN_LED_VERDE, (avgBpm > rLo + band && avgBpm < rHi - band) ? HIGH : LOW);
        digitalWrite(PIN_LED_AMARILLO,
            ((avgBpm >= rLo && avgBpm <= rLo + band) ||
             (avgBpm >= rHi - band && avgBpm <= rHi)) ? HIGH : LOW);
        digitalWrite(PIN_LED_ROJO, (avgBpm < rLo || avgBpm > rHi) ? HIGH : LOW);
    }
    else
    {
        digitalWrite(PIN_LED_VERDE, LOW);
        digitalWrite(PIN_LED_AMARILLO, LOW);
        digitalWrite(PIN_LED_ROJO, LOW);
    }
}

void showResults()
{
    clearScreen();
    drawTitle(lang == 'E' ? "Resultados" : "Results");

    if (bpmCount == 0)
    {
        tft.setTextSize(2);
        tft.setTextColor(TFT_YELLOW);
        tft.drawString(lang == 'E' ? "Sin datos" : "No data", 20, 60);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(lang == 'E' ? "No se detecto pulso" : "No pulse detected", 20, 95);
        tft.drawString(lang == 'E' ? "Repita la medicion" : "Repeat measurement", 20, 115);
        digitalWrite(PIN_LED_VERDE, LOW);
        digitalWrite(PIN_LED_AMARILLO, LOW);
        digitalWrite(PIN_LED_ROJO, HIGH);
        tft.setTextSize(1);
        tft.setTextColor(TFT_DARKGREY);
        tft.drawString("A. " + String(lang == 'E' ? "Volver" : "Back"), 120, 180);
        return;
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.drawString((lang == 'E' ? "Promedio: " : "Average: ") + String(avgBpm) + " BPM", 20, 45);
    tft.drawString((lang == 'E' ? "Maximo: " : "Max: ") + String(maxBpm) + " BPM", 20, 70);
    tft.drawString((lang == 'E' ? "Minimo: " : "Min: ") + String(minBpm) + " BPM", 20, 95);

    int totalSec = MEASURE_DURATION / 1000;
    tft.drawString((lang == 'E' ? "Duracion: " : "Duration: ") + String(totalSec / 60) + " min", 20, 120);

    int rLo, rHi;
    computeRange(rLo, rHi);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString((lang == 'E' ? "Tu rango: " : "Your range: ") + String(rLo) + "-" + String(rHi) + " BPM", 20, 148);

    // Mantener LED activo según último BPM
    if (avgBpm > 0)
    {
        int band = (rHi - rLo) * 15 / 100;
        if (band < 8) band = 8;
        digitalWrite(PIN_LED_VERDE, (avgBpm > rLo + band && avgBpm < rHi - band) ? HIGH : LOW);
        digitalWrite(PIN_LED_AMARILLO,
            ((avgBpm >= rLo && avgBpm <= rLo + band) ||
             (avgBpm >= rHi - band && avgBpm <= rHi)) ? HIGH : LOW);
        digitalWrite(PIN_LED_ROJO, (avgBpm < rLo || avgBpm > rHi) ? HIGH : LOW);
    }

    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("A. " + String(lang == 'E' ? "Volver" : "Back"), 120, 180);
}
