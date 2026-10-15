#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>

// TIMG8 counts up at 24 MHz / 8 and wraps every 1 ms; usable as a fine-grained cycle timer
#define TIMEBASE_TICKS_PER_MS 3000
#define TIMEBASE_CYCLES_PER_TICK 8
static inline uint32_t timebase_ticks(void) { return TIMG8->COUNTERREGS.CTR; }

uint32_t get_timestamp_ms(void);

bool ms_have_elapsed(uint32_t original_time, uint32_t duration_ms);

void delay_ms(uint32_t duration_ms);

void timebase_init(void);
