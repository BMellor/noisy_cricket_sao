#include "led.h"
#include "pins.h"
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_timerg.h>

// gamma = 2.8
static const uint8_t gamma8[256] = {
    0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,
    2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,
    6,   7,   7,   7,   7,   8,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  12,  12,  12,  13,  13,
    13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,
    25,  25,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  34,  34,  35,  36,  37,  38,  38,  39,  40,
    41,  42,  43,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  62,
    63,  64,  65,  66,  67,  68,  70,  71,  72,  73,  75,  76,  77,  78,  80,  81,  82,  84,  85,  87,  88,  89,
    91,  92,  94,  95,  97,  98,  100, 101, 103, 104, 106, 108, 109, 111, 112, 114, 116, 117, 119, 121, 123, 124,
    126, 128, 130, 131, 133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167,
    169, 171, 173, 176, 178, 180, 182, 185, 187, 189, 192, 194, 196, 199, 201, 203, 206, 208, 211, 213, 216, 218,
    221, 223, 226, 228, 231, 234, 236, 239, 242, 244, 247, 250, 253, 255};

static inline void set_red(uint8_t value) {
  DL_TimerG_setCaptureCompareValue(TIMG14, gamma8[value], DL_TIMER_CC_0_INDEX);
}

static inline void set_green(uint8_t value) {
  DL_TimerG_setCaptureCompareValue(TIMG14, gamma8[value], DL_TIMER_CC_1_INDEX);
}

static inline void set_blue(uint8_t value) {
  DL_TimerG_setCaptureCompareValue(TIMG14, gamma8[value], DL_TIMER_CC_3_INDEX);
}

void led_init(void) {
  // Power up & reset the timer
  DL_TimerG_reset(TIMG14);
  DL_TimerG_enablePower(TIMG14);

  // Initialize GPIO to alternate function
  DL_GPIO_initPeripheralOutputFunction(LED_R.iomux, IOMUX_PINCM24_PF_TIMG14_CCP0);
  DL_GPIO_initPeripheralOutputFunction(LED_G.iomux, IOMUX_PINCM25_PF_TIMG14_CCP1);
  DL_GPIO_initPeripheralOutputFunction(LED_B.iomux, IOMUX_PINCM26_PF_TIMG14_CCP3);

  // Setup TIMG14 for ~1 kHz 8-bit PWM
  // Timer clock configuration to be sourced by BUSCLK (24000000 Hz)
  // timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
  //  255.3 kHz = 24 MHz / (1 * (93 + 1))
  DL_TimerG_ClockConfig gClockConfig = {
      .clockSel = DL_TIMER_CLOCK_BUSCLK,
      .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
      .prescale = 93U,
  };
  DL_TimerG_setClockConfig(TIMG14, (DL_TimerG_ClockConfig *)&gClockConfig);

  // Setup capture/compare channels for PWM
  DL_TimerG_PWMConfig pwmConfig = {
      .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
      .period = 255,
      .isTimerWithFourCC = true, // TIMG14 has 4 CC channels
      .startTimer = DL_TIMER_STOP,
  };
  DL_TimerG_initPWMMode(TIMG14, &pwmConfig);

  // CC0 = Red, CC1 = Green, CC3 = Blue
  DL_TimerG_setCCPDirection(TIMG14, DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT | DL_TIMER_CC3_OUTPUT);

  set_red(0);
  set_green(0);
  set_blue(0);

  DL_TimerG_enableClock(TIMG14);
  DL_TimerG_startCounter(TIMG14);
}

void led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
  set_red(r);
  set_green(g);
  set_blue(b);
}

// Set channels as output compare, no zero, load, advance, or capture conditions (used with input capture)
//   DL_TimerG_setCaptureCompareCtl(TIMG14, DL_TIMER_CC_MODE_COMPARE, DL_TIMER_CC_ZCOND_NONE, DL_TIMER_CC_0_INDEX);
//   DL_TimerG_setCaptureCompareCtl(TIMG14, DL_TIMER_CC_MODE_COMPARE, DL_TIMER_CC_ZCOND_NONE, DL_TIMER_CC_1_INDEX);
//   DL_TimerG_setCaptureCompareCtl(TIMG14, DL_TIMER_CC_MODE_COMPARE, DL_TIMER_CC_ZCOND_NONE, DL_TIMER_CC_3_INDEX);

//   // Set pins high when output disabled, no inversion, function/PWM mode
//   DL_TimerG_setCaptureCompareOutCtl(TIMG14, DL_TIMER_CC_OCTL_INIT_VAL_HIGH, DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
//                                     DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMER_CC_0_INDEX);
//   DL_TimerG_setCaptureCompareOutCtl(TIMG14, DL_TIMER_CC_OCTL_INIT_VAL_HIGH, DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
//                                     DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMER_CC_1_INDEX);
//   DL_TimerG_setCaptureCompareOutCtl(TIMG14, DL_TIMER_CC_OCTL_INIT_VAL_HIGH, DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
//                                     DL_TIMER_CC_OCTL_SRC_FUNCVAL, DL_TIMER_CC_3_INDEX);

//   // Set pin low on downcount match, high on timer reload
//   DL_TimerG_setCaptureCompareAction(TIMG14, DL_TIMER_CC_CDACT_CCP_LOW | DL_TIMER_CC_LACT_CCP_HIGH,
//   DL_TIMER_CC_0_INDEX); DL_TimerG_setCaptureCompareAction(TIMG14, DL_TIMER_CC_CDACT_CCP_LOW |
//   DL_TIMER_CC_LACT_CCP_HIGH, DL_TIMER_CC_1_INDEX); DL_TimerG_setCaptureCompareAction(TIMG14,
//   DL_TIMER_CC_CDACT_CCP_LOW | DL_TIMER_CC_LACT_CCP_HIGH, DL_TIMER_CC_3_INDEX);