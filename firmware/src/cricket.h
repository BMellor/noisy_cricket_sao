#pragma once

#include <stdint.h>

void cricket_init(uint16_t threshold);

// Main-loop state machine: quiet detection, chirp group scheduling, blanking, LED fade
void cricket_poll(void);

void cricket_set_threshold(uint16_t threshold);

uint16_t cricket_threshold(void);

// Idle LED colour; chirps fade green on top of it
void cricket_set_led(uint8_t r, uint8_t g, uint8_t b);

// STATUS_* bits (i2c_target.h)
uint8_t cricket_status(void);
