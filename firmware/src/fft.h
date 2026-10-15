#pragma once

#include <stdint.h>

#define FFT_N 64
#define FFT_BINS 32 // 0 .. 11.35 kHz in 366 Hz steps

// Buffer the mic capture writes into (FFT_N samples)
int16_t *fft_input(void);

// Hann-windowed FFT of the captured buffer (in place). out[k] = 8 * log2(|X[k]|^2), i.e. 0.376 dB per LSB.
void fft_compute(uint8_t out[FFT_BINS]);
