#include <Arduino.h>
#include "pinout.h"
#include "keypad.h"

static const byte ROWS = 4;
static const byte COLS = 4;

#define NO_KEY '\0'

static char keys[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static byte rowPins[4] = {PIN_KEY_R1, PIN_KEY_R2, PIN_KEY_R3, PIN_KEY_R4};
static byte colPins[4] = {PIN_KEY_C1, PIN_KEY_C2, PIN_KEY_C3, PIN_KEY_C4};

static char lastKey = NO_KEY;

char getKey()
{
    char result = NO_KEY;

    for (byte r = 0; r < ROWS; r++)
    {
        // Activa solo esta fila (LOW)
        for (byte i = 0; i < ROWS; i++)
            digitalWrite(rowPins[i], (i == r) ? LOW : HIGH);

        for (byte c = 0; c < COLS; c++)
        {
            if (digitalRead(colPins[c]) == LOW)
            {
                result = keys[r][c];
            }
        }
    }

    // Detecta flanco de presion (IDLE -> PRESSED)
    char out = NO_KEY;
    if (result != NO_KEY && lastKey == NO_KEY)
        out = result;
    lastKey = result;

    return out;
}

void initKeypad()
{
    for (byte r = 0; r < ROWS; r++)
    {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], HIGH);
    }
    for (byte c = 0; c < COLS; c++)
    {
        pinMode(colPins[c], INPUT);   // requiere pull-up externo
    }
}
