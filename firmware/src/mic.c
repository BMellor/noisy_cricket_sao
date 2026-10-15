#include "mic.h"
#include "audio.h"
#include "pins.h"
#include "timebase.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_spi.h>

#define SPI_SCR                                                                                                        \
  3 // 24 MHz / ((1 + 3) * 2) = 3 MHz PDM clock (standard mode; 750 kHz low-power mode measured ~10-15 dB noisier)
#define SPI_FIFO_DEPTH 4
#define WORDS_PER_SAMPLE 8 // CIC decimation: 187.5 kHz words -> 23.4375 kHz samples
#define CIC_GAIN (WORDS_PER_SAMPLE * WORDS_PER_SAMPLE * WORDS_PER_SAMPLE)
#define ISR_ENTRY_EXIT_CYCLES 26 // exception entry/exit, not seen by the timer measurement
#ifndef MIC_PROFILE
#define MIC_PROFILE 1 // ISR load/stall counters (~2-3% of CPU)
#endif

#define DC_SHIFT 8    // ~15 Hz high-pass
#define FAST_SHIFT 10 // 1024 samples, ~44 ms
#define SLOW_DECIM 16
#define SLOW_SHIFT 14   // 16 * 16384 samples, ~11 s
#define LEVEL_SAT 32767 // level path clips at -6 dBFS; only "how quiet" matters there
#define FFT_SAT 16383   // FFT input clips at -12 dBFS, keeping quiet-room bins above its rounding floor

// 4th-order Butterworth low-pass at 2.5 kHz (keeps the chirp band out of the level), two Q14 biquads with
// b = b0 * [1 2 1]. From matlab/design_lowpass.m.
#define LP1_B0 1432
#define LP1_A1 -20748
#define LP1_A2 10091
#define LP2_B0 1126
#define LP2_A1 -16317
#define LP2_A2 4436

static const uint8_t popcount8[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2,
    3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3,
    3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5,
    6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4,
    3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4,
    5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6,
    6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

static struct {
  uint32_t i1, i2, i3; // CIC integrators, modular arithmetic
  uint32_t c1, c2, c3; // CIC comb delays
  int32_t dc;          // DC estimate << DC_SHIFT
  int32_t x1, x2;      // low-pass input history
  int32_t m1, m2;      // between the two sections
  int32_t y1, y2;      // low-pass output history
  volatile uint32_t fast, slow;
  volatile uint32_t entropy;
  int16_t *volatile cap_buf;
  volatile uint8_t cap_idx, cap_len;
  volatile uint32_t busy_cycles, stalls, entries;
  volatile bool blanked;
  uint32_t phase; // words left until the next output sample
  uint8_t slow_count;
} s;

static inline int32_t sat(int32_t v, int32_t limit) {
  if (v > limit)
    return limit;
  if (v < -limit)
    return -limit;
  return v;
}

static void process_sample(uint32_t v, uint32_t w) {
  uint32_t e = s.entropy;
  s.entropy = ((e << 1) | (e >> 31)) ^ w;

  uint32_t d1 = v - s.c1;
  s.c1 = v;
  uint32_t d2 = d1 - s.c2;
  s.c2 = d1;
  uint32_t d3 = d2 - s.c3;
  s.c3 = d2;

  // CIC output is 0..16 * CIC_GAIN (popcount isn't centred); scale so it's +/-65536 after the DC blocker
  int32_t x = (int32_t)d3 * (8192 / CIC_GAIN);
  s.dc += x - (s.dc >> DC_SHIFT);
  int32_t y = x - (s.dc >> DC_SHIFT);

  audio_tick();

  uint8_t i = s.cap_idx;
  if (i < s.cap_len) {
    s.cap_buf[i] = (int16_t)sat(y, FFT_SAT);
    s.cap_idx = i + 1;
  }

  // While blanked (chirp/beep playing) the level is frozen, so skip the low-pass too (~1/3 of the ISR).
  // It resumes from its pre-blank state; in a quiet room that mismatch is tiny.
  if (s.blanked)
    return;

  // Section 2's input history is section 1's output history (m1, m2), so read both before shifting
  int32_t u = sat(y, LEVEL_SAT);
  int32_t m = sat((LP1_B0 * (u + 2 * s.x1 + s.x2) - LP1_A1 * s.m1 - LP1_A2 * s.m2) >> 14, LEVEL_SAT);
  int32_t z = sat((LP2_B0 * (m + 2 * s.m1 + s.m2) - LP2_A1 * s.y1 - LP2_A2 * s.y2) >> 14, LEVEL_SAT);
  s.x2 = s.x1;
  s.x1 = u;
  s.m2 = s.m1;
  s.m1 = m;
  s.y2 = s.y1;
  s.y1 = z;

  uint32_t a = (uint32_t)(z < 0 ? -z : z);
  uint32_t f = s.fast;
  f += a - (f >> FAST_SHIFT);
  s.fast = f;
  if (++s.slow_count >= SLOW_DECIM) {
    s.slow_count = 0;
    uint32_t sl = s.slow;
    sl += (f >> FAST_SHIFT) - (sl >> SLOW_SHIFT);
    s.slow = sl;
  }
}

// Every RX word is matched by one TX write, so words-in-flight stays at SPI_FIFO_DEPTH. If the ISR is
// held off (e.g. flash write) the TX FIFO drains, the clock stops, and the pending RX event restarts it.
void SPI0_IRQHandler(void) {
#if MIC_PROFILE
  uint32_t t0 = timebase_ticks();
  s.entries++;
  if (SPI0->STAT & SPI_STAT_TFE_MASK)
    s.stalls++; // the clock may already have stopped
#endif
  (void)SPI0->CPU_INT.IIDX; // reading IIDX clears the RX event

  uint32_t i1 = s.i1, i2 = s.i2, i3 = s.i3, phase = s.phase;
  while (!(SPI0->STAT & SPI_STAT_RFE_MASK)) {
    uint16_t w = (uint16_t)SPI0->RXDATA;
    SPI0->TXDATA = 0;
    i1 += (uint32_t)popcount8[w & 0xFF] + popcount8[w >> 8];
    i2 += i1;
    i3 += i2;
    if (--phase == 0) {
      phase = WORDS_PER_SAMPLE;
      process_sample(i3, w);
    }
  }
  s.i1 = i1;
  s.i2 = i2;
  s.i3 = i3;
  s.phase = phase;

#if MIC_PROFILE
  int32_t d = (int32_t)timebase_ticks() - (int32_t)t0;
  if (d < 0)
    d += TIMEBASE_TICKS_PER_MS;
  s.busy_cycles += (uint32_t)d * TIMEBASE_CYCLES_PER_TICK + ISR_ENTRY_EXIT_CYCLES;
#endif
}

void mic_init(void) {
  s.phase = WORDS_PER_SAMPLE;
  s.dc = 65536 << DC_SHIFT; // popcount midscale (8 per word) after scaling: start settled

  DL_SPI_reset(SPI0);
  DL_SPI_enablePower(SPI0);
  DL_Common_delayCycles(16);

  DL_GPIO_initPeripheralOutputFunction(PDM_CLK.iomux, IOMUX_PINCM18_PF_SPI0_SCLK);
  DL_GPIO_initPeripheralInputFunction(PDM_DATA.iomux, IOMUX_PINCM23_PF_SPI0_POCI);

  DL_SPI_ClockConfig clkCfg = {
      .clockSel = DL_SPI_CLOCK_BUSCLK,
      .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1,
  };
  DL_SPI_setClockConfig(SPI0, &clkCfg);

  DL_SPI_Config spiCfg = {
      .mode = DL_SPI_MODE_CONTROLLER,
      .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0, // L/R = GND: mic drives on falling edge, sample on rising
      .parity = DL_SPI_PARITY_NONE,
      .dataSize = DL_SPI_DATA_SIZE_16,
      .bitOrder = DL_SPI_BIT_ORDER_MSB_FIRST,
      .chipSelectPin = DL_SPI_CHIP_SELECT_NONE,
  };
  DL_SPI_init(SPI0, &spiCfg);
  DL_SPI_setBitRateSerialClockDivider(SPI0, SPI_SCR);
  DL_SPI_setFIFOThreshold(SPI0, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

  DL_SPI_enableInterrupt(SPI0, DL_SPI_INTERRUPT_RX);
  NVIC_SetPriority(SPI0_INT_IRQn, 0);
  NVIC_EnableIRQ(SPI0_INT_IRQn);

  DL_SPI_enable(SPI0);
  for (int i = 0; i < SPI_FIFO_DEPTH; i++)
    DL_SPI_transmitData16(SPI0, 0);
}

void mic_set_blanking(bool blank) { s.blanked = blank; }

uint16_t mic_level_fast(void) { return (uint16_t)(s.fast >> FAST_SHIFT); }

uint16_t mic_level_slow(void) { return (uint16_t)(s.slow >> SLOW_SHIFT); }

uint32_t mic_entropy(void) { return s.entropy; }

void mic_capture_start(int16_t *buf, uint8_t len) {
  s.cap_idx = len; // park the ISR while the buffer pointer changes
  s.cap_buf = buf;
  s.cap_len = len;
  s.cap_idx = 0;
}

bool mic_capture_done(void) { return s.cap_idx >= s.cap_len; }

uint32_t mic_isr_cycles(void) { return s.busy_cycles; }

uint32_t mic_stalls(void) { return s.stalls; }
