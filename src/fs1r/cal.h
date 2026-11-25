// fs1r/cal.h - the calibration surface: every constant tuned so the model matches a real FS1R.
//
// Tuned. Neither custom chip has public register documentation, so what they do with a register value is modelled. Most of these start as models from the DX7 lineage (same design family), the formant synthesis patent or the Data List, and are replaced by a measurement of a real FS1R.
//
// They all live here so calibrating is one table edit rather than a hunt through the engine.
//
// A value here is a claim about the hardware, so it is changed by measuring, never by taste.
//
// The FS1R's fixed numbers are in fs1r/hardware.h. Knobs that are ours rather than the hardware's, and which must never change the output, are in engine/tuning.h.
#pragma once
#include <cmath>
#include "fs1r/hardware.h"

namespace cal {
static const double WORD         = 8.0;     // a full-scale serial word, in the DSP's register units (dsp/word.h)
static const double FM_INDEX     = 3.369;   // cycles of phase deviation at full modulator level. MEASURED on a real FS1R, 3.364 to 3.375 cycles (21.14 to 21.21 radians) at register 0.
// The operator phase generator, MEASURED on a real FS1R. An operator's phase increment is an integer in units of 2^-OP_PHASE_BITS of a cycle a sample, and a frequency word w (1024 to the octave) becomes M[w & 1023] shifted left by (w >> 10) - OP_OCTAVE0, or right with the bits below dropped, where M[i] = floor(OP_MANT_ONE * 2^(i / 1024)) (tonegen/operator.cpp).
// It gives the FS1R's increments to 0.001 of a step, fixed and ratio-mode, every floor-or-round choice a floor, and a saturated word's 23982 Hz line, 4093 << 10 steps.
// Yamaha's US5744741 gives the same shape, a pitch frequency number in log form, a log/linear converter and a shifter, accumulated into the phase.
static const int    OP_PHASE_BITS = 23;
static const int    OP_MANT_ONE   = 2048;  // M[0]: the table runs 2048..4093, twelve bits
static const int    OP_OCTAVE0    = 21;    // the octave whose increment is M itself
static const double LEVEL_DB     = 0.376287;// dB per step of the 8-bit level registers (LEVTAB doubled). MEASURED on a real FS1R at 0.3761 over 36 dB and 0.3767 over the top 21 dB, both 20*log10(2)/16, a halving every sixteen steps, which is what a binary attenuator does.
static const double EG_LEVEL_DB  = 1.5;     // dB per step of the 6-bit EG level registers (LEVTAB >> 1)
// An operator is cut, exactly zero, once its attenuation (level register plus EG) reaches this many dB, the voiced and the noise half each at their own.
// MEASURED on a real FS1R. A voiced sine sounds at level register 236 (88.8 dB) and is digital zero at 237 (89.2), and the same cut holds with the attenuation in the EG instead (register 15 plus EG step 55 sounds, step 57 is zero); the noise sounds at 251 (94.4) and is zero at 252 (94.8), and at register 15 plus EG step 55, but not 61.
// The part level is not in it, since at part volume 64 a sine at register 199 still plays.
// The CPU writes 254 or 255 for output level 0 with no case of its own, so this is the chip's.
static const double OP_CUT_DB    = 236.5 * LEVEL_DB;
static const double NOISE_CUT_DB = 251.5 * LEVEL_DB;
// dB per step of the carrier level correction (voice 0x2D-0x34). MEASURED on a real FS1R, where corrections 4 and 8 take a carrier 6.03 and 12.05 dB down, 1.506 a step.
// On a modulator after an edit the same steps lower its index as an output level would.
static const double CARRIER_DB   = 1.5;
static const double FEEDBACK     = 0.53;    // feedback gain = FEEDBACK * 2^(fb - 7). MEASURED on a real FS1R over feedback 1 to 6, where 0.5 left the second harmonic a steady 0.5 dB under the FS1R's.
static const double EG_ATTACK_K  = 0.0625;  // rising EG time constant as a fraction of rate_secs. MEASURED on a real FS1R as an exponential in dB, tau/rate_secs = 0.0615 over rates 36 to 44. 1/16 is also what the DX7 EGS does, its attack increment being (17 - level/2^24) times the decay's, so the constant and the lineage agree.
static const double EG_OVERSHOOT = 3.9;     // dB past the TOP OF THE SCALE that a rising EG aims at, not past its own target, since an attack to a target 21 dB down climbs at the same speed a climb to full does, which only an approach aimed at the top and clamped at the target can do. The DX7's EGS is the same shape, its rising increment being (17 - level/2^24) where 16 is full scale. MEASURED 3.94 on a real FS1R with EG_ATTACK_K at 1/16, 0.85 dB rms over ten segments that reach three different targets from three levels.
static const double EG_ATTACK_FLOOR = -54.5;// dB the EG jumps to when a segment starts rising from below it. MEASURED on a real FS1R, a rise from 58.5 dB down is at 52.7 dB down one millisecond in, where one from 36 dB down starts at 36 dB and stays there, so the floor is a jump the chip takes from anywhere below it and not merely where a rise from silence begins. The DX7 does the same with a jump target of 1716 of 4096, which is 55.8 dB below full.
static const double EG_HOLD_FRAC = 0.5;     // the hold segment as a fraction of a full traverse at the hold rate.
static const double EG_HOLD_LAG  = 0.0081;  // ... plus this, fixed. MEASURED on a real FS1R over seven hold settings from 19 ms to 3.65 s, where a free fit lands on 0.4998 of a traverse, so the fraction is exactly a half, and the lag is what the seven leave over, to about six milliseconds either way. Any pure fraction of rate_secs misses the fastest holds by 80 %. Hold register 0x3F means no hold at all, which is why the FS1R leaves that one value alone.
static const double FEG_SEMIS    = 48.0;    // frequency EG range at the full register swing of 128 (four octaves).
                                            // The sysex byte reaches the register through FEGLVL, which is measured
// The frequency EG holds its start level this many samples before it moves. MEASURED on a real FS1R, where over 115 settings the phase a ramp adds fits 0.120 cycle rms with 0 and 0.059 with 3.
static const int    FEG_LATCH    = 3;
static const double FEG_TRAVERSE = 24.0;    // semitones the operator frequency EG covers in one traverse at rate_secs(word), the amplitude EG's own rate ladder. MEASURED on a real FS1R, two attack segments at time 40 (rate word 37, a 546 ms traverse) ramp at 4393 and 4391 cents per second, which is 23.99 and 23.98 semitones per traverse. So the chip steps one EG engine for both and only the unit of measure differs, 0.375 dB per step on the amplitude side and a quarter tone here, a 96-unit scale quartered either way. The ramp is linear and its slope does not depend on the swing, which is what separates this from the filter EG's exponential approach (FEG_SEG_S, a different chip).
static const double FEG_SEG_S    = 1.33e-4; // MEASURED on a real FS1R. A filter EG segment whose aim lies half its swing past its target, which is how the CPU sends them, takes FEG_SEG_S * 2^(word / FEG_RATE_OCT) for the rate word the CPU hands VOP3-1 (register 0x2B). Eight attack times, words 119 to 224, 52 ms to 3.0 s, fit to 1.2% rms; the same word at swings of 128 and 512 took the same 0.23 s, so the approach is exponential and a segment is ln 3 of its time constants.
static const double FEG_RATE_OCT = 15.5;    // rate words to a doubling of that time
static const double FEG_FLAT_S   = 0.78;    // MEASURED on a real FS1R, a filter EG segment with no swing holds 0.78 s (0.71 to 0.86 over eleven notes) at every rate word.
static const double WIN_SKIRT    = 2.0;     // the grain window is sin^p, p = WIN_SKIRT * step^skirt. The skirt is the one shape control all1, all2, odd1 and odd2 have, voice byte 6 being the formant's bandwidth and res1/res2's resonance, which those four do not read. MEASURED: skirt 0 is sin^2 on every form, which puts the two partials either side of a one-period grain 6.02 dB down, exactly as the FS1R does on res1, res2, odd1 and odd2
static const double WIN_SKIRT_STEP = 2.0;   // what the skirt multiplies p by, per step, on every form. MEASURED on odd2 and res2, whose grain is one period long under a carrier at an integer multiple of the grain rate, so their line amplitudes are the window's own Fourier coefficients; p = 2, 4, 8, 16, 32, 64, 128 reproduces them to 0.01 dB at skirt 1 and 0.7 dB at skirt 6. The "1" forms and the formant take the same p on the window's rising half only, with the fall held at sin^2 (tonegen/operator.cpp, init_tables), MEASURED with all six harmonic forms inside 0.5 to 1.8 dB per skirt and the formant's sixteen skirt settings inside 0.4 to 2.4.
static const double FORM_LEVEL   = 0.520;   // what a grain train is worth on every form but the formant, against a grain sum normalised by its own window length. MEASURED: it puts all1, all2, odd1, odd2 and both resonant forms within 0.14 dB of the FS1R at once. The 3.5 dB between the all forms and the odd ones is not a constant at all, since it falls out of the odd forms retriggering twice a period under a window half as long.
static const double FRMT_BW_HZ0  = 3.1;     // the formant window's bandwidth in Hz at byte 0 ...
static const double FRMT_BW_DB   = 8.0;     // ... doubling every FRMT_BW_DB steps, so the window lasts 1 / (FRMT_BW_HZ0 * 2^(bw / FRMT_BW_DB)) seconds. MEASURED on a real FS1R, the window is a fixed TIME, not a fixed number of periods of the fundamental, since the bandwidth at note 36 and at note 60 gives groups of the same width in Hz, 573 against 559 at byte 56 and 4278 against 4172 at byte 88, while their widths in partials differ by the four the two fundamentals differ by. Held at a fixed fraction of the period instead, the same law puts its knee at byte 44 at note 60 and at byte 27 at note 36.
static const double FRMT_WL_MAX  = 2.0;     // and the window never runs longer than this many periods, which is two grains overlapping and is what the engine has slots for. Below the bandwidth where it binds the group is narrower than one partial at either note measured, so no measurement can see it and the clamp is estimated, not confirmed.
static const double FRMT_NORM    = 0.0;     // how a formant's level follows its window length: 0 leaves the
                                            // window's peak at 1, so a wide bandwidth is quiet; 1 holds the
                                            // spectral peak instead, which is what a FOF generator does
// The unvoiced generator is a clamped random walk, the "correlative noise" of Yamaha's US5610354 (FIG. 14), spelled out in US5744741 and JPH04346502A; Yamaha's S-YXG100 soft synth computes the same walk in floating point.
// White noise through a one-pole set by the skirt is the drive. Each sample the walk moves to a point between itself less the width and itself plus the width, clamped to [-1, 1], placed there by the drive two samples back plus the resonance's drift, and the walk times the centre's sine is the operator.
// Two one-poles on white noise give the spectrum and not the waveform, since the FS1R's noise is sine valued below bandwidth 44, bursty at 52 to 56 and Gaussian above.
// FITTED on one FS1R by level, band shape and kurtosis, at a 1 kHz and an 8 kHz centre; not yet confirmed on a second.
// The walk's width is the bandwidth register in 64ths of a rail, WALK_STEP a register. Register 0 freezes the walk, which leaves the carrier alone at the level the walk stood at, 64 is one rail, and from about 90 the walk is the drive itself.
// The widths fitted setting by setting are the register over 64 to 1 to 3 % from register 30 to 73, and the law reads level and band shape as well as an interpolated table of them or better, past the rails too.
// It reads as the 7-bit register with six fraction bits, a shift in fixed point; the bit widths are not measured.
// WALK_COEF is the drive's one-pole by skirt, 1/16 at 0.
// WALK_DRIFT is the S-YXG100's table by resonance; fitted free, resonances 4 to 6 land on it and 7 reads 0.35 against 0.25.
// WALK_GAIN is one gain for every setting, the FS1R's level past the rails.
static const double WALK_STEP     = 1.0 / 64;
static const double WALK_COEF[8]  = {0.0625, 0.0935993, 0.122913, 0.187135, 0.246659, 0.368925, 0.486767, 0.710525};
static const double WALK_DRIFT[8] = {0, 0.00390625, 0.0078125, 0.015625, 0.03125, 0.0625, 0.125, 0.25};
static const double WALK_GAIN     = 1.0457;
// The unvoiced output has no two-sample mean. MEASURED on a real FS1R, its left channel alone is the walk's band to 20 kHz at 8, 12 and 16 kHz centres, where the mean took 1.1, 2.7 and 5.3 dB off.
// The cos^2 the L+R sum shows comes from the right channel running one sample behind the left, which every channel carries, voiced and unvoiced alike.
static const double PMS_FRAC[8]  = {0, 0.038, 0.076, 0.125, 0.212, 0.365, 0.620, 1.0};        // per-op pitch mod sensitivity. MEASURED on a real FS1R at pitch depth 99, the fundamental's swing at pms 1..6 is 0.038, 0.076, 0.125, 0.212, 0.365, 0.620 of pms 7's, where the DX7 curve gives 0.026, 0.053, 0.089, 0.161, 0.277, 0.497, a third shy at every step.
// Per-op freq mod sensitivity, in 256ths of FMS 7's swing. MEASURED on a real FS1R, a fixed 1 kHz sine under a square LFO1 swings 92.9, 184.9, 305.4, 518.8, 889.6, 1482.1 and 2373.2 cents at FMS 1..7, FMD 99, each within 0.3 % of these 256ths.
// FMS 7 is the engine's full LFO word to 0.1 cents at FMD 99, 66 and 33, and FMS 5 keeps its ratio at FMD 66. Unmeasured on the noise half, which takes the same table.
// A linear fms / 7 is twice the FS1R at FMS 5.
static const double FMS_FRAC[8]  = {0, 10 / 256.0, 20 / 256.0, 33 / 256.0, 56 / 256.0, 96 / 256.0, 160 / 256.0, 1.0};
// The output path, MEASURED off a real FS1R's digital output.
static const double OUT_GAIN     = 0.14992; // fixed gain between the summed bus and the digital output. Nine single-operator measurements agree to 0.005 dB.
static const double FX_DRY_GAIN  = 5.646458;// the effect chip model's dry path at performance volume 127 with a
                                            // full-scale word in, 15.04 dB. 
static const double LEVEL_SLEW_MS= 1.6;     // the voiced level register does not step, it glides, one pole with this time constant. MEASURED on a real FS1R under an Fseq that rewrites the level thirty to eighty dB at a time, where the octave band error bottoms at 4.74 dB here, against 9.7 for a register that steps and 10.6 for one latched to the grain. Flat between 1.2 and 2.0, so the last digit is not measured. The unvoiced level glides on it too, MEASURED at 1.7-2.0 ms 10-90 % on the FS1R at three notes against 1.5-1.9 here (FSVR#103).
static const double DAMP_MS      = 1.0;     // the note-on damp: what is left of the previous note in a channel the allocator reuses decays with this time constant instead of being cut to zero. Estimated, not yet confirmed, and the one constant here with no measurement behind it. The FS1R does damp, setting the taken channel's EG stage word to 4, the release stage, and writing a one-hot channel mask to register 0xFC/FD, but nothing says how fast the chip fades. 1.0 ms is the order of the level register's own glide and it removes the step.
static const double CHAN_CLIP    = 1.1919;  // the channel accumulator saturates here, hard and memoryless, before the filter loop. Driven 4x and 8x past one carrier, the FS1R gives the same ceiling to five places.
// The VOP3-1 filter loop costs nothing. MEASURED on a real FS1R, the part filter on against off on one voice, LPF24 at resonance 0, reads 0.00 dB at part volumes 127 to 40, cutoffs 127 to 80, and on an all1 source at note 24; input gains 0, 6, 18 and 24 land on -12.06, -6.03, +6.03 and +12.05 dB.
static const double FLT_LOSS     = 1.0;     // the gain through the VOP3-1 filter loop, which loses nothing
}
using cal::FM_INDEX;
using cal::LEVEL_DB;
