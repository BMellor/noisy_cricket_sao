#pragma once

#include <stdbool.h>
#include <stdint.h>

uint32_t get_timestamp_ms();

bool ms_have_elapsed(uint32_t original_time, uint32_t duration_ms);

void init_timebase();
