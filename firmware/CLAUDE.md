# NoisyCricket: project handoff

Exported from the Claude project "NoisyCricket" on 2026-10-14 for use in Claude Code. Sources: project memory and the full firmware design thread (2026-10-07). The base firmware project lives with Brent, not in this export.

## Working preferences (follow these)
- Brent wants brief, non-conversational responses. Act as a tool, not a person.
- Planning first. Generate no code, files or docs unless Brent explicitly asks. Answer in prose and numbers.
- Code snippets Brent requests are usually generic, not MCU-specific.
- Explicit file/code requests override the no-generation rule for that request only.
- Brent does analysis on the PC in MATLAB (Octave-compatible, no toolboxes preferred).

## Goal
Hobby add-on module for a conference badge: small board with a cricket motif. It monitors ambient noise and, when it gets quiet, plays random cricket chirps through a speaker.

## Hardware (fixed; board is already fabricated, no wiring changes)
- MCU: TI MSPM0C1104. Cortex-M0+, 24 MHz system clock, 16 KB flash, 1 KB SRAM, no FPU, 32-cycle multiply. Source: TI datasheet SLASF90D (https://www.ti.com/document-viewer/lit/html/SLASF90D/GUID-ACFD2B2B-7C42-42FF-82B9-7CBA94AFB2DC). Some details were read from a summary; verify against the TRM.
- DMA supports only the ADC (confirmed by Brent). All mic RX and PWM updates are ISR-driven.
- Mic: PDM MEMS mic, read through the SPI peripheral (no PDM peripheral). SPI has 4-deep RX/TX FIFOs, up to 16-bit frames, max 12 MHz.
- SPI SCK pad is shared with TIMA0_C1, so the SPI must be the controller and drive SCK itself. It cannot clock from a timer output.
- Audio out: TIMA0_C0 PWM, 8-bit resolution, 93.75 kHz (24 MHz / 256), then a filter, class-AB amp, tiny speaker.
- Other timers: TIMG14, TIMG8. TIMA0 is 16-bit with 8 channels.
- I2C is available for readout. No UART is wired out; use the debugger to read memory.

## Behavior
- Monitor ambient noise; chirp at random when quiet (hysteresis on the quiet decision).
- Chirp: 3.5 to 4.5 kHz fundamental sweep (assumed up; confirm against Brent's recording) plus three overtones (offset and relative amplitude controllable), half-sine envelope, about 30 ms long. The recording is 1479 samples at 46.9 kHz. The current generated LUT is 703 samples at 23.4375 kHz. Brent's original estimate of a 35 Hz envelope (14 ms) was wrong.
- Terminology: a **chirp** is 3 playbacks of the sample with ~20 ms gaps (about 130 ms total; supersedes the earlier 2-3 chirps / 5-10 ms gaps idea). A **chirp group** is 1 to 3 chirps, with 500-1000 ms between chirps (randomized within that range). The first chirp of a group starts a fixed, known delay after the quiet threshold trips, as a sync anchor across devices; only the chirp count and inter-chirp gaps within the group are randomized.
- The mic is swamped while chirping and must be blanked for the whole chirp group (see Mic blanking) - including the 500-1000 ms inter-chirp pauses, since bringing the mic back up for a partial second between chirps isn't worth it.

## Decisions and recommendations

### Scope (Brent, 2026-10-07)
- Chirp recognition and unit-to-unit synchronization are deferred. Neighbor chirps are ignored by excluding about 3.25 to 4.75 kHz from the noise calculation (see Ambient level). Revisit only if CPU budget forces chirp detection.
- Keep hooks so adding them later needs no rework: a shared post-CIC sample tap with multiple consumers, chirp scheduling as a phase timer with randomized period, group count and gap (sync later is a phase nudge), and a single blanking flag that everything downstream respects.
- Original sync idea (not implemented): pulse-coupled oscillators, like fireflies. Detection would use a 4 kHz band-pass or Goertzel plus an envelope, and the in-band to broadband energy ratio.

### Clocking and timing
- PDM clock 1.5 MHz (24 MHz / 16). SPI frames are 16 bits, giving 93.75k words/s. One output sample is 4 words (64 bits).
- Audio/output rate 23.4375 kHz (24 MHz / 1024, PWM / 4). 46.875 kHz is possible if 23.4 kHz sounds harsh (constant change, not a restructure).
- SPI and TIMA0 both derive from the 24 MHz clock, so they are coherent with no drift. Confirm neither sits behind an asynchronous divider. Timer sharing is not needed.
- Rule: set the PWM period equal to the measured SPI word period in 24 MHz cycles. That is 256 with no inter-frame gap, or 272 with a 1-bit gap (88.2 kHz PWM, 22.05 kHz audio).
- Start the SPI at a known TIMA0 count so the 2-word RX IRQ lands mid-PWM-period. Use a shadow/buffered compare load so ISR jitter does not move the output edge.
- Coherent clocks also make PWM-carrier leakage into the mic alias to DC, where the DC blocker removes it.

### Mic path
- Per 16-bit word: popcount (two lookups into a 256-byte LUT, about 8 to 10 cycles, or bit-twiddling at about 15 to 20). This is the R=16 integrate-and-dump stage.
- CIC at the word rate: 3 integrators (32-bit wraparound adds), decimate by 4, 3 combs, giving 23.4375 kHz. No multiplies. The CIC cannot be fed from raw bits; each integrator needs a multibit input.
- CIC order is open: many MEMS mics use a 4th-order modulator, so a true CIC would want order 5. A cheap fix is one more integrator/comb stage. Decide with offline MATLAB simulation on real captures.
- Boxcar-32/64 as the first stage was rejected. It saves about 2% CPU and costs alias rejection. Keep boxcar-16 (popcount).
- After the CIC: DC-blocking high-pass, then droop correction if needed.
- Interrupts: RX FIFO threshold at 2 words (about 680 cycles slack), so two mic IRQs per output sample and the audio step runs on every second one. SPI RX gets top priority; keep the I2C handler short.
- SPI is the controller and needs a TX write per RX word. Prime 4 dummy words, then write 2 per RX IRQ. The TX FIFO must never empty, since that stops the mic clock. Check the TRM for an RX-only or auto-repeat mode.
- Estimated CPU: about 18% for mic work at the 2-word threshold (about 170 to 200 of 1024 cycles per output sample).

### Ambient level
- Do not derive the level from the FFT. Use a time-domain band-stop around 3.25 to 4.75 kHz ahead of the level averager, then leaky averages of |x| with shift-only coefficients: fast (1024 samples, ~44 ms) and slow (~11 s, updated every 16 samples from the fast level) as the noise floor. (3.25-4.75 kHz corrected from an earlier 3.6-4.4 kHz / misread-FFT 7.5-9.5 kHz; final as of 2026-10-14.) Coefficients are compile-time constants; making the band runtime-adjustable over I2C would need storing them in RAM/flash instead, not currently planned.
- Implemented as a 4th-order Butterworth band-stop (two cascaded Q14 biquads), not a single biquad: one biquad is only -3.5 dB at 3.4/4.5 kHz, so most of the band wasn't really excluded. Response: -3 dB at 3.25/4.75 kHz, -8.7 dB at 3.5 kHz, -22 dB at 3.75 kHz, -50 dB at 4 kHz, -17 dB at 4.25 kHz, -7 dB at 4.5 kHz, flat passband. Coefficients from `../matlab/design_bandstop.m` (widen the design edges there if the band edges need more attenuation). Host simulation of the real `mic.c` with a sigma-delta PDM stream matched this (4 kHz -31 dB, limited by modulator noise) and shows the expected CIC droop above 5 kHz (-2 dB at 5 kHz, -5 dB at 8 kHz).
- The 8 kHz second harmonic aliases to about 3.7 kHz at an 11.7 kHz rate, inside the excluded band. (At 23.4 kHz it does not alias, so re-check against the final rate.)
- FFT is optional readout only: 32 or 64 points, int16 in place (about 128 to 256 B), log-magnitude bytes over I2C, run in the main loop at low priority. Raw audio over I2C is too fast.
- Consumer (Brent, 2026-10-14): drives a simple spectrogram on the badge's own screen, polled by the host over I2C roughly every 100 ms with any further averaging done in the reading software. Tearing was accepted, but the implementation avoids it at no RAM cost: every 50 ms the main loop arms a one-shot capture of 64 DC-blocked samples straight into the FFT buffer, and the ISR stops writing once it's full, so the in-place FFT never races the ISR.
- Implemented: 64-point Hann-windowed radix-2 FFT, int16 in place (256 B), halved every stage, bins 0-31 (366 Hz each) published as 8*log2(power) bytes (0.376 dB/LSB). Takes the DC-blocked signal before the band-stop, so neighbours' chirps (and the unit's own) show on the spectrogram. Host test: peaks land in the right bins, magnitude within 0.25 dB of theory.
- Note: an earlier plan decimated to about 11.7 kHz for lower ISR load. The later 23.4375 kHz plan superseded it. If the FFT needs fewer points, decimate again for it.

### Chirp output
- Decision (Brent, 2026-10-14): pre-generated table playback only, final. No on-chip synthesis, now or later; the generator can be rerun any time the sweep/overtones need retuning instead.
- At 23.4375 kHz: 703 samples, 703 B flash. CPU is a few cycles per sample, with no ring buffer or main-loop synthesis and deterministic timing.
- Playback volume (Brent, 2026-10-14): runtime, I2C register 0x0C (u8, 0 = mute, 255 = table as-is, gain = v + v/128 out of 256), default `DEFAULT_VOLUME` 255, persisted with the threshold by the same commit. Leaves `chirp_lut.h` untouched; costs one multiply per sample, only while playing. The table peaks at +/-118 of 128, so clipping heard at full volume is analog (amp/speaker headroom), not digital.
- Cheap variation from the table: fractional read increment (8.8 fixed point, 240 to 272 gives about ±6% pitch/duration, optionally with linear interpolation), gain by shift or one multiply, and groups/gaps by scheduling.
- The generator (`gen_chirp_lut.m`) builds the fundamental sweep plus three overtones in plain double-precision float (each overtone is the same sweep shape, shifted by a configurable Hz offset and scaled by a configurable relative amplitude), sums them, and quantizes to 8-bit unsigned only at the final step.

### Mic blanking (updated 2026-10-14)
- Keep clocking the mic and discard the data. Stopping the clock may put the mic to sleep with a startup delay; check its datasheet.
- Ownership is main-loop, not ISR: the main loop sets the blanking flag once before the first chirp of a chirp group, runs every chirp and every inter-chirp gap in that group (500-1000 ms each) with the flag held the whole time, then clears it after the last chirp finishes plus `SETTLE_MS` (100 ms) of ring-down. The ISR owns no timing of its own (no ring-down counter in the ISR).
- Implemented differently from the earlier "drop samples before the CIC, reset CIC/biquad at unblank" plan: the whole chain (CIC, DC blocker, band-stop, FFT capture) keeps running during blanking and only the two level averagers freeze. Nothing is reset, so there's no restart transient at unblank (resetting the DC blocker would have caused an ~11 ms decaying DC step into the level). The settle delay now happens before unblanking, still blanked, so the frozen pre-chirp level resumes cleanly.
- Rationale for blanking the whole group rather than relying on the band-stop notch and letting things settle naturally: the mic may be acoustically saturated by its own chirp (broadband clipping distortion, not just energy in the notched 3.25-4.75 kHz band), and the slow noise-floor average (~10 s time constant) would carry that bias for seconds afterward, skewing the device's own sense of ambient level. Worth confirming empirically once real mic captures exist (bring-up step 2) - if saturation turns out to be mild, blanking could be relaxed later.
- The unit's own chirp phase is known, so sync logic needs no self-hearing.

### SPI/mic ISR (Brent, 2026-10-14)
- Single IRQ, sourced from RX FIFO half-full (2 of 4 words). In that handler: drain 2 RX words, push 2 TX zero-words to replenish (no separate TX-empty IRQ). At 1.5 MHz PDM clock vs. 24 MHz core there's about 512 core cycles between words, expected to be enough margin; confirm at bring-up (scope SCK for gaps) and only add a second IRQ if that margin isn't real.
- The audio-output step moves into this same ISR: the PWM compare write happens on the decimate tick (every second RX IRQ, i.e. every 4 raw PDM words = 23.4375 kHz), replacing `audio.c`'s current free-running TIMA0 repeat-counter IRQ. TIMA0 keeps generating the PWM carrier only; it no longer needs its own interrupt.
- The mic-blanking flag is checked here too: while set, the level averagers freeze; everything else keeps running (see Mic blanking).
- Implemented in `src/mic.c` (`SPI0_IRQHandler`, priority 0): drains whatever RX words are present and writes one TX zero per word read, so words in flight stay at the FIFO depth (4). If the ISR is held off (flash write), the TX FIFO drains and the clock stops, and the RX event still pending at unmask restarts it - no explicit resync needed. SCR = 7 gives 1.5 MHz (24 MHz / ((1 + SCR) * 2)); the old test code's SCR = 48 (~245 kHz) was a deliberate slowdown for earlier tests; the mic originally ran at 2 MHz, so 1.5 MHz is within its known-good range.
- Per-sample cost estimate: ~300 instructions in the ISR path; GCC turned most constant-coefficient multiplies into shift/add, so the 32-cycle multiplier matters less than feared. Still unmeasured - check ISR load at bring-up (e.g. SysTick before/after, or toggle a pin).

### Chirp scheduling (Brent, 2026-10-14)
- Three main-loop states: idle (loud) -> armed (quiet detected, counting the fixed sync-anchor pre-roll) -> chirping (1-3 chirps = one chirp group, see Behavior). Hysteresis lives on the quiet/loud threshold itself (already decided).
- Implemented in `src/cricket.c`; all timing knobs in `src/config.h` (`PREROLL_MS` 2000, gaps 500-1000 ms, `SETTLE_MS` 100, PWM then amp warm-up 5 + 500 ms before the first chirp; the AD8107's turn-on ramp is set by its BYPASS cap, and 2.2 uF parts were fitted instead of 1 uF - with only 20 ms the first chirp of each group came out quiet). Quiet = fast level < threshold; loud again above threshold + threshold/4. Amp and PWM stay on through the whole group and switch off after the last chirp.
- Rest period (Brent, 2026-10-14): if still quiet after a group, the unit waits a random `REST_MIN_MS`..`REST_MAX_MS` (5-20 s, compile-time constants only, not I2C) and then starts the next group directly, with no pre-roll. Noise during the rest returns it to idle, so the next quiet edge goes through the fixed pre-roll sync anchor again. Status reports REST as armed.
- All per-group randomization (chirp count, inter-chirp gaps) is drawn once at group start, then played out open-loop against the existing 1 ms timebase (`timebase.c`).
- No mid-group noise recovery check: once a chirp group starts, it always runs to completion (1 to 3 chirps) regardless of ambient noise in between - consistent with blanking covering the whole group.
- PRNG seed source: raw SPI mic words (pre-filter), not the CIC/filtered output - more entropy than the already-decimated signal, and no hardware RNG has been confirmed on this part (check the TRM if that matters later).

### I2C register map (Brent, 2026-10-14)
- Target mode, flat register-file style: a register-address byte selects the register, then a burst read or write.
- Registers: noise threshold (1-2 B, R/W, freely adjustable, no flash activity on its own); a commit register (W, magic key) that triggers the flash write below; FFT bins (32 B, RO, log-magnitude, feeds the host's spectrogram, tearing allowed - see Ambient level). Optional: LED RGB (3 B, W), beep request - frequency + duration (W).
- Implemented (`src/i2c_target.h`), 7-bit address 0x42 (`config.h`; picked to avoid 0x3C/0x3D displays and the 0x50 SAO EEPROM): 0x00 ID (0xC7), 0x01 status (bit0 quiet, bit1 armed, bit2 chirping), 0x02-03 threshold u16 LE, 0x04-05 fast level, 0x06-07 slow level, 0x08 commit (write 0xA5), 0x09-0B LED RGB, 0x0C volume, 0x10-0x2F FFT bins. Always write the register address first. Added beyond the plan: ID, status and the two level registers, so the host can see the live level while tuning the threshold. Beep request not implemented yet.
- TX side loads exactly one byte per TX request (TX-empty-on-request, host clock-stretched), so nothing is prefetched and the pointer always matches what the host read. Threshold/LED writes apply at the transaction's STOP.
- SRAM, measured at first build: 488 B static (FFT re/im 256 B, mic state 76 B, I2C regs 48 B, flash RAM routine 56 B, scheduler/misc ~50 B), leaving ~536 B for the stack; rough worst case with all three ISRs nested on the main loop is ~350 B.
- Build changes needed to get there: the baseline image already used 648 B of SRAM on unused library state. `makefile.rs` now links with `-nostartfiles` (newlib's crt0 was kept by the linker script's `KEEP(*(.text))` and dragged in exit() and stdio tables, ~300 B; TI's startup calls main() directly), drops the build system's nosys stub workaround (it pulled in newlib's 80 B reentrancy struct through errno), and `mspm0/mod.rs` no longer builds `dl_interrupt.c` (its 192 B RAM vector table is unconditionally kept). Baseline went from 648 B to 8 B.
- `SCL` shares a pin with `NRST` (fixed hardware, see `pins.h`). With NRST active, SCL low pulses would reset the MCU, so `main()` calls `DL_SYSCTL_disableNRSTPin()` first thing (locked until the next POR). Consequences: no pin reset while running (power-cycle or debugger SYSRESETREQ instead), and between POR and that call (boot ROM + startup, a few ms) bus traffic can still reset the part, which could cause repeated resets if the host polls constantly. Worth watching at bring-up; check whether BCR/NONMAIN can disable NRST from boot if it's a problem.

### Flash persistence (Brent, 2026-10-14)
- One reserved flash page holds the noise threshold. The threshold register itself can be written freely over I2C with no flash activity - only writing the magic key to the commit register triggers an actual flash write, so tuning the threshold to a good value doesn't wear the flash or disturb anything.
- Brent is fine with the flash write disrupting the rest of the system while it happens, so no scheduling/gating around it: mask interrupts for the duration of the `*FromRAM` erase/program call (required - see below), do the write, unmask. The mic restarts by itself (see SPI/mic ISR). Implemented in `src/settings.c`: a whole 1 KB-aligned `const` sector holds magic, threshold, volume and a check word (two 64-bit flash words), saved together by one commit; reflashing the firmware wipes it back to defaults.
- Why interrupts must be masked: checked `dl_flashctl.c` - the erase/program commands run from a RAM-resident function (`.ramfunc`) because the flash array is expected to be unreadable for instruction fetch while a command is in progress (typical for small single-bank parts; worth confirming the exact erase/program timing in the TRM, though any plausible value is far longer than the ~21 us SPI TX FIFO budget regardless). That RAM routine does not mask interrupts itself - if it fires during the busy window, the ISR vector/code fetch from flash would hang, not just run late. The mic clock will stop for the duration either way, by design, per Brent's call above.

### LED fade (Brent, 2026-10-14)
- Green LED fades in/out over the chirp group duration (fixed, about 130 ms) via a software envelope stepped from the existing 1 ms timebase tick, feeding the existing gamma-corrected `led_set_green()`. No sample-accurate timing needed.

### Power-on self-test (Brent, 2026-10-14)
- `src/selftest.c`, blocking, run from `main()` after init (~5.5 s; the mic ISR and I2C keep running, and host writes apply after it): one chirp through the normal path with the green fade, LED white 1 s, then 4 s of LED colour following the fast mic level on a log scale (blue = 1 CIC LSB, then cyan, green, yellow, red = full scale). Durations are constants at the top of the file.

### Beep tone (optional, Brent, 2026-10-14)
- If built: its own small runtime phase-accumulator tone generator (quarter-wave LUT), separate from the table-only chirp path. Arbitrary frequency/duration over I2C needs this; it does not reopen the chirp's no-on-chip-synthesis decision, since it's a distinct, optional feature.
- Deferred (Brent, 2026-10-14) until the chirp path works on hardware. When built: writing the duration register triggers the beep (no separate trigger register), and a beep overrides the normal chirp audio path (preempts a playing chirp rather than queueing or being refused). Proposed timbre: soft-clipped sine with an editable gain constant, 5 ms attack/release; blanks the mic like a chirp group.

## Existing files
Firmware (`src/`, each `.c` listed in `src/mod.rs`): `main.c` init, self-test call, main loop glue (I2C writes, FFT scheduling, status publishing); `mic.c` SPI/PDM + DSP ISR; `audio.c` PWM carrier, amp, chirp sequencer stepped from the mic ISR; `chirp_lut.h` the playback table; `cricket.c` scheduler/LED fade/PRNG; `selftest.c` power-on test; `fft.c`; `i2c_target.c`; `settings.c`; `led.c`; `timebase.c` (1 ms tick, TIMG8, priority 1); `config.h` tunables.

In `../matlab/`:
- `design_bandstop.m`: prints the band-stop Q14 coefficients used in `mic.c` and the quantised response.
- `gen_chirp_lut.m`: MATLAB/Octave generator, no toolboxes, plain floating point throughout (quantized to 8-bit only at the final step; no MCU-synthesis mirroring). Set `fs_option` (1 = 46875 Hz, 2 = 23437.5 Hz), `f0`, `f1`, `T_ms`, `amp`, `overtone_offset_hz`, `overtone_amp` at the top. The last two are 3-element vectors: Hz offset and relative amplitude (vs. the fundamental) for each overtone, each sweeping the same shape as the fundamental. Note: `out_name` is hardcoded to `chirp_lut.h` regardless of `fs_option`, so switching sample rate overwrites the existing file rather than producing a second one — rename the output first if both are wanted.
- `chirp_lut.h`: currently 23.4375 kHz, 703 samples, 3500 to 4500 Hz fundamental up-sweep plus 3 overtones (150/320/500 Hz offsets, 0.5/0.35/0.2 relative amplitude), half-sine envelope, 8-bit unsigned centred on 128, peak 127.
- `chirp_lut_wave.png`: waveform and quantization-error plot.
- `chirp_lut_spec.png`: spectrogram and spectrum.
- `chirp_lut_qw.png`: stale — from the retired quarter-wave fixed-point generator, no longer produced by the current script. Safe to delete.
- The generator makes a single chirp only; groups and gaps are the scheduler's job.
- Note: the firmware plays Brent's 650-sample table (23.4375 kHz, 27.7 ms), now in `src/chirp_lut.h` in the same layout the generator writes, so a regenerated `chirp_lut.h` can be copied over it directly. It is not the generator's current 703-sample overtone version.

## Bring-up checklist (firmware implemented 2026-10-14, untested on hardware)
Everything below compiles and the DSP/FFT path passed a host simulation; none of it has run on the board yet.
1. Boot with I2C idle: check the board boots repeatedly (NRST is disabled in `main()`; see I2C register map) and that SWD still connects.
2. SPI/mic: scope SCK - expect 1.5 MHz, continuous 16-bit frames. Measure any inter-frame gap: the PWM period (255) assumes 256 cycles per word, and audio pitch scales with the real word rate. Read `REG_LEVEL_FAST` (or `s.fast` via the debugger) in quiet vs. noise.
3. ISR load: measure SPI ISR time per output sample against the 1024-cycle budget.
4. Audio: the power-on self-test chirps immediately (otherwise wait for the 2 s pre-roll in a quiet room) - listen for pops at amp on/off and check pitch/duration. 23.4 vs 46.9 kHz comparison is now moot unless 23.4 sounds bad.
5. Mic capture during a chirp (debugger dump of the FFT buffer or levels with blanking temporarily off) to see whether the mic saturates - decides whether whole-group blanking can be relaxed.
6. Threshold tuning over I2C: read the fast level, write threshold, then commit (0xA5 to 0x08) and power-cycle to confirm it persists.
7. FFT: read 0x10-0x2F while whistling; peak should track pitch at 366 Hz/bin.
8. Offline (any time): emulate CIC order 3 vs 4 on real captures (popcount-nibble dump plan: pack per-word popcount-8 as nibbles, 256 B = 5.5 ms).

## Mic noise floor and CPU (measured on hardware 2026-10-14)
- Problem: at 1.5 MHz the threshold window between quiet room and loud music was tiny. Quiet-room FFT showed the floor rising with frequency (74% of level power above 4.8 kHz): the mic's shaped modulator noise, because 1.5 MHz is near the bottom of standard mode (1.3-4.8 MHz, datasheet test clock 2.4 MHz). Capture edge (PHA0 vs PHA1, L/R = GND) made no difference.
- Fix: PDM clock 3 MHz (SCR 3), 8 words per sample (CIC N=3, R=8, still 23.4375 kHz), and the level path is now a 4th-order Butterworth low-pass at 2.5 kHz (`matlab/design_lowpass.m`) instead of the band-stop; it also rejects the chirp band (-10.5 dB at 3.25 kHz, -19 dB at 4 kHz). Quiet floor dropped ~11-13 dB across the band.
- Level units changed: full scale +/-65536 (8x the old Q4 units); level path clips at 32767, FFT input clips at -12 dBFS (FFT now +18 dB vs before). Thresholds tuned under the old build need re-tuning.
- Low-power mode tried (750 kHz, SCR 15, 2 words/sample): 43% load but floor ~10-15 dB worse than 3 MHz. Rejected.
- CPU: `muls` is 32 cycles on this part (benchmarked). ISR load at 3 MHz ~78%: ~340 cycles/sample in the per-sample chain (mostly the low-pass) + ~50 cycles/word. No real clock stalls; each probe-rs memory read causes ~2 (debugger halts), so ignore stall counts taken while polling. ISR load and stalls are I2C registers 0x0D/0x0E (`MIC_PROFILE`). SysTick reads as all zeros on this part; load is timed with TIMG8's counter.
- Pairing words (32-bit boxcar, CIC R=4) would save ~10% CPU but loses the 3rd-order null at 93.75 kHz; not done.
- Measuring levels over SWD: each probe-rs read halts the core long enough to stall the PDM clock, and the mic's restart transient lands in the level (decays in ~0.2 s). Poll at most ~1 read/s; faster polling gives a false, flat high level (13 reads/s read ~2200 regardless of sound). Earlier quiet-room figures of 100-400 were inflated this way.
- Clean levels at 1 read/s (Brent's room, 2026-10-14): quiet 41-46, talking ~140-360 (60-95 in pauses), music 80-220, keystrokes 85-150 decaying. Threshold 75 (loud above 93) works: chirps in a quiet room, ignores keystrokes, silenced by talking/music. Also confirmed it ignores a recording of its own chirp played nearby (low-pass rejects the chirp band), so neighbouring units shouldn't silence each other.
- Low-frequency room noise (366 Hz FFT bin) is visible in the spectrum. Raising `DC_SHIFT` 8 -> 4 (~230 Hz high-pass) would exclude it at no cost; deferred (Brent, revisit later).

## Debugging notes (2026-10-14)
- probe-rs 0.32 resets MSPM0 with a CPU-only reset (CPURST, RSTCAUSE 0x1D). On some reset paths (GDB `monitor reset` / `reset halt`, and likely the VS Code "Flash & Debug" config with haltAfterReset) the core then takes SP/PC from SRAM 0x20000000 instead of the flash vector table and HardFaults before main. Fixed upstream by probe-rs PR #4381 (SYSRST via SPREC), merged 2026-10-14, not yet released.
- `flash.sh` (`probe-rs download --reset`) boots correctly. Use it to flash, then the "probe-rs - Attach" launch config to debug. A power cycle always recovers.
- The installed OpenOCD (/opt/homebrew/bin) is ST's fork with no MSPM0 support; upstream OpenOCD after 0.12.0 has it.

## Open questions
- Does the SPI offer an RX-only mode, or are dummy TX writes required (expected)? Verify in the TRM.
- Measured SCK inter-frame gap, and whether the mic standby threshold is affected.
- Mic datasheet: modulator order (CIC order), sleep/startup behavior when the clock stops, output noise spectrum.
- Is PWM compare shadow-loaded at the period boundary?
- Does the real recording sweep up or down? Generator assumes up.
- Is 23.4375 kHz output acceptable on the real speaker?
- Any asynchronous clock divider between the 24 MHz clock and SPI/TIMA0?
- DMA limitation and the 1-DMA-channel-for-ADC finding came from datasheet summary and Brent's confirmation.
- Is 0x42 free on the badge's I2C bus?
- Default threshold is 75 (tuned on hardware, see Mic noise floor). Level range 0..32767; writes are not bounds-checked, any u16 is safe.
- Beep tone: deferred (see Beep tone).
