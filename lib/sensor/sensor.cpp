#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "pinout.h"
#include "app.h"
#include "sensor.h"

static MAX30105 hrmSensor;

// --- Filtro PPG (solo la task los usa) ---
static float dcSlow = 0.0;
static float dcFast = 0.0;
static float acSignal = 0.0;
static float envelope = 0.0;
static float normSignal = 0.5;

// Piso de ruido adaptativo: el umbral de latido se exige relativo
// a el (ver punto 2). Sube lento, baja rapido.
static float noiseRef = 1.0;
static const float NOISE_RATIO = 4.0f;   // envolvente debe superar 4x el piso
static const float MIN_AC = 12.0f;       // amplitud minima absoluta (cuentas)

static const float SCHMITT_HIGH = 0.50;
static const float SCHMITT_LOW = 0.35;
static bool beatDetected = false;

// --- Canal rojo (SpO2) ---
static float dcRedSlow = 0.0;
static float redFast = 0.0;
static double sumRedAC2 = 0.0;
static double sumIrAC2 = 0.0;
static int spo2Count = 0;
static const int SPO2_WINDOW = 300;      // 3 s a 100 Hz (estabiliza el ratio)
// Curva de Maxim por defecto. PENDIENTE: calibrar contra oximetro
// de referencia con este hardware antes de uso real.
static float spo2Smooth = 0.0f;

// --- Muñeca presente (umbral adaptativo con histeresis) ---
static float irBaseline = 8000.0f;       // nivel IR sin muñeca (se autoajusta)
static const float FINGER_ENTER = 1.40f; // x baseline para entrar
static const float FINGER_EXIT = 1.20f;  // x baseline para salir
static const float FINGER_FLOOR_ENTER = 15000.0f;
static const float FINGER_FLOOR_EXIT = 12000.0f;

// --- Estado compartido con app/display (solo la task escribe) ---
// El portMUX protege la publicacion/lectura coherente del grupo. Solo
// se toma critica alrededor de calculo puro (no de lecturas I2C).
static portMUX_TYPE sensorMux = portMUX_INITIALIZER_UNLOCKED;

volatile byte rates[RATE_SIZE] = {0};
volatile byte rateSpot = 0;

volatile long lastBeat = 0;
volatile float bpm = 0;
volatile int avgBpm = 0;
volatile int maxBpm = 0;
volatile int minBpm = 999;
volatile int bpmCount = 0;
volatile bool normInit = false;
volatile bool schmittState = false;
volatile bool wristPresent = false;
volatile bool sensorAvailable = false;
volatile int spo2 = 0;
volatile bool spo2Valid = false;
volatile float perfusion = 0.0f;

const float ALPHA_DC = 0.002;
static const float ALPHA_AC = 0.12;
static const float ENV_ATTACK = 0.3;
static const float ENV_DECAY = 0.985;

// Mediana de los slots no-cero (robusta contra outliers).
static float ratesMedian()
{
    byte tmp[RATE_SIZE];
    int n = 0;
    for (byte i = 0; i < RATE_SIZE; i++)
        if (rates[i] > 0) tmp[n++] = rates[i];
    if (n == 0) return 0.0f;
    // insertion sort
    for (int i = 1; i < n; i++)
    {
        byte k = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > k) { tmp[j + 1] = tmp[j]; j--; }
        tmp[j + 1] = k;
    }
    if (n & 1) return tmp[n / 2];
    return (tmp[n / 2 - 1] + tmp[n / 2]) / 2.0f;
}

static void processSample(long irValue, long redValue)
{
    if (!normInit)
    {
        dcSlow = (float)irValue;
        dcFast = (float)irValue;
        dcRedSlow = (float)redValue;
        redFast = (float)redValue;
        envelope = 1.0f;
        noiseRef = 1.0f;
        normInit = true;
    }

    float alphaAC = isCalibrating ? 0.3f : ALPHA_AC;

    dcSlow = dcSlow * (1.0f - currentAlphaDC) + (float)irValue * currentAlphaDC;
    dcFast = dcFast * (1.0f - alphaAC) + (float)irValue * alphaAC;
    acSignal = dcFast - dcSlow;

    float absAC = fabsf(acSignal);
    if (absAC > envelope)
        envelope = envelope * (1.0f - ENV_ATTACK) + absAC * ENV_ATTACK;
    else
        envelope *= ENV_DECAY;

    if (envelope < 1.0f) envelope = 1.0f;
    normSignal = acSignal / envelope;

    // Piso de ruido: baja rapido con la envolvente, sube lento.
    if (envelope < noiseRef)
        noiseRef = envelope;
    else
        noiseRef += (envelope - noiseRef) * 0.001f;
    if (noiseRef < 1.0f) noiseRef = 1.0f;

    // Puerta de perfusion: sin amplitud real no hay latido.
    bool enoughSignal = (envelope > noiseRef * NOISE_RATIO) && (envelope > MIN_AC);

    beatDetected = false;
    if (enoughSignal && normSignal > SCHMITT_HIGH && !schmittState)
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
        unsigned long now = millis();
        unsigned long delta = now - (unsigned long)lastBeat;
        lastBeat = (long)now;

        if (delta > 250 && delta < 2000)
        {
            bpm = 60000.0f / delta;

            if (bpm > 30 && bpm < 220)
            {
                // Aprendizaje: primeros 12 latidos entran directo.
                // Despues se rechaza contra la MEDIANA (>35% = sospechoso).
                bool accept = true;
                if (bpmCount > 12 && avgBpm > 0)
                {
                    float med = ratesMedian();
                    if (med > 0 && fabsf(bpm - med) > med * 0.35f)
                        accept = false;
                }
                if (accept)
                {
                    rates[rateSpot++] = (byte)bpm;
                    rateSpot %= RATE_SIZE;

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

    // Muñeca: baseline autoajustable con histeresis.
    if (!wristPresent)
    {
        irBaseline += (dcSlow - irBaseline) * 0.02f;  // sigue al ambiente
        float th = irBaseline * FINGER_ENTER;
        if (th < FINGER_FLOOR_ENTER) th = FINGER_FLOOR_ENTER;
        if ((float)irValue > th) wristPresent = true;
    }
    else
    {
        float th = irBaseline * FINGER_EXIT;
        if (th < FINGER_FLOOR_EXIT) th = FINGER_FLOOR_EXIT;
        if ((float)irValue < th) wristPresent = false;
    }

    // SpO2: ratio-de-ratios. Solo se acumula fuera de calibracion y de
    // warmup (cuando currentAlphaDC ya es estable). Rojo e IR usan los
    // MISMOS alphas para que el cociente no quede sesgado.
    float aR = redFast - dcRedSlow;
    dcRedSlow += ((float)redValue - dcRedSlow) * currentAlphaDC;
    redFast += ((float)redValue - redFast) * alphaAC;

    bool spo2Allowed = !isCalibrating && !warmupActive;
    if (spo2Allowed)
    {
        sumRedAC2 += (double)aR * aR;
        sumIrAC2 += (double)acSignal * acSignal;
        if (++spo2Count >= SPO2_WINDOW)
        {
            float rmsR = sqrtf((float)(sumRedAC2 / spo2Count));
            float rmsI = sqrtf((float)(sumIrAC2 / spo2Count));
            sumRedAC2 = 0.0;
            sumIrAC2 = 0.0;
            spo2Count = 0;

            perfusion = (dcSlow > 1000.0f) ? rmsI / dcSlow : 0.0f;
            bool ok = (perfusion > 0.0004f) && (dcRedSlow > 1000.0f) && (dcSlow > 1000.0f);
            if (ok)
            {
                float r = (rmsR / dcRedSlow) / (rmsI / dcSlow);
                if (r > 0.4f && r < 1.0f)
                {
                    float s = -45.06f * r * r + 30.354f * r + 94.845f;
                    if (s > 100.0f) s = 100.0f;
                    if (s < 70.0f) s = 70.0f;
                    spo2Smooth = spo2Valid ? (spo2Smooth * 3.0f + s) / 4.0f : s;
                    spo2 = (int)(spo2Smooth + 0.5f);
                    spo2Valid = true;
                }
                else
                {
                    spo2Valid = false;
                }
            }
            else
            {
                spo2Valid = false;
            }
        }
    }
    else
    {
        sumRedAC2 = 0.0;
        sumIrAC2 = 0.0;
        spo2Count = 0;
        spo2Valid = false;
    }
}

// Drena el FIFO procesando TODAS las muestras pendientes en pares IR+rojo.
// Solo la task llama. La critica envuelve el calculo (no las lecturas I2C)
// para que sensorReadMeasure() nunca vea un estado a medias.
void processSensor()
{
    if (!sensorAvailable) { wristPresent = false; return; }
    hrmSensor.check();
    while (hrmSensor.available())
    {
        long ir = (long)hrmSensor.getFIFOIR();
        long red = (long)hrmSensor.getFIFORed();
        hrmSensor.nextSample();

        portENTER_CRITICAL(&sensorMux);
        processSample(ir, red);
        portEXIT_CRITICAL(&sensorMux);
    }
}

void sensorReadMeasure(SensorMeasure &m)
{
    portENTER_CRITICAL(&sensorMux);
    m.avgBpm = avgBpm;
    m.maxBpm = maxBpm;
    m.minBpm = minBpm;
    m.spo2 = spo2;
    m.spo2Valid = spo2Valid;
    m.wristPresent = wristPresent;
    portEXIT_CRITICAL(&sensorMux);
}

// Deja el estado listo para arrancar una medicion nueva. Sin esto, el
// buffer de BPM y el SpO2 arrastran valores de la medicion anterior.
void sensorBeginMeasurement()
{
    portENTER_CRITICAL(&sensorMux);
    for (byte i = 0; i < RATE_SIZE; i++) rates[i] = 0;
    rateSpot = 0;
    avgBpm = 0;
    maxBpm = 0;
    minBpm = 999;
    bpmCount = 0;
    bpm = 0;
    lastBeat = 0;
    schmittState = false;
    normInit = false;
    spo2 = 0;
    spo2Valid = false;
    spo2Smooth = 0.0f;
    spo2Count = 0;
    sumRedAC2 = 0.0;
    sumIrAC2 = 0.0;
    perfusion = 0.0f;
    isCalibrating = true;
    currentAlphaDC = 0.05f;
    portEXIT_CRITICAL(&sensorMux);
}

// Fin de calibracion: limpia lo acumulado y pasa a medicion real.
void sensorEndCalibration()
{
    portENTER_CRITICAL(&sensorMux);
    for (byte i = 0; i < RATE_SIZE; i++) rates[i] = 0;
    rateSpot = 0;
    avgBpm = 0;
    bpmCount = 0;
    spo2 = 0;
    spo2Valid = false;
    spo2Smooth = 0.0f;
    spo2Count = 0;
    sumRedAC2 = 0.0;
    sumIrAC2 = 0.0;
    isCalibrating = false;
    currentAlphaDC = 0.1f;
    portEXIT_CRITICAL(&sensorMux);
}

static void sensorTask(void *)
{
    for (;;)
    {
        processSensor();
        vTaskDelay(pdMS_TO_TICKS(10));  // sensor a 100 Hz: drena cada 10 ms
    }
}

bool sensorInit()
{
    sensorAvailable = false;
    Wire.begin(PIN_SDA, PIN_SCL);

    // 3 intentos a 400kHz + 2 intentos a 100kHz (fallback para
    // cables largos o pull-ups debiles). Sin while(1): el que llama
    // decide que hacer si no hay sensor (modo demo).
    const uint32_t speeds[2] = {I2C_SPEED_FAST, I2C_SPEED_STD};
    const int tries[2] = {3, 2};
    for (int s = 0; s < 2 && !sensorAvailable; s++)
    {
        for (int i = 0; i < tries[s] && !sensorAvailable; i++)
        {
            if (hrmSensor.begin(Wire, speeds[s]))
            {
                sensorAvailable = true;
                break;
            }
            Serial.printf("MAX30102 intento %d (I2C %lu Hz) fallido\n", i + 1, (unsigned long)speeds[s]);
            delay(150);
        }
    }

    if (!sensorAvailable)
    {
        Serial.println("MAX30102 NO encontrado (modo demo, sin PPG)");
        return false;
    }
    // Configuración explícita: 100 Hz efectivo (sampleRate=100, sin promedio)
    // powerLevel=0x1F (máx brillo LED), sampleAverage=1, ledMode=2 (Red+IR),
    // sampleRate=100, pulseWidth=411, adcRange=16384
    hrmSensor.setup(0x1F, 1, 2, 100, 411, 16384);
    Serial.println("MAX30102 OK (IR+rojo, task en core 0)");
    xTaskCreatePinnedToCore(sensorTask, "ppg", 4096, NULL, 2, NULL, 0);
    return true;
}

bool sensorBeatLost()
{
    if (!sensorAvailable) return false;
    portENTER_CRITICAL(&sensorMux);
    unsigned long lb = (unsigned long)lastBeat;
    portEXIT_CRITICAL(&sensorMux);
    return (millis() - lb) > 3000UL;
}
