#include <Arduino.h>
#include "app.h"
#include "profile.h"
#include "keypad.h"
#include "audio.h"
#include "sensor.h"
#include "display.h"
#include "ble.h"
#include "pinout.h"

State state = ST_SPLASH;

unsigned long stateStart = 0;
unsigned long measureStart = 0;
bool isCalibrating = true;
float currentAlphaDC = 0.05f;

static void handleOnboarding(char key)
{
    switch (state)
    {
        case ST_ON_LANG:
            if (key == '1') { lang = 'E'; state = ST_ON_AGE; showOnAge(); beep(800, 50); }
            else if (key == '2') { lang = 'I'; state = ST_ON_AGE; showOnAge(); beep(800, 50); }
            break;

        case ST_ON_AGE:
            if (key >= '0' && key <= '9' && ageIdx < 3)
            {
                ageInput[ageIdx++] = key;
                updateAgeDisplay();
                beep(600, 30);
            }
            else if (key == 'C' && ageIdx > 0)
            {
                ageIdx--;
                ageInput[ageIdx] = 0;
                updateAgeDisplay();
                beep(500, 30);
            }
            else if (key == 'A' && ageIdx > 0)
            {
                age = 0;
                for (int i = 0; i < ageIdx; i++)
                    age = age * 10 + (ageInput[i] - '0');
                state = ST_ON_GENDER;
                showOnGender();
                beep(800, 50);
            }
            else if (key == 'B')
            {
                state = ST_ON_LANG;
                showOnLang();
                beep(500, 30);
            }
            break;

        case ST_ON_GENDER:
            if (key == '*') { gender = 'F'; state = ST_ON_EXERCISE; showOnExercise(); beep(800, 50); }
            else if (key == '#') { gender = 'M'; state = ST_ON_EXERCISE; showOnExercise(); beep(800, 50); }
            else if (key == 'B')
            {
                state = ST_ON_AGE;
                showOnAge();
                beep(500, 30);
            }
            break;

        case ST_ON_EXERCISE:
            if (key == '0') { exerciseType = 0; state = ST_ON_CONDITIONS; showOnConditions(); beep(800, 50); }
            else if (key >= '1' && key <= '8')
            {
                exerciseType = 1;
                sportSel = key - '0' - 1;
                state = ST_ON_CONDITIONS;
                showOnConditions();
                beep(800, 50);
            }
            else if (key == '9')
            {
                exerciseType = 2;
                state = ST_ON_EXERCISE_INTENSITY;
                showOnExerciseIntensity();
                beep(800, 50);
            }
            else if (key == 'B')
            {
                state = ST_ON_GENDER;
                showOnGender();
                beep(500, 30);
            }
            break;

        case ST_ON_EXERCISE_INTENSITY:
            if (key >= '1' && key <= '3')
            {
                exerciseLevel = key - '0' - 1;
                state = ST_ON_CONDITIONS;
                showOnConditions();
                beep(800, 50);
            }
            else if (key == 'B')
            {
                state = ST_ON_EXERCISE;
                showOnExercise();
                beep(800, 50);
            }
            break;

        case ST_ON_CONDITIONS:
            if (key == 'A') { conditions = true; state = ST_ON_CONDITION_TYPE; showOnConditionType(); beep(800, 50); }
            else if (key == 'B') { conditions = false; conditionSel = 0; saveProfile(); state = ST_MENU; showMenu(); beep(1000, 100); }
            break;

        case ST_ON_CONDITION_TYPE:
            if (key >= '1' && key <= '7')
            {
                conditionSel = key - '0';
                conditions = true;
                saveProfile();
                state = ST_MENU;
                showMenu();
                beep(1000, 100);
            }
            else if (key == 'B')
            {
                state = ST_ON_CONDITIONS;
                showOnConditions();
                beep(800, 50);
            }
            break;

        default:
            break;
    }
}

static void handleMenu(char key)
{
    if (key == '1')
    {
        state = ST_COUNTDOWN;
        stateStart = millis();
        normInit = false;
        lastBeat = 0;
        bpm = 0;
        avgBpm = 0;
        maxBpm = 0;
        minBpm = 999;
        bpmCount = 0;
        schmittState = false;
        isCalibrating = true;
        currentAlphaDC = 0.05f;
        showCountdown();
        beep(800, 50);
    }
    else if (key == '2')
    {
        state = ST_CONFIG;
        showConfig();
        beep(800, 50);
    }
}

static void handleConfig(char key)
{
    if (key == '0' || key == 'B')
    {
        state = ST_MENU;
        showMenu();
        beep(800, 50);
    }
    else if (key == '1')
    {
        state = ST_CONFIG_LANG;
        showConfigLang();
        beep(800, 50);
    }
    else if (key == '2')
    {
        state = ST_CONFIG_PROFILE;
        showConfigProfile();
        beep(800, 50);
    }
    else if (key == '3')
    {
        state = ST_CONFIG_RESET;
        showConfigReset();
        beep(800, 50);
    }
}

static void handleConfigLang(char key)
{
    if (key == '0' || key == 'B')
    {
        state = ST_CONFIG;
        showConfig();
        beep(800, 50);
    }
    else if (key == '1')
    {
        lang = 'E';
        saveProfile();
        state = ST_CONFIG;
        showConfig();
        beep(800, 50);
    }
    else if (key == '2')
    {
        lang = 'I';
        saveProfile();
        state = ST_CONFIG;
        showConfig();
        beep(800, 50);
    }
}

static void handleConfigProfile(char key)
{
    if (key == 'A' || key == 'B')
    {
        state = ST_CONFIG;
        showConfig();
        beep(800, 50);
    }
}

static void handleConfigReset(char key)
{
    if (key == 'A')
    {
        resetProfile();
        beep(1200, 200);
        delay(500);
        ESP.restart();
    }
    else if (key == 'B')
    {
        state = ST_CONFIG;
        showConfig();
        beep(800, 50);
    }
}

static void handleResults(char key)
{
    if (key == 'A')
    {
        state = ST_MENU;
        showMenu();
        beep(800, 50);
    }
}

void appSetup()
{
    Serial.begin(115200);

    pinMode(PIN_LED_VERDE, OUTPUT);
    pinMode(PIN_LED_AMARILLO, OUTPUT);
    pinMode(PIN_LED_ROJO, OUTPUT);

    // Test LEDs: Verde -> Amarillo -> Rojo -> Todos -> Apagar
    digitalWrite(PIN_LED_VERDE, HIGH); delay(300);
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_AMARILLO, HIGH); delay(300);
    digitalWrite(PIN_LED_AMARILLO, LOW);
    digitalWrite(PIN_LED_ROJO, HIGH); delay(300);
    digitalWrite(PIN_LED_ROJO, LOW);
    digitalWrite(PIN_LED_VERDE, HIGH);
    digitalWrite(PIN_LED_AMARILLO, HIGH);
    digitalWrite(PIN_LED_ROJO, HIGH); delay(500);
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_AMARILLO, LOW);
    digitalWrite(PIN_LED_ROJO, LOW);

    sensorInit();
    displayInit();
    initKeypad();
    initAudio();
    bleInit();

    // Splash
    stateStart = millis();
    showSplash();
    beep(1000, 150);
    delay(100);
    beep(1500, 150);
    delay(1800);

    // Cargar perfil
    if (loadProfile())
    {
        state = ST_MENU;
        showMenu();
    }
    else
    {
        state = ST_ON_LANG;
        showOnLang();
    }
}

void appLoop()
{
    char key = getKey();

    if (key)
    {
        Serial.print("Key pressed: ");
        Serial.println(key);

        beep(600, 20);

        switch (state)
        {
            case ST_ON_LANG:
            case ST_ON_AGE:
            case ST_ON_GENDER:
            case ST_ON_EXERCISE:
            case ST_ON_EXERCISE_INTENSITY:
            case ST_ON_CONDITIONS:
            case ST_ON_CONDITION_TYPE:
                handleOnboarding(key);
                break;

            case ST_MENU:
                handleMenu(key);
                break;

            case ST_CONFIG:
                handleConfig(key);
                break;

            case ST_CONFIG_LANG:
                handleConfigLang(key);
                break;

            case ST_CONFIG_PROFILE:
                handleConfigProfile(key);
                break;

            case ST_CONFIG_RESET:
                handleConfigReset(key);
                break;

            case ST_RESULTS:
                handleResults(key);
                break;

            default:
                break;
        }
    }

    // --- Estado: SPLASH ---
    if (state == ST_SPLASH)
    {
        if (millis() - stateStart > 2000)
        {
            if (loadProfile())
            {
                state = ST_MENU;
                showMenu();
            }
            else
            {
                state = ST_ON_LANG;
                showOnLang();
            }
        }
    }

    // --- Estado: COUNTDOWN ---
    if (state == ST_COUNTDOWN)
    {
        processSensor();

        unsigned long elapsed = millis() - stateStart;
        int remaining = 3 - (elapsed / 1000);
        if (remaining < 0) remaining = 0;
        updateCountdownDisplay(remaining);

        if (elapsed >= COUNTDOWN_DURATION)
        {
            state = ST_CALIBRATE;
            stateStart = millis();
            showCalibrate();
            beep(1000, 100);
        }
    }

    // --- Estado: CALIBRATE ---
    if (state == ST_CALIBRATE)
    {
        processSensor();

        unsigned long elapsed = millis() - stateStart;
        int remaining = 5 - (elapsed / 1000);
        if (remaining < 0) remaining = 0;
        updateCalibrateDisplay(remaining);

        // LEDs durante calibración
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

        if (elapsed >= CALIBRATE_DURATION)
        {
            isCalibrating = false;
            currentAlphaDC = 0.1f;
            rateSpot = 0;
            memset(rates, 0, sizeof(rates));
            avgBpm = 0;
            bpmCount = 0;
            state = ST_MEASURE;
            measureStart = millis();
            showMeasure();
            beep(1000, 100);
        }
    }

    // --- Estado: MEASURE ---
    if (state == ST_MEASURE)
    {
        processSensor();
        bleUpdate(avgBpm);

        unsigned long elapsed = millis() - measureStart;
        const unsigned long WARMUP_MS = 10000;  // 10s de ajuste

        // Ramp-down: alpha DC baja de 0.05 → ALPHA_DC en 10s (lineal)
        if (elapsed < WARMUP_MS)
        {
            float progress = (float)elapsed / WARMUP_MS;
            currentAlphaDC = 0.05f - (0.05f - ALPHA_DC) * progress;
            if (currentAlphaDC < ALPHA_DC) currentAlphaDC = ALPHA_DC;
        }
        else if (currentAlphaDC < ALPHA_DC)
        {
            currentAlphaDC = ALPHA_DC;
        }

        // Display: warmup muestra "Ajustando...", después muestra normal
        if (elapsed >= WARMUP_MS)
        {
            updateMeasureDisplay();
        }
        else
        {
            // Warmup visual: BPM + tiempo + barra, pero texto "Ajustando..." y LEDs apagados
            int remaining = (MEASURE_DURATION - elapsed) / 1000;
            int mins = remaining / 60;
            int secs = remaining % 60;

            tft.fillRect(20, 50, 280, 50, TFT_BLACK);
            tft.setTextSize(4);
            if (fingerPresent && avgBpm > 0)
            {
                tft.setTextColor(TFT_CYAN);
                tft.drawString(String(avgBpm) + " BPM", 40, 60);
            }
            else
            {
                tft.setTextColor(TFT_DARKGREY);
                tft.drawString("--", 120, 60);
            }

            tft.fillRect(240, 35, 80, 15, TFT_BLACK);
            tft.setTextSize(1);
            tft.setTextColor(TFT_WHITE);
            String timeStr = String(mins) + ":" + (secs < 10 ? "0" : "") + String(secs);
            tft.drawString(timeStr, 260, 35);

            int barWidth = map(elapsed, 0, MEASURE_DURATION, 0, 280);
            tft.drawRect(20, 110, 280, 12, TFT_DARKGREY);
            tft.fillRect(20, 110, barWidth, 12, TFT_CYAN);

            tft.fillRect(20, 130, 280, 40, TFT_BLACK);
            tft.setTextSize(1);
            tft.setTextColor(TFT_DARKGREY);
            tft.drawString(lang == 'E' ? "Ajustando..." : "Adjusting...", 100, 140);

            digitalWrite(PIN_LED_VERDE, LOW);
            digitalWrite(PIN_LED_AMARILLO, LOW);
            digitalWrite(PIN_LED_ROJO, LOW);
        }

        if (elapsed >= MEASURE_DURATION)
        {
            state = ST_RESULTS;
            showResults();
            beep(1000, 100);
            delay(200);
            beep(1500, 100);
            delay(200);
            beep(2000, 150);
        }
    }

    delay(20);
}
