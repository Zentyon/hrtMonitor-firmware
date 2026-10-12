#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "pinout.h"
#include "app.h"
#include "sensor.h"

static MAX30105 hrmSensor;

// --- Filtro PPG (solo processSensor los usa) ---
static float dcSlow = 0.0;
static float dcFast = 0.0;
static float acSignal = 0.0;
static float envelope = 0.0;
static float normSignal = 0.5;

const float ALPHA_DC = 0.002;
static const float ALPHA_AC = 0.12;
static const float ENV_ATTACK = 0.3;
static const float ENV_DECAY = 0.985;

static const float SCHMITT_HIGH = 0.55;
static const float SCHMITT_LOW = 0.45;
static bool beatDetected = false;

// --- Estado compartido con app/display ---
byte rates[RATE_SIZE] = {0};
byte rateSpot = 0;

long lastBeat = 0;
float bpm = 0;
int avgBpm = 0;
int maxBpm = 0;
int minBpm = 999;
int bpmCount = 0;
bool normInit = false;
bool schmittState = false;
bool fingerPresent = false;

void sensorInit()
{
    Wire.begin(PIN_SDA, PIN_SCL);

    if (!hrmSensor.begin(Wire, I2C_SPEED_FAST))
    {
        Serial.println("MAX30102 NO encontrado");
        while (1);
    }
    // Configuración explícita: 100 Hz efectivo (sampleRate=100, sin promedio)
    // powerLevel=0x1F (máx brillo LED), sampleAverage=1, ledMode=2 (Red+IR),
    // sampleRate=100, pulseWidth=411, adcRange=16384
    hrmSensor.setup(0x1F, 1, 2, 100, 411, 16384);
}

void processSensor()
{
    long irValue = hrmSensor.getIR();

    if (!normInit)
    {
        dcSlow = irValue;
        dcFast = irValue;
        envelope = 1.0f;
        normInit = true;
    }

    float alphaAC = isCalibrating ? 0.3f : ALPHA_AC;

    dcSlow = dcSlow * (1.0 - currentAlphaDC) + (float)irValue * currentAlphaDC;
    dcFast = dcFast * (1.0 - alphaAC) + (float)irValue * alphaAC;
    acSignal = dcFast - dcSlow;

    float absAC = fabs(acSignal);
    if (absAC > envelope)
        envelope = envelope * (1.0 - ENV_ATTACK) + absAC * ENV_ATTACK;
    else
        envelope *= ENV_DECAY;

    if (envelope < 1.0) envelope = 1.0;
    normSignal = acSignal / envelope;

    beatDetected = false;
    if (normSignal > SCHMITT_HIGH && !schmittState)
    {
        schmittState = true;
        beatDetected = true;
    }
    else if (normSignal < SCHMITT_LOW)
    {
        schmittState = false;
    }

    if (beatDetected)
    {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        if (delta > 250 && delta < 2000)
        {
            bpm = 60000.0 / delta;

            if (bpm > 30 && bpm < 220)
            {
                // Rechazar latido solo si ya hay datos y difiere >40% del promedio
                if (bpmCount > 10 && avgBpm > 0 && fabs(bpm - avgBpm) > avgBpm * 0.40f)
                {
                    // Latido sospechoso, descartar
                }
                else
                {
                    rates[rateSpot++] = (byte)bpm;
                    rateSpot %= RATE_SIZE;

                    // Promedio ignorando slots en 0 (evita dilución)
                    int sum = 0, cnt = 0;
                    for (byte i = 0; i < RATE_SIZE; i++)
                        if (rates[i] > 0) { sum += rates[i]; cnt++; }
                    avgBpm = cnt > 0 ? sum / cnt : 0;

                    bpmCount++;
                    if (avgBpm > maxBpm) maxBpm = avgBpm;
                    if (avgBpm < minBpm && avgBpm > 0) minBpm = avgBpm;
                }
            }
        }
    }

    fingerPresent = (irValue > 20000);
}
