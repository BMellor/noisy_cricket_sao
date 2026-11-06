#pragma once

#include <stdint.h>

// Main-loop state machine: quiet detection, chirp group scheduling, blanking, LED fade
void cricket_poll(void);

void cricket_set_threshold(uint16_t threshold);

// Pre-roll before the first chirp after quiet, and the random rest range between groups (rest_min <= rest_max)
void cricket_set_timing(uint16_t preroll_ms, uint16_t rest_min_ms, uint16_t rest_max_ms);

// Idle LED colour; chirps fade green on top of it
void cricket_set_led(uint8_t r, uint8_t g, uint8_t b);

// STATUS_* bits (i2c_target.h)
uint8_t cricket_status(void);
