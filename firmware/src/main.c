#include <ti/devices/msp/msp.h>
#include <ti/driverlib/m0p/dl_sysctl.h>

#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_timerg.h>

#include "pins.h"

// void led_init(void) {
//   DL_GPIO_initPeripheralOutputFunction(LED_R.iomux, IOMUX_PINCM24_PF_TIMG14_CCP0);
//   DL_GPIO_initPeripheralOutputFunction(LED_G.iomux, IOMUX_PINCM25_PF_TIMG14_CCP1);
//   DL_GPIO_initPeripheralOutputFunction(LED_B.iomux, IOMUX_PINCM26_PF_TIMG14_CCP3);

//   // Setup TIMG14 for ~976Hz PWM
//   // Timer clock configuration to be sourced by BUSCLK (24000000 Hz)
//   // timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
//   //   24 kHz = 24000000 Hz / (1 * (23 + 1))
//   DL_TimerG_ClockConfig gClockConfig = {
//       .clockSel    = DL_TIMER_CLOCK_BUSCLK,
//       .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
//       .prescale    = 23U,
//   };

//   // Downcounting, preload to 10-bit value, set timer enable
//   DL_TimerG_TimerConfig gTimerConfig = {
//       .period     = 1024U,
//       .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
//       .startTimer = DL_TIMER_START,
//   };
//   DL_TimerG_enablePower(TIMG14);
//   DL_TimerG_setClockConfig(TIMG14, (DL_TimerG_ClockConfig *) &gClockConfig);
//   DL_TimerG_initTimerMode(TIMG14, (DL_TimerG_TimerConfig *) &gTimerConfig);

//   // Start timer/PWM
//   DL_TimerG_enableClock(TIMG14);
// }

int main(void) {

  // Setup clock for maximum 24MHz
  DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
  DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
  DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

  // Power and reset GPIOA (all pins are in GPIOA)
  DL_GPIO_reset(GPIOA);
  DL_GPIO_enablePower(GPIOA);
  DL_Common_delayCycles(16);

  // led_init();

  // DL_GPIO_initDigitalOutput(LED_R.iomux);
  // DL_GPIO_initDigitalOutput(LED_G.iomux);
  // DL_GPIO_initDigitalOutput(LED_B.iomux);
  // DL_GPIO_enableOutput(LED_R.port, LED_R.pinmask);
  // DL_GPIO_enableOutput(LED_G.port, LED_G.pinmask);
  // DL_GPIO_enableOutput(LED_B.port, LED_B.pinmask);
  // DL_GPIO_setPins(LED_R.port, LED_R.pinmask);
  // DL_GPIO_setPins(LED_G.port, LED_G.pinmask);
  // DL_GPIO_setPins(LED_B.port, LED_B.pinmask);

  while (1) {
    // DL_GPIO_setPins(LED_B.port, LED_B.pinmask);
    // DL_GPIO_clearPins(LED_R.port, LED_R.pinmask);
    // DL_Common_delayCycles(4000000);
    // DL_GPIO_setPins(LED_R.port, LED_R.pinmask);
    // DL_GPIO_clearPins(LED_G.port, LED_G.pinmask);
    // DL_Common_delayCycles(4000000);
    // DL_GPIO_setPins(LED_B.port, LED_B.pinmask);
    // DL_GPIO_clearPins(LED_B.port, LED_B.pinmask);
    DL_Common_delayCycles(4000000);
  }
}