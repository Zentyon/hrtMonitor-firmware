#include <Arduino.h>

// Autotest de pines del teclado SIN teclado conectado.
// Detecta cortos entre pines y distingue flotante de corto real.
//
// BIDIR (con pull-up interno): 32,33,25,26
// READERS (sin pull-up, deben flotar): 34,35,36,39
//
// Reporta por serial (115200) cada 3 s:
//   T1: matriz bidireccionales (corto = FAIL)
//   T2: a quien siguen los lectores (correlacion = corto, azar = flotante)

static const uint8_t BIDIR[4] = {32, 33, 25, 26};
static const uint8_t READERS[4] = {34, 35, 36, 39};

static void allPullup()
{
    for (auto p : BIDIR) { pinMode(p, INPUT_PULLUP); }
    for (auto p : READERS) { pinMode(p, INPUT); }
}

void setup()
{
    Serial.begin(115200);
    allPullup();
    Serial.println("PINTEST: listo. Teclado DESCONECTADO para esta prueba.");
}

void loop()
{
    // ---- T1: cortos entre bidireccionales ----
    // Cada driver en LOW, los demas con pull-up deben leer HIGH.
    Serial.println("--- T1 bidir-vs-bidir (todo HIGH = PASS) ---");
    bool t1pass = true;
    for (auto drv : BIDIR)
    {
        allPullup();
        pinMode(drv, OUTPUT);
        digitalWrite(drv, LOW);
        delay(2);
        Serial.print("DRV");
        Serial.print(drv);
        Serial.print(":");
        for (auto p : BIDIR)
        {
            if (p == drv) continue;
            int v = digitalRead(p);
            Serial.print(" ");
            Serial.print(p);
            Serial.print("=");
            Serial.print(v);
            if (v == LOW) t1pass = false;
        }
        Serial.println();
    }
    Serial.print("T1-result: ");
    Serial.println(t1pass ? "PASS" : "FAIL(corto entre bidireccionales o a GND)");

    // ---- T2: correlacion de lectores ----
    // Se alterna cada driver LOW/HIGH 6 veces; un lector en corto
    // sigue al driver (6/6), uno flotante responde al azar.
    Serial.println("--- T2 lectores (sigue=correlacion 6/6, flotante=mezcla) ---");
    for (auto drv : BIDIR)
    {
        int follow[4] = {0, 0, 0, 0};
        for (int k = 0; k < 6; k++)
        {
            allPullup();
            pinMode(drv, OUTPUT);
            digitalWrite(drv, (k % 2 == 0) ? LOW : HIGH);
            delay(3);
            for (uint8_t i = 0; i < 4; i++)
            {
                int expectHigh = (k % 2 == 1);
                if (digitalRead(READERS[i]) == (expectHigh ? HIGH : LOW))
                    follow[i]++;
            }
        }
        Serial.print("DRV");
        Serial.print(drv);
        Serial.print(": ");
        for (uint8_t i = 0; i < 4; i++)
        {
            Serial.print(READERS[i]);
            Serial.print("=");
            Serial.print(follow[i]);
            Serial.print("/6 ");
        }
        Serial.println();
    }
    allPullup();
    Serial.println("=== fin ciclo (3 s) ===");
    delay(3000);
}
