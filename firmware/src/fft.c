#include "fft.h"

static int16_t re[FFT_N], im[FFT_N];

// sin(k * pi / 32), k = 0..16, Q15
static const int16_t qsin[17] = {0,     3212,  6393,  9512,  12539, 15446, 18204, 20787, 23170,
                                 25329, 27245, 28898, 30273, 31356, 32137, 32609, 32767};

// Periodic Hann, n = 0..32 (symmetric about 32), Q15
static const int16_t hann[33] = {0,     79,    315,   705,   1247,  1935,  2761,  3719,  4799,  5990,  7281,
                                 8660,  10114, 11628, 13187, 14778, 16383, 17989, 19580, 21139, 22653, 24107,
                                 25486, 26777, 27968, 29048, 30006, 30832, 31520, 32062, 32452, 32688, 32767};

int16_t *fft_input(void) { return re; }

static uint8_t bitrev6(uint8_t i) {
  uint8_t r = 0;
  for (int b = 0; b < 6; b++) {
    r = (uint8_t)((r << 1) | (i & 1));
    i >>= 1;
  }
  return r;
}

static uint8_t log2q3(uint32_t p) {
  if (p == 0)
    return 0;
  uint8_t e = 31;
  while (!(p & 0x80000000u)) {
    p <<= 1;
    e--;
  }
  return (uint8_t)((e << 3) | ((p >> 28) & 7));
}

void fft_compute(uint8_t out[FFT_BINS]) {
  for (int n = 0; n < FFT_N; n++) {
    re[n] = (int16_t)((re[n] * hann[n <= 32 ? n : FFT_N - n]) >> 15);
    im[n] = 0;
  }
  for (uint8_t i = 0; i < FFT_N; i++) {
    uint8_t j = bitrev6(i);
    if (j > i) {
      int16_t t = re[i];
      re[i] = re[j];
      re[j] = t;
    }
  }

  // Radix-2 DIT, halved every stage so it can't overflow (output = X / 64)
  for (int len = 2; len <= FFT_N; len <<= 1) {
    int half = len >> 1;
    int step = FFT_N / len;
    for (int j = 0; j < half; j++) {
      int k = j * step; // twiddle angle k * 2pi / 64
      int32_t wr = k <= 16 ? qsin[16 - k] : -qsin[k - 16];
      int32_t wi = k <= 16 ? qsin[k] : qsin[32 - k];
      for (int a = j; a < FFT_N; a += len) {
        int b = a + half;
        int32_t tr = (wr * re[b] + wi * im[b]) >> 15;
        int32_t ti = (wr * im[b] - wi * re[b]) >> 15;
        int32_t ar = re[a], ai = im[a];
        re[b] = (int16_t)((ar - tr) >> 1);
        im[b] = (int16_t)((ai - ti) >> 1);
        re[a] = (int16_t)((ar + tr) >> 1);
        im[a] = (int16_t)((ai + ti) >> 1);
      }
    }
  }

  for (int k = 0; k < FFT_BINS; k++)
    out[k] = log2q3((uint32_t)(re[k] * re[k]) + (uint32_t)(im[k] * im[k]));
}
