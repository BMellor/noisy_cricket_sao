#pragma once

#include <stdbool.h>
#include <stdint.h>

// Register map: write the register address first, then burst read or write (auto-increment).
// Multi-byte values are little-endian.
#define REG_ID 0x00         // RO, REG_ID_VALUE
#define REG_STATUS 0x01     // RO, STATUS_* bits
#define REG_THRESHOLD 0x02  // RW u16, mic level units; takes effect at STOP, not persisted until COMMIT
#define REG_LEVEL_FAST 0x04 // RO u16, ~44 ms average
#define REG_LEVEL_SLOW 0x06 // RO u16, ~11 s average
#define REG_COMMIT 0x08     // W, write COMMIT_KEY to save threshold + volume to flash (stalls everything briefly)
#define REG_LED 0x09        // RW, 3 bytes R G B: idle colour, chirps fade green on top
#define REG_VOLUME 0x0C     // RW u8, chirp volume (see DEFAULT_VOLUME); takes effect at STOP, saved by COMMIT
#define REG_ISR_LOAD 0x0D   // RO u8, % of CPU in the mic ISR over the last second
#define REG_STALLS 0x0E     // RO u8, PDM clock stalls (TX FIFO ran dry) in the last second, saturating
#define REG_FFT 0x10        // RO, 32 bins, 8 * log2(power) (0.376 dB/LSB), 0 .. 11.35 kHz in 366 Hz steps
#define REG_COUNT 0x30

#define REG_ID_VALUE 0xC7
#define COMMIT_KEY 0xA5

#define STATUS_QUIET 0x01
#define STATUS_ARMED 0x02
#define STATUS_CHIRPING 0x04

void i2c_target_init(void);

// Copy to/from the register file without racing the I2C ISR
void i2c_target_set(uint8_t reg, const uint8_t *src, uint8_t len);
void i2c_target_get(uint8_t reg, uint8_t *dst, uint8_t len);

// True once after each host write transaction (at its STOP)
bool i2c_target_host_wrote(void);
