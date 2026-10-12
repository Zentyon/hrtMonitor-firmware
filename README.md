# hrtMonitor-firmware

Firmware PlatformIO (ESP32 DevKit V1 + Arduino) del monitor HRT / Omnipulsera.
Derivado del proyecto HRT-Monitor original, migrado a estructura PlatformIO modular.

## Hardware

- ESP32 DevKit V1
- Sensor MAX30102 (I2C: SDA=GPIO21, SCL=GPIO22)
- Display TFT 2.4" ILI9341 (SPI: MOSI=23, SCK=18, CS=5, DC=2, RST=EN, BL=3.3V)
- Teclado matricial 4x4 (filas: 32,33,25,26 / columnas: 34,35,36,39)
- LEDs semáforo (verde=13, amarillo=14, rojo=19)
- Audio I2S MAX98357A + bocina 8Ω 1W (BCLK=27, LRC=15, DIN=4, SD=12)

Ver `include/pinout.h` para el mapa completo de pines.

## Estructura

```
platformio.ini      # env:esp32dev (app) + env:voice_test (prueba audio)
src/main.cpp        # setup()/loop() -> appSetup()/appLoop()
src/voice_test_main.cpp  # prueba de voz ("hola"/"listo"), solo env:voice_test
src/voice_data.h    # generado por tools/synth_voice.py (no editar a mano)
include/pinout.h    # mapa de pines
lib/app/            # máquina de estados (splash, onboarding, menú, calibración, medición)
lib/sensor/         # driver MAX30102
lib/display/        # driver TFT ILI9341
lib/keypad/         # teclado 4x4
lib/profile/        # perfil / persistencia
lib/ble/            # BLE (stub)
lib/audio/          # I2S + reproducción PCM
tools/synth_voice.py # síntesis de voz por formantes -> src/voice_data.h
```

## Dependencias (PlatformIO)

- `bodmer/TFT_eSPI@^2.5.43`
- `sparkfun/SparkFun MAX3010x Pulse and Proximity Sensor Library@^1.1.2`

Se instalan solas con `pio run`.

## Uso

```bash
# Firmware principal
pio run -e esp32dev -t upload
pio device monitor -e esp32dev

# Prueba de voz (bocina + MAX98357A, alimenta a 3.3V)
pio run -e voice_test -t upload
pio device monitor -e voice_test

# Regenerar voz
python tools/synth_voice.py
```

## Flujo de la app

Splash → onboarding (idioma, edad, género, ejercicio, condiciones) → menú →
config / calibrar (5 s) / countdown (3 s) / medición (90 s) → resultados.
