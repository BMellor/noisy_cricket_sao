#include "settings.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_flashctl.h>

#define SECTOR_SIZE 1024
#define SETTINGS_MAGIC 0x334B5243u // "CRK3"
#define RECORD_WORDS 6             // three 64-bit flash words

// A whole sector allocated as ordinary const data, so erasing it can never touch code
static const uint32_t settings_sector[SECTOR_SIZE / 4] __attribute__((aligned(SECTOR_SIZE))) = {0};

// Record: magic, threshold | volume << 16, preroll | rest_min << 16, rest_max, 0, check
static uint32_t check(const uint32_t *w) { return ~(w[0] ^ w[1] ^ w[2] ^ w[3] ^ w[4]); }

bool settings_load(settings_t *s) {
  const volatile uint32_t *p = settings_sector;
  uint32_t w[RECORD_WORDS];
  for (int i = 0; i < RECORD_WORDS; i++)
    w[i] = p[i];
  if (w[0] != SETTINGS_MAGIC || w[5] != check(w))
    return false;
  s->threshold = (uint16_t)w[1];
  s->volume = (uint8_t)(w[1] >> 16);
  s->preroll_ms = (uint16_t)w[2];
  s->rest_min_ms = (uint16_t)(w[2] >> 16);
  s->rest_max_ms = (uint16_t)w[3];
  return true;
}

bool settings_save(const settings_t *s) {
  uint32_t addr = (uint32_t)settings_sector;
  uint32_t data[RECORD_WORDS] = {SETTINGS_MAGIC,
                                 s->threshold | (uint32_t)s->volume << 16,
                                 s->preroll_ms | (uint32_t)s->rest_min_ms << 16,
                                 s->rest_max_ms,
                                 0,
                                 0};
  data[5] = check(data);

  // Flash can't be read while a command runs, so no ISR may fetch from it
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  DL_FlashCTL_executeClearStatus(FLASHCTL);
  DL_FlashCTL_unprotectSector(FLASHCTL, addr, DL_FLASHCTL_REGION_SELECT_MAIN);
  bool ok = DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL, addr, DL_FLASHCTL_COMMAND_SIZE_SECTOR) ==
            DL_FLASHCTL_COMMAND_STATUS_PASSED;
  for (int i = 0; ok && i < RECORD_WORDS; i += 2) {
    DL_FlashCTL_executeClearStatus(FLASHCTL);
    DL_FlashCTL_unprotectSector(FLASHCTL, addr, DL_FLASHCTL_REGION_SELECT_MAIN);
    ok = DL_FlashCTL_programMemoryFromRAM64(FLASHCTL, addr + 4 * i, &data[i]) == DL_FLASHCTL_COMMAND_STATUS_PASSED;
  }

  __set_PRIMASK(primask);
  return ok;
}
