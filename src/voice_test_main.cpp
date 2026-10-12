#include <Arduino.h>
#include "audio.h"
#include "voice_data.h"

// Prueba de voz: bocina 8ohm 1W + MAX98357A.
// Dice "hola" y "listo" en loop. Sirve para validar la etapa
// de audio antes de usar prompts de voz en la app.
// NOTA: alimenta el MAX98357A a 3.3V para esta prueba
// (a 5V puede entregar ~2.5W y forzar la bocina de 1W).

void setup()
{
    Serial.begin(115200);
    Serial.println("========================================");
    Serial.println("   Voice test - MAX98357A + 8ohm 1W");
    Serial.println("========================================");

    if (!initAudio())
    {
        Serial.println("ERROR: no se pudo inicializar I2S");
        while (1) delay(1000);
    }
    Serial.println("I2S OK. Hablando...");
}

void loop()
{
    Serial.println("dice: hola");
    audioPlayPCM(voice_hola, voice_hola_len, 1.0f);
    delay(1000);
    Serial.println("dice: listo");
    audioPlayPCM(voice_listo, voice_listo_len, 1.0f);
    delay(2500);
}
