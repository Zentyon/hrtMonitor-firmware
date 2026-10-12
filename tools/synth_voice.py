"""Sintesis de voz por formantes (estilo Klatt simplificado) para MAX98357A.

Genera clips cortos en espanol (44.1kHz mono 16-bit) y los escribe como
header C (int16 array) para embeber en flash del ESP32.

Uso:
    python tools/synth_voice.py
Sale:
    src/voice_data.h  (voice_hola, voice_listo + longitudes)
"""
import numpy as np
import os

SR = 44100
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "voice_data.h")

rng = np.random.default_rng(7)

# Formantes vocales espanol (varon adulto aprox): F1,F2,F3 + anchos de banda
FORMANTS = {
    "a": ((730, 1090, 2440), (80, 90, 120)),
    "e": ((530, 1840, 2480), (70, 90, 120)),
    "i": ((300, 2200, 2960), (60, 100, 140)),
    "o": ((570, 840, 2410), (80, 100, 120)),
    "u": ((440, 1020, 2240), (70, 90, 120)),
    "l": ((380, 1150, 2600), (90, 100, 130)),  # lateral aprox (sin antiformante)
}


def resonator(x, freq, bw):
    """Resonador de 2 polos (celda de Klatt en cascada)."""
    t = 1.0 / SR
    r = np.exp(-np.pi * bw * t)
    c = -r * r
    b = 2.0 * r * np.cos(2.0 * np.pi * freq * t)
    a = 1.0 - b - c
    y = np.empty_like(x)
    y1 = y2 = 0.0
    for n in range(len(x)):
        y0 = a * x[n] + b * y1 + c * y2
        y[n] = y0
        y2, y1 = y1, y0
    return y


def glottal_train(n, f0):
    """Tren de impulsos glotales con f0 (Hz) fijo en el segmento."""
    period = SR / f0
    x = np.zeros(n)
    x[:: max(1, int(round(period)))] = 1.0
    return x


def render_vowel(dur, formant, f0, amp):
    n = int(SR * dur)
    tt = np.arange(n) / SR
    # Micro-jitter de f0 para que suene menos robotico
    jit = 1.0 + 0.004 * np.sin(2 * np.pi * 5.3 * tt)
    # Tren de impulsos con periodo variable (aprox por tramos)
    x = np.zeros(n)
    phase = 0.0
    for i in range(n):
        fi = f0 * (1.0 + 0.004 * np.sin(2 * np.pi * 5.3 * i / SR))
        phase += fi / SR
        if phase >= 1.0:
            phase -= 1.0
            x[i] = 1.0
    # Aspiracion leve mezclada en la fuente
    x = x + 0.02 * rng.standard_normal(n)
    (f1, f2, f3), (b1, b2, b3) = formant
    y = resonator(x, f1, b1)
    y = resonator(y, f2, b2)
    y = resonator(y, f3, b3)
    # Envolvente: ataque/decaimiento de 15 ms
    m = int(0.015 * SR)
    env = np.ones(n)
    env[:m] = np.linspace(0, 1, m)
    env[-m:] = np.linspace(1, 0, m)
    return amp * env * y


def render_silence(dur):
    return np.zeros(int(SR * dur))


def render_s(dur, amp=0.5):
    """Fricativa /s/: ruido pasa-altas (pico ~5-6 kHz)."""
    n = int(SR * dur)
    noise = rng.standard_normal(n)
    # Pasa-altas simple: x - pasa-bajas(x)
    k = np.exp(-2 * np.pi * 4500 / SR)
    lp = np.empty(n)
    acc = 0.0
    for i in range(n):
        acc = (1 - k) * noise[i] + k * acc
        lp[i] = acc
    y = noise - lp
    m = int(0.02 * SR)
    env = np.ones(n)
    env[:m] = np.linspace(0, 1, m)
    env[-m:] = np.linspace(1, 0, m)
    return amp * env * y


def render_t(closure=0.06, amp=0.6):
    """Oclusiva /t/: cierre (silencio) + explosion corta."""
    n = int(SR * 0.03)
    noise = rng.standard_normal(n)
    k = np.exp(-2 * np.pi * 2500 / SR)
    lp = np.empty(n)
    acc = 0.0
    for i in range(n):
        acc = (1 - k) * noise[i] + k * acc
        lp[i] = acc
    burst = noise - 0.5 * lp
    env = np.exp(-np.arange(n) / (0.008 * SR))
    return np.concatenate([np.zeros(int(SR * closure)), amp * env * burst])


def xfade(a, b, ms=0.03):
    """Une dos segmentos con crossfade de ms para evitar clicks."""
    m = int(ms * SR)
    m = min(m, len(a), len(b))
    if m <= 0:
        return np.concatenate([a, b])
    w = np.linspace(0, 1, m)
    mid = a[-m:] * (1 - w) + b[:m] * w
    return np.concatenate([a[:-m], mid, b[m:]])


def phrase(segments):
    y = np.zeros(0)
    for seg in segments:
        y = xfade(y, seg) if len(y) else seg
    # Normaliza a pico ~0.37 (-8.7 dB, headroom para bocina 1W)
    peak = np.max(np.abs(y))
    if peak > 0:
        y = y * (0.37 / peak)
    return (y * 32767).astype(np.int16)


def build():
    sil = lambda d: render_silence(d)
    V = lambda ph, d, f0, amp=1.0: render_vowel(d, FORMANTS[ph], f0, amp)

    # "hola" /o'la/ — acento en 'a', f0 sube y luego cae
    hola = phrase([
        sil(0.10),
        V("o", 0.18, 122, 0.85),
        V("l", 0.08, 128, 0.60),
        V("a", 0.34, 132, 1.00),
        sil(0.15),
    ])
    # "listo" /'listo/ — acento en 'i'
    listo = phrase([
        sil(0.10),
        V("l", 0.07, 132, 0.60),
        V("i", 0.18, 136, 1.00),
        render_s(0.15),
        render_t(),
        V("o", 0.26, 112, 0.90),
        sil(0.15),
    ])
    return {"hola": hola, "listo": listo}


def write_header(clips):
    lines = []
    lines.append("#pragma once")
    lines.append("#include <stdint.h>")
    lines.append("#include <stddef.h>")
    lines.append("")
    lines.append("// Generado por tools/synth_voice.py — voz por formantes 44.1kHz mono 16-bit")
    lines.append("// Coincide con SAMPLE_RATE del modulo audio (I2S MAX98357A).")
    lines.append("")
    for name, data in clips.items():
        lines.append(f"const size_t voice_{name}_len = {len(data)};")
        lines.append(f"const int16_t voice_{name}[{len(data)}] = {{")
        for i in range(0, len(data), 12):
            chunk = data[i:i + 12]
            lines.append("  " + ", ".join(str(int(v)) for v in chunk) + ",")
        lines.append("};")
        lines.append("")
    with open(OUT, "w") as f:
        f.write("\n".join(lines))
    for name, data in clips.items():
        dur = len(data) / SR
        rms = float(np.sqrt(np.mean(data.astype(np.float64) ** 2)) / 32768)
        print(f"{name}: {len(data)} muestras, {dur:.2f}s, pico={np.max(np.abs(data))}, rms={rms:.3f}")
    print("Escrito", OUT)


if __name__ == "__main__":
    write_header(build())
