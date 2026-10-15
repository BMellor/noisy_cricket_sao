#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t threshold;
  uint8_t volume;
} settings_t;

// Returns false (leaving *s untouched) if nothing valid has been saved
bool settings_load(settings_t *s);

// Erases and rewrites the settings sector with interrupts masked; the mic clock stalls meanwhile
bool settings_save(const settings_t *s);
