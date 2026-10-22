#include <Arduino.h>
#include "pinout.h"
#include "keypad.h"

static const byte ROWS = 4;
static const byte COLS = 4;

// Tiempo que una lectura debe mantenerse estable para aceptarse.
// Cherry MX rebota ~5 ms; 15 ms da margen sin hacerse lento.
#define KEY_DEBOUNCE_MS 15

#define NO_KEY '\0'

// Tabla logica [fila][columna] -> caracter (filas/columnas DEL TECLADO).
// OJO: el barrido maneja drivers d (=columnas del teclado: 32,33,25,26
// = COL1..COL4) y lectores r (=filas del teclado: 34,35,36,39 =
// FILA1..FILA4). Por eso abajo se indexa keys[lector][driver].
// Si el teclado nuevo traspone filas<->columnas, trasponer esta tabla.
// Si solo cambia el ORDEN de pines, no tocar: el orden vive en
// PIN_KEY_R1..R4 / PIN_KEY_C1..C4 (pinout.h).
static char keys[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static byte rowPins[4] = {PIN_KEY_R1, PIN_KEY_R2, PIN_KEY_R3, PIN_KEY_R4};
static byte colPins[4] = {PIN_KEY_C1, PIN_KEY_C2, PIN_KEY_C3, PIN_KEY_C4};

static char candidate = NO_KEY;
static unsigned long candidateSince = 0;
static bool waitingRelease = false;

// Lectura instantanea sin debounce: recorre filas, una en LOW y el
// resto en alta impedancia (sin cortos con 2 teclas misma columna).
static char scanRaw()
{
    char result = NO_KEY;

    for (byte r = 0; r < ROWS; r++)
    {
        for (byte i = 0; i < ROWS; i++)
        {
            if (i == r)
            {
                pinMode(rowPins[i], OUTPUT);
                digitalWrite(rowPins[i], LOW);
            }
            else
            {
                pinMode(rowPins[i], INPUT);  // alta impedancia
            }
        }
        delayMicroseconds(50);  // asentamiento del pull-up 10k

        for (byte c = 0; c < COLS; c++)
        {
            if (digitalRead(colPins[c]) == LOW)
            {
                // r = driver = columna del teclado, c = lector = fila:
                // se traspone para indexar keys[fila][columna].
                result = keys[c][r];
            }
        }
    }

    // Reposo seguro: todas las filas en pull-up, ningun LOW activo.
    for (byte i = 0; i < ROWS; i++)
    {
        pinMode(rowPins[i], INPUT_PULLUP);
    }

    return result;
}

char getKey()
{
    char raw = scanRaw();
    unsigned long now = millis();

    if (raw != candidate)
    {
        candidate = raw;
        candidateSince = now;
    }

    if (waitingRelease)
    {
        if (candidate == NO_KEY)
            waitingRelease = false;
        return NO_KEY;
    }

    if (candidate != NO_KEY && (now - candidateSince) >= KEY_DEBOUNCE_MS)
    {
        waitingRelease = true;
        return candidate;
    }

    return NO_KEY;
}

void initKeypad()
{
    for (byte r = 0; r < ROWS; r++)
    {
        pinMode(rowPins[r], INPUT_PULLUP);  // reposo: ninguna fila en LOW
    }
    for (byte c = 0; c < COLS; c++)
    {
        pinMode(colPins[c], INPUT);   // pull-up EXTERNO 10k a 3V3
    }
    candidate = NO_KEY;
    waitingRelease = false;
}
