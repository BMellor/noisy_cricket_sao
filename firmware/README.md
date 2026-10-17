# Noisy Cricket SAO firmware

Firmware for the Noisy Cricket badge add-on: a small SAO board that listens to the room and, once it has been quiet for a while, chirps like a cricket. Talking, music or other noise makes it go silent again, the same way a real cricket stops when you walk up to it.

It runs on a TI MSPM0C1104 (Cortex-M0+, 24 MHz, 16 KB flash, 1 KB SRAM). A badge host can tune it, read its noise levels and pull a small spectrum over the SAO's I2C pins.

## How it works

1. **Listening.** A PDM MEMS microphone is clocked at 3 MHz from the SPI peripheral. The SPI interrupt turns the 1-bit stream into 23.4375 kHz audio samples, low-pass filters them at 2.5 kHz, and keeps a running average of the level (~44 ms and ~11 s).
2. **Deciding it's quiet.** The main loop compares the fast level against a threshold, with 25% hysteresis. Once it's quiet it waits a fixed 2 s before chirping. That fixed delay is deliberate: units that hear the same noise stop together, so they tend to start together too.
3. **Chirping.** A *chirp* is the sampled cricket call played 3 times, 20 ms apart. A *chirp group* is 1–3 chirps, 300–400 ms apart (random). The microphone is ignored for the whole group plus 100 ms, so the cricket never hears itself. The LED fades green in and out with each chirp.
4. **Repeat or stop.** If the room is still quiet afterwards, it rests for a random 1–5 s and chirps another group. Any noise sends it back to waiting for quiet.

The 2.5 kHz low-pass means the chirp's own frequency band (3.25–4.75 kHz) barely registers. So it ignores other Noisy Crickets, and recordings of itself, while still reacting to voices and music.

### Power-on self-test

At power-on, before normal operation, it plays two chirps, shows white for 0.5 s, then for 2 s colours the LED by microphone level: blue (quiet), then cyan, green, yellow and red (loud). It's a quick check that the speaker, LED and mic all work.

## Hardware connections

| Function | MCU pin | Peripheral |
|---|---|---|
| PDM mic clock / data | PA17 / PA22 | SPI0 SCLK / POCI (3 MHz, mic L/R = GND) |
| Audio PWM | PA28 | TIMA0 CC0, 93.75 kHz, 8-bit |
| Amp shutdown (active high) | PA26 | GPIO, AD8107 class-AB amp |
| RGB LED | PA23 / PA24 / PA25 | TIMG14 CC0 / CC1 / CC3 |
| I2C SDA / SCL | PA0 / PA1 | I2C0 target |

PA1 is also the NRST pin. Firmware disables the reset function at boot so I2C traffic can't reset the chip, which means there is no pin reset while it's running. Power-cycle it, or reset it over SWD.

## Code structure

All source is in `src/`. Each `.c` file must also be listed in `src/mod.rs` for the build system to compile it.

| File | Role |
|---|---|
| `main.c` | Clock/GPIO setup, module init, self-test call, and the main loop: runs the scheduler, applies I2C writes, schedules FFTs, publishes status registers. |
| `config.h` | All tunable constants: chirp timing, defaults for the I2C-tunable settings (threshold, volume, pre-roll, rest times), I2C address. |
| `mic.c` | SPI setup and the SPI0 interrupt, which does all per-sample work: popcount, CIC decimation, DC blocker, 2.5 kHz low-pass, level averages, audio step and FFT capture. |
| `audio.c` | Audio PWM and amp control. Plays the chirp table one sample per mic-ISR tick, with the volume applied. |
| `chirp_lut.h` | The chirp sample table (23.4375 kHz, 8-bit). Same layout `matlab/gen_chirp_lut.m` writes. |
| `cricket.c` | Behaviour state machine: idle → armed → (rest) → chirp group → settle. Also the PRNG and the LED (idle colour, green chirp fade). |
| `fft.c` | 64-point windowed FFT, giving 32 log-magnitude bins for the I2C spectrum. |
| `i2c_target.c` | I2C target and the register file (see below). |
| `settings.c` | Saves threshold and volume to a dedicated 1 KB flash sector, and loads them at boot. |
| `selftest.c` | Power-on self-test. |
| `led.c`, `timebase.c`, `pins.h` | RGB LED PWM with gamma correction, 1 ms tick (TIMG8), and pin map. |

### Timing and interrupts

All timing comes from one 24 MHz clock, so the mic, audio output and PWM never drift relative to each other.

- **Mic interrupt (SPI0, priority 0):** fires whenever two words are waiting in the receive FIFO. Each sample also steps the audio, so playback is locked to the mic's clock. It uses about 78% of the CPU while listening, and about 65% while a chirp group plays, when the level filtering is skipped.
- **1 ms tick (TIMG8, priority 1).**
- **I2C target (priority 2).**

The main loop handles everything else and sleeps (`WFI`) between interrupts.

`muls` takes 32 cycles on this core, and SRAM is 1 KB. That's why the per-sample code avoids runtime multiplies where it can, and why the C runtime's stdio and startup files are left out of the link (see `makefile.rs`).

## I2C register map

7-bit address **0x42** (`I2C_TARGET_ADDR` in `config.h`). Write the register address first, then read or write any number of bytes; the address auto-increments. Multi-byte values are little-endian. Registers past 0x35 read as 0xFF. Writes to read-only registers are ignored.

**Settings (0x02–0x0B)** are applied when the write transaction finishes and saved to flash only by COMMIT. They're contiguous so a host can write all of them, with COMMIT last, in one transaction.

| Addr | Name | Access | Description |
|---|---|---|---|
| 0x00 | ID | R | Always 0xC7. |
| 0x01 | STATUS | R | bit 0 quiet, bit 1 armed (waiting to chirp, or resting between groups), bit 2 chirping (a chirp group is in progress, mic ignored). |
| 0x02–0x03 | THRESHOLD | R/W | u16 noise threshold in level units. Quiet below this; loud again above threshold + threshold/4. Default 75. |
| 0x04–0x05 | PREROLL | R/W | u16 ms from the room going quiet to the first chirp. Default 2000. Keep it the same on all units: it's what lines their chirping up. |
| 0x06–0x07 | REST_MIN | R/W | u16 ms, shortest random rest between chirp groups while it stays quiet. Default 1000. |
| 0x08–0x09 | REST_MAX | R/W | u16 ms, longest random rest. Default 5000. If written below REST_MIN it's raised to REST_MIN (read it back to see). |
| 0x0A | VOLUME | R/W | u8 chirp volume, 0 = mute, 255 = full table level. Default 160. |
| 0x0B | COMMIT | W | Write 0xA5 to save all the settings above to flash. Reads 0. The mic and audio pause for a few ms while flash is written. |
| 0x0C–0x0E | LED_R, LED_G, LED_B | R/W | Idle LED colour, 0–255 per channel (gamma corrected; default off). The chirp's green fade is drawn on top. Applied when the write finishes; not saved by COMMIT. |
| 0x0F | — | — | Reserved. |
| 0x10–0x11 | LEVEL_FAST | R | u16 current noise level (~44 ms average). This is what's compared against THRESHOLD. |
| 0x12–0x13 | LEVEL_SLOW | R | u16 long-term noise level (~11 s average), for reference. |
| 0x14 | ISR_LOAD | R | % of CPU spent in the mic interrupt over the last second (diagnostic). |
| 0x15 | STALLS | R | Mic clock stalls in the last second (diagnostic; should be 0). |
| 0x16–0x35 | FFT | R | 32 spectrum bins, 0–11.35 kHz in 366 Hz steps. Each is 8 × log2(power), i.e. 0.376 dB per count, 0 = silent. Updated every 50 ms, including during chirps. |

**Level units:** a full-scale sine would read about 41,700, and the level saturates at 32,767. In a typical quiet room it reads about 40–50; talking and music reach roughly 100–400.

### Tuning from a host

1. Watch LEVEL_FAST (0x10) in your quiet environment and with normal activity around.
2. Write a THRESHOLD (0x02) between the two; roughly 1.5–2× the quiet reading works well.
3. Adjust the timings and VOLUME (0x04–0x0A) if wanted.
4. Write 0xA5 to COMMIT (0x0B) to keep everything across power cycles.

For example, to set everything and save in one go: `02 5A 00 D0 07 E8 03 88 13 A0 A5` (threshold 90, pre-roll 2000, rest 1000–5000, volume 160, commit).

Reflashing the firmware erases the saved settings, so the defaults from `config.h` come back.

## Building and flashing

Needs Rust/cargo (for the build system), the Arm GNU toolchain, and [probe-rs](https://probe.rs) with a CMSIS-DAP probe.

```sh
cargo run      # compile; output in target/build/mspm0.elf
./flash.sh     # compile and flash with probe-rs
```

New `.c` files go in `src/mod.rs`; run `cargo build` once after editing it so the build system picks them up.

## Debugging

- **Debugger resets:** probe-rs 0.32 resets this part with a CPU-only reset, which can leave it faulting before `main`. `flash.sh` boots fine. To debug, attach to the running firmware (the "probe-rs - Attach" config) rather than flash-and-halt. A power cycle always recovers it. Fixed upstream in probe-rs (PR #4381), not yet released.
- **Polling RAM:** each probe-rs memory read briefly stalls the core and the mic clock, and the restart shows up in the level. When reading levels over SWD, poll at most about once a second.
- **SVD:** `svd/MSPM0C110X.svd` gives the peripheral register view in the debugger.

More detail on design decisions and hardware measurements is in `CLAUDE.md`. Filter coefficients come from `../matlab/design_lowpass.m`.
