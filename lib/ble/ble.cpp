#include <Arduino.h>
#include "ble.h"

// TODO: NimBLE-Arduino — servicio 0x180D, caracteristica 0x2A37 (NOTIFY).
// Formato: [flags: 1 byte][bpm: 1 byte]. Nombre: "HeartRateMonitor".

void bleInit()
{
    // Pendiente de implementar. No-op para no cambiar conducta actual.
}

void bleUpdate(int bpm)
{
    (void)bpm;
    // Pendiente de implementar. No-op para no cambiar conducta actual.
}
