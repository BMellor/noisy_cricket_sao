#pragma once

#include <stdbool.h>
#include <stdint.h>

// Level units: mean |x| of the 2.5 kHz low-passed signal, full scale +/-65536 (clipped at 32767);
// (8x the old 1.5 MHz Q4 units). A full-scale sine would read ~41700 before clipping.

void mic_init(void);

// While blanked the level averagers are frozen; the filters and FFT capture keep running.
void mic_set_blanking(bool blank);

uint16_t mic_level_fast(void); // ~44 ms time constant
uint16_t mic_level_slow(void); // ~11 s time constant

// Rotate-xor accumulation of raw PDM words (one per output sample), for PRNG seeding
uint32_t mic_entropy(void);

// Capture `len` consecutive DC-blocked samples (full scale +/-65536, saturated to +/-16383) into buf
void mic_capture_start(int16_t *buf, uint8_t len);
bool mic_capture_done(void);

// Cumulative core cycles spent in the mic ISR (incl. an estimate of entry/exit), wraps
uint32_t mic_isr_cycles(void);
// Cumulative count of ISR entries that found the SPI TX FIFO empty (PDM clock stalled)
uint32_t mic_stalls(void);
