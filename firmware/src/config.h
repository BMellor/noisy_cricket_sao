#pragma once

// Audio/mic sample rate: 3 MHz PDM / 16 bits per SPI word / 8 words per sample
#define SAMPLE_RATE_HZ 23437.5f

// Chirp = CHIRP_REPS playbacks of the LUT with CHIRP_REP_GAP_MS of silence after each
#define CHIRP_REPS 3
#define CHIRP_REP_GAP_MS 20

// Default playback volume, 0..255: gain = v + v/128 out of 256, so 0 = mute and 255 = table as-is
// (table peaks at +/-118 of 128). Host-tunable over I2C and saved with the threshold.
#define DEFAULT_VOLUME 160

// Chirp group = CHIRPS_MIN..CHIRPS_MAX chirps, CHIRP_GAP_MIN_MS..CHIRP_GAP_MAX_MS apart (random)
#define CHIRPS_MIN 1
#define CHIRPS_MAX 3
#define CHIRP_GAP_MIN_MS 300
#define CHIRP_GAP_MAX_MS 400

// Fixed delay from the quiet decision to the first chirp (sync anchor between devices). Host-tunable over I2C
// and saved with the threshold, like the rest times below.
#define DEFAULT_PREROLL_MS 2000

// Still quiet after a group: random rest before the next group (no pre-roll; noise during it restarts from idle)
#define DEFAULT_REST_MIN_MS 1000
#define DEFAULT_REST_MAX_MS 5000

// PWM running at midscale before the amp is enabled, then amp settling before the first chirp.
// The AD8107 ramps up over a time set by its BYPASS cap (2.2 uF fitted), several hundred ms.
#define PWM_WARMUP_MS 5
#define AMP_WARMUP_MS 300

// Extra blanking after the last chirp of a group (acoustic ring-down) before listening again
#define SETTLE_MS 100

// Noise threshold in mic level units (0..32767, see mic.h); host-tunable over I2C.
#define DEFAULT_THRESHOLD 60
// Loud again once level exceeds threshold + threshold/HYSTERESIS_DIV
#define HYSTERESIS_DIV 4

#define FFT_INTERVAL_MS 50
#define REG_UPDATE_MS 10

// 7-bit address. Avoids 0x3C/0x3D (SSD1306 displays) and 0x50 (SAO ID EEPROM).
#define I2C_TARGET_ADDR 0x42
