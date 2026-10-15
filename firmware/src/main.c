#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/m0p/dl_sysctl.h>

#include "audio.h"
#include "config.h"
#include "cricket.h"
#include "fft.h"
#include "i2c_target.h"
#include "led.h"
#include "mic.h"
#include "selftest.h"
#include "settings.h"
#include "timebase.h"

static void handle_host_write(void) {
  uint8_t t[2], rgb[3], volume, commit;
  i2c_target_get(REG_THRESHOLD, t, sizeof t);
  i2c_target_get(REG_LED, rgb, sizeof rgb);
  i2c_target_get(REG_VOLUME, &volume, 1);
  i2c_target_get(REG_COMMIT, &commit, 1);

  cricket_set_threshold((uint16_t)(t[0] | (t[1] << 8)));
  cricket_set_led(rgb[0], rgb[1], rgb[2]);
  audio_set_volume(volume);

  if (commit) {
    if (commit == COMMIT_KEY)
      settings_save(&(settings_t){.threshold = cricket_threshold(), .volume = audio_volume()});
    commit = 0;
    i2c_target_set(REG_COMMIT, &commit, 1);
  }
}

static void publish_status(void) {
  uint16_t fast = mic_level_fast();
  uint16_t slow = mic_level_slow();
  uint8_t status = cricket_status();
  uint8_t lv[4] = {(uint8_t)fast, (uint8_t)(fast >> 8), (uint8_t)slow, (uint8_t)(slow >> 8)};
  i2c_target_set(REG_STATUS, &status, 1);
  i2c_target_set(REG_LEVEL_FAST, lv, sizeof lv);
}

int main(void) {
  // PA1 is wired as I2C SCL; with NRST active, bus traffic would reset the MCU. Locked until next POR.
  DL_SYSCTL_disableNRSTPin();

  DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE); // 24 MHz
  DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
  DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

  DL_GPIO_reset(GPIOA);
  DL_GPIO_enablePower(GPIOA);
  DL_Common_delayCycles(16);

  timebase_init();
  led_init();
  audio_init();

  settings_t cfg = {.threshold = DEFAULT_THRESHOLD, .volume = DEFAULT_VOLUME};
  settings_load(&cfg);
  cricket_init(cfg.threshold);
  audio_set_volume(cfg.volume);

  i2c_target_init();
  uint8_t t[2] = {(uint8_t)cfg.threshold, (uint8_t)(cfg.threshold >> 8)};
  i2c_target_set(REG_THRESHOLD, t, sizeof t);
  i2c_target_set(REG_VOLUME, &cfg.volume, 1);

  mic_init();
  selftest_run();

  uint32_t last_fft = 0, last_regs = 0, last_load = 0, last_cycles = 0, last_stalls = 0;
  bool fft_pending = false;
  while (1) {
    cricket_poll();

    if (i2c_target_host_wrote())
      handle_host_write();
    if (i2c_target_beep_requested()) {
      uint8_t b[4];
      i2c_target_get(REG_BEEP_FREQ, b, sizeof b);
      cricket_beep((uint16_t)(b[0] | (b[1] << 8)), (uint16_t)(b[2] | (b[3] << 8)));
    }

    uint32_t now = get_timestamp_ms();
    if (fft_pending && mic_capture_done()) {
      uint8_t bins[FFT_BINS];
      fft_compute(bins);
      i2c_target_set(REG_FFT, bins, FFT_BINS);
      fft_pending = false;
    } else if (!fft_pending && now - last_fft >= FFT_INTERVAL_MS) {
      last_fft = now;
      mic_capture_start(fft_input(), FFT_N);
      fft_pending = true;
    }

    if (now - last_load >= 1000) {
      uint32_t cycles = mic_isr_cycles(), stalls = mic_stalls();
      uint32_t dt = now - last_load;
      uint8_t lr[2] = {(uint8_t)((cycles - last_cycles) / (24000u / 100u) / dt),
                       (uint8_t)(stalls - last_stalls > 255 ? 255 : stalls - last_stalls)};
      i2c_target_set(REG_ISR_LOAD, lr, sizeof lr);
      last_load = now;
      last_cycles = cycles;
      last_stalls = stalls;
    }

    if (now - last_regs >= REG_UPDATE_MS) {
      last_regs = now;
      publish_status();
    }

    __WFI();
  }
}
