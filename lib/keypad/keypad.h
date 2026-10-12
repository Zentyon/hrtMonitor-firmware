#pragma once

// Keypad 4x4 con barrido manual.
// Filas = OUTPUT (32,33,25,26). Columnas = INPUT (34,35,36,39).
// Los GPIO 34/35/36/39 son solo-entrada (sin pull-up interno).
// REQUISITO HARDWARE: 4 resistencias pull-up de 10k de cada
// columna (34,35,36,39) a 3.3V, para que en reposo lean HIGH.

void initKeypad();
char getKey();  // '\0' si no hay tecla nueva (detecta flanco)
