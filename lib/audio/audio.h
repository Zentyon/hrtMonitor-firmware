#pragma once

// Audio I2S via MAX98357A. El amplificador solo se enciende
// durante el beep (SD en HIGH) para evitar ruido de fondo en idle.

void beep(int freq, int durationMs);
bool initAudio();
void audioMute();
void audioUnmute();

// Reproduce PCM mono 16-bit a SAMPLE_RATE (datos en flash; se copian
// a RAM por bloques porque el DMA de I2S no lee de flash).
void audioPlayPCM(const int16_t* pcm, size_t sampleCount, float gain = 1.0f);
