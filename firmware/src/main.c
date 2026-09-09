#include <ti/devices/msp/msp.h>
#include <ti/driverlib/m0p/dl_sysctl.h>

#include <ti/driverlib/dl_gpio.h>
#include <ti/driverlib/dl_spi.h>

#include "audio.h"
#include "led.h"
#include "mic.h"
#include "pins.h"
#include "timebase.h"

// // Index = byte captured from SPI (8 raw PDM bits), value = popcount (0-8)
// static const uint8_t popcount_lut[256] = {
//     0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2,
//     3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3,
//     3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5,
//     6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4,
//     3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4,
//     5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6,
//     6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

// // Index = popcount (0-8, number of set bits in captured PDM byte)
// // Symmetric: cool blue at center (4), warm red at both extremes (0 and 8)
// static const uint8_t rgb_lut[9][3] = {
//     {0xFF, 0x00, 0x00}, // 0: red
//     {0xFF, 0xD4, 0x00}, // 1: amber
//     {0x54, 0xFF, 0x00}, // 2: yellow-green
//     {0x00, 0xFF, 0x7F}, // 3: spring green
//     {0x00, 0xA9, 0xFF}, // 4: cyan-blue (center)
//     {0x00, 0xFF, 0x7F}, // 5: spring green
//     {0x54, 0xFF, 0x00}, // 6: yellow-green
//     {0xFF, 0xD4, 0x00}, // 7: amber
//     {0xFF, 0x00, 0x00}, // 8: red
// };

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
  audio_init();
  led_init();
  // mic_init();

  audio_enable(true);

  // DL_SPI_reset(SPI0);
  // DL_SPI_enablePower(SPI0);

  // DL_GPIO_initPeripheralOutputFunction(PDM_CLK.iomux, IOMUX_PINCM18_PF_SPI0_SCLK);
  // DL_GPIO_initPeripheralInputFunction(PDM_DATA.iomux, IOMUX_PINCM23_PF_SPI0_POCI);

  // DL_SPI_ClockConfig clkCfg = {
  //     .clockSel = DL_SPI_CLOCK_BUSCLK,
  //     .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1,
  // };
  // DL_SPI_setClockConfig(SPI0, (DL_SPI_ClockConfig *)&clkCfg);

  // DL_SPI_Config spiCfg = {
  //     .mode = DL_SPI_MODE_CONTROLLER,
  //     .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0,
  //     .parity = DL_SPI_PARITY_NONE,
  //     .dataSize = DL_SPI_DATA_SIZE_8,
  //     .bitOrder = DL_SPI_BIT_ORDER_MSB_FIRST,
  //     .chipSelectPin = DL_SPI_CHIP_SELECT_NONE, // CS unused
  // };
  // DL_SPI_init(SPI0, &spiCfg);

  // DL_SPI_setBitRateSerialClockDivider(SPI0, 12); // 2MHz, could probably go to 3
  // DL_SPI_enable(SPI0);

  // chirp();

  // DL_GPIO_initDigitalOutput(LED_R.iomux);
  // DL_GPIO_enableOutput(LED_R.port, LED_R.pinmask);
  // DL_GPIO_setPins(LED_R.port, LED_R.pinmask);

  while (1) {
    chirp();
    delay_ms(20);
    chirp();
    delay_ms(20);
    chirp();
    delay_ms(500);

    chirp();
    delay_ms(20);
    chirp();
    delay_ms(20);
    chirp();
    delay_ms(6000);

    chirp();
    delay_ms(20);
    chirp();
    delay_ms(20);
    chirp();
    delay_ms(500);

    chirp();
    delay_ms(20);
    chirp();
    delay_ms(20);
    chirp();
    delay_ms(500);

    chirp();
    delay_ms(20);
    chirp();
    delay_ms(20);
    chirp();
    delay_ms(10000);

    // led_set_red(255);
    // delay_ms(1000);
    // led_set_red(0);
    // DL_GPIO_togglePins(LED_R.port, LED_R.pinmask);

    // LED only updates at around 1kHz, need to accumulate and average samples
    // until we get to roughly the update rate

    // uint16_t accu = 0;
    // for (uint8_t count = 0; count < 255; count++) {
    //   DL_SPI_transmitDataBlocking8(SPI0, 0xFF);
    //   uint8_t rx = DL_SPI_receiveDataBlocking8(SPI0);
    //   accu += popcount_lut[rx];
    // }

    // uint8_t idx = accu / 255;
    // led_set_rgb(rgb_lut[idx][0], rgb_lut[idx][1], rgb_lut[idx][2]);
  }
}