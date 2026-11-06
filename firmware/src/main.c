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

// Live configuration: host writes update it, commit saves it
static settings_t cfg = {.threshold = DEFAULT_THRESHOLD,
                         .volume = DEFAULT_VOLUME,
                         .preroll_ms = DEFAULT_PREROLL_MS,
                         .rest_min_ms = DEFAULT_REST_MIN_MS,
                         .rest_max_ms = DEFAULT_REST_MAX_MS};

static uint16_t get16(uint8_t reg) {
  uint8_t b[2];
  i2c_target_get(reg, b, sizeof b);
  return (uint16_t)(b[0] | (b[1] << 8));
}

static void set16(uint8_t reg, uint16_t v) {
  uint8_t b[2] = {(uint8_t)v, (uint8_t)(v >> 8)};
  i2c_target_set(reg, b, sizeof b);
}

static void apply_cfg(void) {
  if (cfg.rest_max_ms < cfg.rest_min_ms)
    cfg.rest_max_ms = cfg.rest_min_ms;
  cricket_set_threshold(cfg.threshold);
  cricket_set_timing(cfg.preroll_ms, cfg.rest_min_ms, cfg.rest_max_ms);
  audio_set_volume(cfg.volume);
  set16(REG_THRESHOLD, cfg.threshold);
  i2c_target_set(REG_VOLUME, &cfg.volume, 1);
  set16(REG_PREROLL, cfg.preroll_ms);
  set16(REG_REST_MIN, cfg.rest_min_ms);
  set16(REG_REST_MAX, cfg.rest_max_ms); // shows the host any correction made above
}

static void handle_host_write(void) {
  uint8_t rgb[3], commit;
  i2c_target_get(REG_LED, rgb, sizeof rgb);
  i2c_target_get(REG_VOLUME, &cfg.volume, 1);
  i2c_target_get(REG_COMMIT, &commit, 1);
  cfg.threshold = get16(REG_THRESHOLD);
  cfg.preroll_ms = get16(REG_PREROLL);
  cfg.rest_min_ms = get16(REG_REST_MIN);
  cfg.rest_max_ms = get16(REG_REST_MAX);

  apply_cfg();
  cricket_set_led(rgb[0], rgb[1], rgb[2]);

  if (commit) {
    if (commit == COMMIT_KEY)
      settings_save(&cfg);
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

  settings_load(&cfg);
  i2c_target_init();
  apply_cfg();

  mic_init();
  selftest_run();

  uint32_t last_fft = 0, last_regs = 0, last_load = 0, last_cycles = 0, last_stalls = 0;
  bool fft_pending = false;
  while (1) {
    cricket_poll();

    if (i2c_target_host_wrote())
      handle_host_write();

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
