#pragma once

#include <stdbool.h>
#include <stdint.h>

void led_init(void);

void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);