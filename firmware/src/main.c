#include <ti/devices/msp/msp.h>
#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/m0p/dl_sysctl.h>
#define LED_R_PIN (1 << 23)
#define LED_R_IOMUX (IOMUX_PINCM24)

#define LED_G_PIN (1 << 24)
#define LED_G_IOMUX (IOMUX_PINCM25)

#define LED_B_PIN (1 << 25)
#define LED_B_IOMUX (IOMUX_PINCM26)

int main(void) {

  DL_GPIO_reset(GPIOA);
  DL_GPIO_enablePower(GPIOA);
  DL_Common_delayCycles(16);

  DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
  DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
  DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

  DL_GPIO_initDigitalOutput(IOMUX_PINCM24);
  DL_GPIO_initDigitalOutput(IOMUX_PINCM25);
  DL_GPIO_initDigitalOutput(IOMUX_PINCM26);
  DL_GPIO_enableOutput(GPIOA, LED_R_PIN | LED_G_PIN | LED_B_PIN);
  DL_GPIO_setPins(GPIOA, LED_R_PIN | LED_G_PIN | LED_B_PIN);

  while (1) {

    DL_GPIO_setPins(GPIOA, LED_B_PIN);
    DL_GPIO_clearPins(GPIOA, LED_R_PIN);
    DL_Common_delayCycles(4000000);
    DL_GPIO_setPins(GPIOA, LED_R_PIN);
    DL_GPIO_clearPins(GPIOA, LED_G_PIN);
    DL_Common_delayCycles(4000000);
    DL_GPIO_setPins(GPIOA, LED_G_PIN);
    DL_GPIO_clearPins(GPIOA, LED_B_PIN);
    DL_Common_delayCycles(4000000);
  }
}