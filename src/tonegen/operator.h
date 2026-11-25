// tonegen/operator.h - one operator: its waveforms, grains and level. Tuned: the constants are in fs1r/cal.h.
#pragma once
#include <algorithm>
#include <cmath>
#include "fs1r/hardware.h"
#include "fs1r/cal.h"
#include "math/fastmath.h"
#include "presets/voice.h"
#include "tonegen/envelope.h"

// The chip's LFO amplitude modulation: the channel AM word is 7 bits, so it floors at 127 steps (48 dB) where the CPU can send 255, and the per-operator sensitivity weights it {0,1,2,4,5,6,7,8}/8, not k/7.
static const int AMS_W[8] = {0, 1, 2, 4, 5, 6, 7, 8};
static inline double am_att(int regAM, int ams) { return std::min(regAM, 127) * LEVEL_DB * AMS_W[ams & 7] / 8.0; }
// One pole per sample coefficient for the voiced level register, cal::LEVEL_SLEW_MS as a rate.
const double LEVEL_SLEW_K = 1.0 - exp(-1.0 / (cal::LEVEL_SLEW_MS * 0.001 * SR));

// ------------------------------------------------------------------------------------------ tables (chip side)
extern float g_win[2][8][1025];
extern float g_winDC[2][8];
extern uint16_t g_opMant[1024];   // the phase generator's mantissas, floor(2048 * 2^(i / 1024)) (cal.h)
extern double g_slewN[65];        // the level slew over n samples at once, for the Fast steps' blocks (render_chan)
void init_tables();

// An operator's phase increment for a frequency word, in units of 2^-cal::OP_PHASE_BITS of a cycle a sample (cal.h). The word is a 16 bit register that saturates rather than wrapping.
// The formant's grains, the channel pitch and the noise centre take the same law, as a fixed formant holds still against a fixed sine on the same word.
// A word with a fraction, which the FS1R's never has (FSVR's 14-bit bend, the frequency EG's ramp), takes the same law at its fractional place.
static inline uint32_t op_increment(double w) {
    w = w < 0.0 ? 0.0 : w > 32767.0 ? 32767.0 : w;   // no std::clamp: the lane kernels call this (kernel/README.md)
    const int wi = (int)w, i = wi & 1023; const double fr = w - wi;
    const uint32_t M = fr == 0.0 ? g_opMant[i] : (uint32_t)(cal::OP_MANT_ONE * fm::exp2((i + fr) / 1024.0));
    const int s = (wi >> 10) - cal::OP_OCTAVE0;
    return s >= 0 ? M << s : M >> -s;
}
// Ours: the phase is held at 2^32 to a cycle so that it wraps by itself; an increment in those units, and in hertz.
static inline uint32_t op_inc32(uint32_t inc) { return inc << (32 - cal::OP_PHASE_BITS); }
static inline double op_hz(uint32_t inc) { return inc * (double)SR / (double)(1u << cal::OP_PHASE_BITS); }
// The sine and the level path go through math/fastmath.h, which is where the backend is chosen.
static inline double fsin(double ph) { return fm::sin_turns(ph); }
static inline float fwin(int fam, int s, double x) { double y = x * 1024.0; int i = (int)y; if (i >= 1024) return 0.f; float f = (float)(y - i); const float* w = g_win[fam][s]; return w[i] + (w[i + 1] - w[i]) * f; }
// The per-sample level path: anything at or under -128 dB is silence, the rest is the backend's.
static inline double db2lin_fast(double db) { return db <= -128 ? 0.0 : fm::db2lin(db); }

// A grain runs at the carrier frequency it was fired with, latched beside the carrier phase the grain already resets. The level is not latched with it, it follows cal::LEVEL_SLEW_MS (see op_sample).
struct WinGen { double w = 1.0, c = 0.0, fc = 0.0; bool on = false; };
struct OpState {
    uint32_t phase = 0;           // 2^32 to a cycle, stepped by op_inc32(inc): the phase generator (cal.h)
    WinGen g[2]; int nextGen = 0; double fphase = 0;   // the formant's grains
    double word = 0; uint32_t inc = 0;   // the operator's frequency word and its increment (refresh_ctl)
    EG eg; FreqEG feg;
    EG ueg; FreqEG ufeg; double nphase = 0, lp[2] = {}, walk = 0; uint32_t rng = 0x12345678;   // the unvoiced carrier, in turns, and its walk
    double att = 0, attS = 0, fop = 0, fw = 0, wl7 = 1, uatt = 0, uattS = 0, ninc = 0, ucoef = 0, udrift = 0, uwidth = 0; int bw = 0, ratio = 0;   // refresh_ctl
    double gain = 0, gto = 0, dg = 0, ugain = 0, ugto = 0, dug = 0;   // the Fast steps' gain ramps, voiced and unvoiced (render_chan)
    double out = 0;                                                   // this sample's voiced output, for tools/wavetrace
};

// A grain runs at the carrier frequency it was fired with. The chip resets the carrier phase at every
// grain, and the frequency it then runs at is the one latched with that reset, so a write lands on the
// grain after it rather than bending the one in flight.
//
// `gain` is the operator's level and it is not latched here: it arrives already slewed, one pole at cal::LEVEL_SLEW_MS.
// The smoothing is a fixed time. A level applied per sample cuts the grain in flight and clicks at an Fseq frame boundary; a level latched to the grain cures that and goes too far, since at a low fundamental a grain lasts tens of milliseconds and the level cannot wait that long.
//
// The unvoiced operator's level glides on the same constant (FSVR#103, uattS in render_chan). Stepped, every Fseq frame that opened a noise operator was a step in the output: A019 with Fseq 59, 64 or 77 clicked, where the FS1R does not. A frame jump on the FS1R rises 10-90 % in 1.7 to 2.0 ms, and in 1.5 to 1.9 with this glide.
//
// The sine, the stored waves and the odd and res forms read the phase generator's own integer phase, stepped
// by `inc` (cal.h); the formant runs its grains' carriers at `fop`, the same increment in hertz. ph0 is ours,
// op byte 0x1C's initial phase in turns (docs/Differences.md): it turns the operator's own phase, where pm is
// what the modulators send.
inline double op_sample(OpState& s, const OpV& v, double f0, double fop, uint32_t inc, int ratio, double pm, double gain, double ph0 = 0) {
    if (v.form <= 6) {
        s.phase += op_inc32(inc);
        const double ph = s.phase * (1.0 / 4294967296.0) + ph0;
        if (v.form == 0) return gain * fsin(ph + pm);
        // all1 and all2 are a stored waveform read by phase, the same way the sine is: one period of the skirt's window with its mean taken out, so a modulator reaches it as phase like any other form.
        // all2's lines are the symmetric window's own Fourier row C(p, p/2 - k) and all1's the asymmetric window's. At skirt 0 the DC-free sin^2 is -cos / 2, the single partial the FS1R gives, and the mean is gone because the FS1R's is.
        if (v.form <= 2) {
            double x = ph + pm; x -= fm::floor_exact(x);
            return gain * cal::FORM_LEVEL * 2.0 * (fwin(v.form == 1, v.skirt, x) - g_winDC[v.form == 1][v.skirt]);
        }
        // odd and res: a window times a carrier, each addressed by its own phase word with the modulation added to both, which is how Yamaha's patent for the chip's forms builds its windowed carrier (US5610354, FIG. 11: adders 42a/42b put the modulation into the window's address, 45a/45b into the carrier's, and the carrier restarts with the window).
        // The window's and the carrier's phases are the operator's own, so with no modulator this is the windowed grain train described below, line for line, and a fixed operator at 0 Hz still shapes whatever modulates it, as a sine does.
        //
        // odd1/odd2 are a group that starts at the operator's own frequency: the FS1R gives partials 1 and 3 on both, and a window twice a period under a carrier at the operator's frequency puts its lines at fop, 3 fop, 5 fop and nowhere else.
        // res1 and res2 put a three-partial group on harmonic ratio + 1, at every ratio 0..99 of register 0x230, and are otherwise identical: the peak on partial ratio + 1, the two partials either side 6 dB down, nothing else, and the peak level flat across the whole range.
        // The window is one period, FIXED, and the bandwidth (register 0x218) has nothing to do with it: it opens the formant's window and leaves the other six forms as they are. Voice byte 6 reaches these forms as register 0x230, the ratio above.
        // The "1" and "2" of each pair differ in the window's shape (g_win), not its length: a sin^2 window exactly one period long makes the two partials either side of res's peak 6.02 dB down and kills everything beyond them, as the FS1R does at every setting.
        double w, c;
        if (v.form <= 4) { w = 2 * ph + pm; c = ph + pm; }                                   // odd: the window twice a period
        else { const double f = ph - fm::floor_exact(ph); w = f + pm; c = (ratio + 1) * f + pm; }       // res: on partial ratio + 1
        w -= fm::floor_exact(w);
        return gain * cal::FORM_LEVEL * 2.0 * fwin(v.form == 3 || v.form == 5, v.skirt, w) * fsin(c);
    }
    // The formant: grains under a window at the fundamental (its bandwidth curve in cal.h), the asymmetric window, each restarting its carrier, which is what holds its peak still while the fundamental moves.
    const bool modulated = pm != 0;   // ph0 is ours and does not keep a grain reading
    pm += ph0;
    const double wl = std::min(s.wl7, 2.0);
    s.fphase += f0 / SR;
    if (s.fphase >= 1) {
        s.fphase -= 1; WinGen& g = s.g[s.nextGen]; g.on = true; g.w = 0; g.fc = fop; g.c = 0.0;
        s.nextGen ^= 1;
    }
    double winc = f0 / (wl * SR), y = 0;
    for (int k = 0; k < 2; k++) {
        WinGen& g = s.g[k]; if (!g.on) continue;
        // The modulation goes into the window's address as well as the carrier's (US5610354 FIG. 11, adders 42a/42b and 45a/45b), and the address wraps, so a modulated grain does not end where the window does: it rests at the window's end and the modulation keeps reading it, until its slot fires again.
        // So a formant at 0 Hz under a sine plays at the same level whatever its bandwidth or the note.
        // Unmodulated, a grain is its window and nothing after.
        g.w += winc;
        // ponytail: a modulator at exactly 0.0 that sample reads as unmodulated; an input flag if that ever shows.
        if (g.w >= 1 && (!modulated || gain == 0)) { g.on = false; continue; }
        double wa = std::min(g.w, 1.0) + (modulated ? pm : 0.0); wa -= fm::floor_exact(wa);
        y += gain * fwin(1, v.skirt, wa) * fsin(g.c + pm); g.c += g.fc / SR;
    }
    if (cal::FRMT_NORM != 0.0) y *= pow(1.0 / wl, cal::FRMT_NORM);
    return y;
}

// Ours, for a display: voiced operator v alone through op_sample, n points over two periods (operator.cpp).
void op_wave(const OpV& v, float* out, int n);
