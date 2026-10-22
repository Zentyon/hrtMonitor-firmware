# AGENTS.md

Guía para agentes de IA: cómo compilar, flashear y verificar este firmware en el ESP32 real.

## Proyecto

Firmware PlatformIO/Arduino del monitor HRT (Omnipulsera): ESP32 DevKit V1 + TFT ILI9341
2.4" + teclado 4x4 + MAX30102 (I2C SDA=21/SCL=22) + audio I2S MAX98357A.

- `src/main.cpp` → `appSetup()` / `appLoop()` (máquina de estados en `lib/app`).
- `lib/display` (TFT UI), `lib/sensor` (MAX30102 + task dedicada), `lib/keypad`,
  `lib/profile`, `lib/audio`, `lib/ble` (stub no-op), `include/pinout.h` (mapa de pines).
- Ver `README.md` para hardware y estructura.

## Compilar

```bash
pio run -e esp32dev
```

Envs (`platformio.ini`): `esp32dev` (principal), `voice_test`, `keymap`, `pintest`.
Usan `build_src_filter` con mains alternativos: **no borrar** `src/*_main.cpp` ni `tests/`
(sin ellos `env:keymap` y `env:pintest` no compilan).

Objetivo sano actual: RAM ~6.9 %, Flash ~30 %.

### En este entorno (sandbox flatpak, hosts montados)

`pio` del PATH no arranca directo; usar este wrapper:

```bash
LD_LIBRARY_PATH=/run/host/usr/lib64:/run/host/usr/lib \
PYTHONPATH=/home/zent/.local/lib/python3.14/site-packages \
/run/host/usr/bin/python3.14 /home/zent/.local/bin/pio "$@"
```

## Flashear

```bash
pio run -e esp32dev -t upload        # puerto /dev/ttyUSB0
```

- `upload_speed = 460800` está puesto a propósito: con **921600 falla**
  (`Unable to verify flash chip connection`, CP2102). No subirlo.
- Éxito = `Hash of data verified` + `Hard resetting via RTS pin...`.
- Chip: ESP32-D0WD-V3, MAC 30:76:f5:93:b1:c4.

## Monitor serial

`monitor_speed = 115200`. `pio device monitor` rechaza stdin piped en este entorno →
usar pyserial (`pyserial` 3.5 ya está instalado):

```python
# verificacion_boot.py [puerto] [segundos]   — abrir el puerto resetea la placa (DTR)
import sys, time, serial
port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
dur = float(sys.argv[2]) if len(sys.argv) > 2 else 10.0
with serial.Serial(port, 115200, timeout=0.2) as s:
    end = time.time() + dur
    while time.time() < end:
        n = s.in_waiting
        if n:
            sys.stdout.write(s.read(n).decode("utf-8", "replace")); sys.stdout.flush()
```

Nota: el puerto a veces escupe un backlog de datos viejos a toda velocidad (artefacto
del buffer, no del firmware); reintentar tras reset.

## Comportamiento ESPERADO al arrancar (no es bug)

- MAX30102 **no conectado hoy** → `MAX30102 intento N (I2C 400000 Hz) fallido`,
  `MAX30102 NO encontrado (modo demo, sin PPG)` y pantalla "Sensor no detectado /
  Modo demo (sin PPG)". Hasta que no se conecte el sensor, esto es correcto.
- Boot limpio: sin `PANIC` / `Guru Meditation` / `abort()`. Tarda ~6 s en llegar al
  menú (test de LEDs 1.4 s + splash 1.8 s + aviso de sensor 2.2 s).

## Reglas del repo (no romper)

1. **Strings de UI SOLO ASCII**: las fuentes built-in de TFT_eSPI no traen
   `ñ`/acentos/`♀`/`♂` → se dibujan en blanco/garbage. Reglas del renderer:
   `if (c > 255) return; if (!_cp437 && c > 175) c++;` en `drawChar`.
2. **Concurrencia**: `lib/sensor` corre en task propia (core 0, 500 Hz). Todo acceso
   desde el loop (core 1) a `currentBpm`, `currentHr`, `spo2Smooth`, `beatAvg`,
   `isCalibrating`, `currentAlphaDC`, `warmupActive` va con `portMUX_TYPE sensorMux`
   (`sensorReadMeasure()`, `sensorBeginMeasurement()`, `sensorEndCalibration()`).
   Nunca bloquear/dormir dentro de la sección crítica.
3. **Medición**: al entrar a calibración/medición → `sensorBeginMeasurement()`;
   al terminar calibración → `sensorEndCalibration()` (reinicia estadísticas por medición).
4. **SpO2**: solo con pulso detectado y `!isCalibrating && !warmupActive`; canales
   IR/ROJO emparejados (`currentAlphaDC`/`alphaAC`), ventana `SPO2_WINDOW = 300`.
5. `String` → `snprintf` en rutas de dibujo calientes; pantalla de medición con
   throttle ~200 ms (`updateMeasureDisplay`).
6. No commitear ni pushear sin que lo pida el usuario.

## Estado conocido / pendientes

1. **TFT física en blanco** (bug abierto): probar configs de display (driver ILI9341 vs
   ST7789, `TFT_RST`, frecuencia SPI) en una carpeta de tests aparte.
2. **UI virtual en navegador** (plan A aprobado): espejo de comandos de dibujo por
   WebSocket + SoftAP, env `esp32dev_debug` con `-D UI_MIRROR=1` → aún sin implementar.
3. MAX30102 pendiente de conectar.
4. No implementado: oversampling, BLE (`lib/ble` stub), tests unitarios, watchdog.

## Verificación mínima tras tocar código

1. `pio run -e esp32dev` → `SUCCESS`.
2. `-t upload` → `Hash of data verified`.
3. Boot por serial → sin panics → llega a pantalla de menú (o aviso de sensor).
4. Si toca UI: texto visible en pantalla y **solo ASCII**.
