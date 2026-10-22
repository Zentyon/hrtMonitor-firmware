#include <Arduino.h>

// Descubridor de mapeo del teclado 4x4. NO usa pinout.h a proposito:
// los 8 pines se prueban por numero directo porque el orden real
// no coincide con el del codigo.
//
// Protocolo serial (115200):
//   MAP D<drv> b:<otros drivers> r:<lectores 34,35,36,39>
//   EVT <ms> D<drv> R<rd> DOWN|UP   flancos (quedan en el buffer)
//   STICKY ...                      resumen pegajoso ('c' lo limpia)
//
// Sin tecla: todo 1 (salvo flotantes sin pull-ups). Con tecla X:
// el driver de su fila muestra 0 en el bit de su columna.

static const uint8_t BIDIR[4] = {32, 33, 25, 26};
static const uint8_t READERS[4] = {34, 35, 36, 39};

static bool sticky[4][4] = {{false}};
static bool prevLow[4][4] = {{false}};

void setup()
{
    Serial.begin(115200);
    for (auto p : BIDIR) pinMode(p, INPUT_PULLUP);
    for (auto p : READERS) pinMode(p, INPUT);  // flotantes hasta poner pull-ups
    Serial.println("KEYMAP: listo.");
}

void loop()
{
    static unsigned long lastMap = 0, lastRaw = 0;
    unsigned long now = millis();

    if (now - lastMap >= 400)
    {
        lastMap = now;
        for (uint8_t d = 0; d < 4; d++)
        {
            uint8_t drv = BIDIR[d];
            int b[3], r[4];
            for (auto p : BIDIR) pinMode(p, INPUT_PULLUP);
            pinMode(drv, OUTPUT);
            digitalWrite(drv, LOW);
            delayMicroseconds(200);
            uint8_t bi = 0;
            for (auto p : BIDIR)
                if (p != drv) b[bi++] = digitalRead(p);
            for (uint8_t i = 0; i < 4; i++) r[i] = digitalRead(READERS[i]);

            // MAP silenciado (alto volumen: descomentar solo capturas cortas).
            // Serial.print("MAP D");
            // Serial.print(drv);
            // Serial.print(" b:");
            // for (uint8_t i = 0; i < 3; i++) Serial.print(b[i]);
            // Serial.print(" r:");
            // for (uint8_t i = 0; i < 4; i++) Serial.print(r[i]);
            // Serial.println();
            (void)b;

            for (uint8_t i = 0; i < 4; i++)
            {
                bool isLow = (r[i] == LOW);
                if (isLow && !prevLow[d][i])
                {
                    Serial.print("EVT ");
                    Serial.print(millis());
                    Serial.print(" D");
                    Serial.print(drv);
                    Serial.print(" R");
                    Serial.print(READERS[i]);
                    Serial.println(" DOWN");
                }
                else if (!isLow && prevLow[d][i])
                {
                    Serial.print("EVT ");
                    Serial.print(millis());
                    Serial.print(" D");
                    Serial.print(drv);
                    Serial.print(" R");
                    Serial.print(READERS[i]);
                    Serial.println(" UP");
                }
                if (isLow) sticky[d][i] = true;
                prevLow[d][i] = isLow;
            }
        }
        for (auto p : BIDIR) pinMode(p, INPUT_PULLUP);
    }

    while (Serial.available())
    {
        if (Serial.read() == 'c')
        {
            for (auto &dr : sticky) for (auto &s : dr) s = false;
            for (auto &dr : prevLow) for (auto &s : dr) s = false;
            Serial.println("STICKY cleared");
        }
    }

    if (now - lastRaw >= 2000)
    {
        lastRaw = now;
        Serial.print("RAW");
        for (auto p : BIDIR) { Serial.print(" "); Serial.print(p); Serial.print(":"); Serial.print(digitalRead(p)); }
        for (auto p : READERS) { Serial.print(" "); Serial.print(p); Serial.print(":"); Serial.print(digitalRead(p)); }
        Serial.println();
        Serial.print("STICKY");
        for (uint8_t d = 0; d < 4; d++)
        {
            Serial.print(" ");
            Serial.print(BIDIR[d]);
            Serial.print(":");
            for (uint8_t r = 0; r < 4; r++) Serial.print(sticky[d][r] ? "0" : "1");
        }
        Serial.println();
    }
}
