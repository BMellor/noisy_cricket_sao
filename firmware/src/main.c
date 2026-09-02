#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/m0p/dl_sysctl.h>

#include "pins.h"

int main(void) {

  DL_GPIO_reset(GPIOA);
  DL_GPIO_enablePower(GPIOA);
  DL_Common_delayCycles(16);

  DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
  DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
  DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

  DL_GPIO_initDigitalOutput(LED_R.iomux);
  DL_GPIO_initDigitalOutput(LED_G.iomux);
  DL_GPIO_initDigitalOutput(LED_B.iomux);
  DL_GPIO_enableOutput(LED_R.port, LED_R.pinmask);
  DL_GPIO_enableOutput(LED_G.port, LED_G.pinmask);
  DL_GPIO_enableOutput(LED_B.port, LED_B.pinmask);
  DL_GPIO_setPins(LED_R.port, LED_R.pinmask);
  DL_GPIO_setPins(LED_G.port, LED_G.pinmask);
  DL_GPIO_setPins(LED_B.port, LED_B.pinmask);

  while (1) {

    DL_GPIO_setPins(LED_B.port, LED_B.pinmask);
    DL_GPIO_clearPins(LED_R.port, LED_R.pinmask);
    DL_Common_delayCycles(4000000);
    DL_GPIO_setPins(LED_R.port, LED_R.pinmask);
    DL_GPIO_clearPins(LED_G.port, LED_G.pinmask);
    DL_Common_delayCycles(4000000);
    DL_GPIO_setPins(LED_B.port, LED_B.pinmask);
    DL_GPIO_clearPins(LED_B.port, LED_B.pinmask);
    DL_Common_delayCycles(4000000);
  }
}