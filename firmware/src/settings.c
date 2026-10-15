#include "settings.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_flashctl.h>

#define SECTOR_SIZE 1024
#define SETTINGS_MAGIC 0x324B5243u // "CRK2"

// A whole sector allocated as ordinary const data, so erasing it can never touch code
static const uint32_t settings_sector[SECTOR_SIZE / 4] __attribute__((aligned(SECTOR_SIZE))) = {0};

// Record: magic, threshold, volume, check (two 64-bit flash words)
static uint32_t check(uint32_t threshold, uint32_t volume) { return ~(SETTINGS_MAGIC ^ threshold ^ (volume << 16)); }

bool settings_load(settings_t *s) {
  const volatile uint32_t *p = settings_sector;
  uint32_t thr = p[1], vol = p[2];
  if (p[0] != SETTINGS_MAGIC || thr > 0xFFFF || vol > 0xFF || p[3] != check(thr, vol))
    return false;
  s->threshold = (uint16_t)thr;
  s->volume = (uint8_t)vol;
  return true;
}

bool settings_save(const settings_t *s) {
  uint32_t addr = (uint32_t)settings_sector;
  uint32_t data[4] = {SETTINGS_MAGIC, s->threshold, s->volume, check(s->threshold, s->volume)};

  // Flash can't be read while a command runs, so no ISR may fetch from it
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  DL_FlashCTL_executeClearStatus(FLASHCTL);
  DL_FlashCTL_unprotectSector(FLASHCTL, addr, DL_FLASHCTL_REGION_SELECT_MAIN);
  bool ok = DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL, addr, DL_FLASHCTL_COMMAND_SIZE_SECTOR) ==
            DL_FLASHCTL_COMMAND_STATUS_PASSED;
  if (ok) {
    DL_FlashCTL_executeClearStatus(FLASHCTL);
    DL_FlashCTL_unprotectSector(FLASHCTL, addr, DL_FLASHCTL_REGION_SELECT_MAIN);
    ok = DL_FlashCTL_programMemoryFromRAM64(FLASHCTL, addr, &data[0]) == DL_FLASHCTL_COMMAND_STATUS_PASSED;
  }
  if (ok) {
    DL_FlashCTL_executeClearStatus(FLASHCTL);
    DL_FlashCTL_unprotectSector(FLASHCTL, addr, DL_FLASHCTL_REGION_SELECT_MAIN);
    ok = DL_FlashCTL_programMemoryFromRAM64(FLASHCTL, addr + 8, &data[2]) == DL_FLASHCTL_COMMAND_STATUS_PASSED;
  }

  __set_PRIMASK(primask);
  return ok;
}
