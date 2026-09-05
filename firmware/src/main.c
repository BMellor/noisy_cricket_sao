#include <ti/devices/msp/msp.h>
#include <ti/driverlib/m0p/dl_sysctl.h>

#include <ti/driverlib/dl_gpio.h>

#include "led.h"
#include "pins.h"
#include "timebase.h"

int main(void) {

  // Setup clock for maximum 24MHz
  DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
  DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
  DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

  // Power and reset GPIOA (all pins are in GPIOA)
  DL_GPIO_reset(GPIOA);
  DL_GPIO_enablePower(GPIOA);
  DL_Common_delayCycles(16);

  timebase_init();
  led_init();

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 255;
  uint8_t state = 0;

  // DL_GPIO_initDigitalOutput(LED_B.iomux);
  // DL_GPIO_enableOutput(LED_B.port, LED_B.pinmask);

  while (1) {
    // delay_ms(1000);
    // DL_GPIO_togglePins(LED_B.port, LED_B.pinmask);

    delay_ms(10);
    led_set_rgb(r, g, b);
    switch (state) {
    case 0: // red up
      if (r < 255)
        r++;
      else
        state++;
      break;
    case 1: // blue down
      if (b > 0)
        b--;
      else
        state++;
      break;
    case 2: // green up
      if (g < 255)
        g++;
      else
        state++;
      break;
    case 3: // red down
      if (r > 0)
        r--;
      else
        state++;
      break;
    case 4: // blue up
      if (b < 255)
        b++;
      else
        state++;
      break;
    default: // green down
      if (g > 0)
        g--;
      else
        state = 0;
      break;
    }
  }
}