// tonegen/envelope.h - the operators' amplitude and frequency EGs. Tuned: the constants are in fs1r/cal.h.
#pragma once
#include <algorithm>
#include <cmath>
#include "fs1r/hardware.h"
#include "fs1r/cal.h"
#include "fs1r/tables.h"
#include "math/fastmath.h"

static inline int egrate(int t) { return ((99 - clampi(t, 0, 99)) * 0xA4) >> 8; }          // EG time -> chip rate 0..63
// The chip's own rate scaling: register 0x50 is the operator's time scaling 0..7 and register 0xC0 the key code.
// The rate moves by trunc(tscale * x / 8), x being the chip's key offset at key codes 77..113, and each slope lands on the (4 + (q & 3)) << (q >> 2) ladder. x saturates at -13 below 82 and +13 above 109, and it is symmetric about 95.5.
static const signed char EG_KEYOFF[37] = {   // key codes 77 .. 113
    -13, -13, -13, -13, -13, -12, -12, -10, -10, -8, -8, -5, -5, -4, -4, -2, -2, -2, -1,
      1,   2,   2,   2,   4,   4,   5,   5,   8,   8, 10, 10, 12, 12, 13, 13, 13, 13};
static inline int eg_keyoff(int c0) { return EG_KEYOFF[clampi(c0 - 77, 0, 36)]; }
static inline int eg_ratescale(int tscale, int c0) {
    int x = (tscale & 7) * eg_keyoff(c0);
    return x < 0 ? -((-x) >> 3) : x >> 3;                                                 // truncates toward zero
}
// Chip EG rate 0..63 -> seconds for a full 96 dB traverse. The FS1R maps time T to rate (99-T)*0xA4>>8, the exact inverse of the DX7's (R*41)>>6, and its DX7 converter uses T = 99 - R, so the chip is taken to time its EG like the DX7 EGS: increment (4 + (q & 3)) << (q >> 2) per 64 samples on a 2^28 = 96 dB scale (Dexed). 6.6 ms at 63, 380 s at 0.
static inline double rate_secs(int q) { q = clampi(q, 0, 63); return pow(2.0, (double)(26 - (q >> 2))) / (4 + (q & 3)) / SR; }   // the C pow: no template for the lane kernels to instantiate
struct EG {                      // amplitude EG on the chip: hold, 4 segments. Modelled shape and timing, DX7 style
    int stage = 5; double cur = -200, target = -200, rate = 0, holdLeft = 0; bool rising = false;
    int L[4] = {}, R[4] = {}; int hold = 0; int rs = 0;
    int nN = 0; double rateN = 0;   // advance's: the block it last ran and the segment's step over it
    static double lvl_db(int a) { return a >= 63 ? -200.0 : -cal::EG_LEVEL_DB * a; }   // 6-bit attenuation (LEVTAB >> 1), 1.5 dB per step
    void start(const int* lv, const int* rt, int h, int rateScale) {
        for (int i = 0; i < 4; i++) { L[i] = lv[i]; R[i] = rt[i]; } hold = h; rs = rateScale; cur = lvl_db(L[3]); stage = 0;
        int hr = egrate(h); if (hr < 0x3F) hr = std::min(hr + 4, 0x3E);   // +4 only below 0x3F
        // 0x3F is the FS1R's "no hold", the one value it does not offset. Anything else holds for half a traverse at that rate plus a fixed lag, both in cal.h.
        holdLeft = hr < 0x3F ? (rate_secs(hr) * cal::EG_HOLD_FRAC + cal::EG_HOLD_LAG) * SR : 0;   // the key code does not reach the hold, which is the same length at every note 12..120
        if (holdLeft < 1) next(1);
    }
    void next(int s) {
        stage = s; nN = 0; if (s > 4) { target = -200; return; }
        target = lvl_db(L[s - 1]);
        double secs = rate_secs(R[s - 1] + rs);
        rising = target > cur;
        // A rising segment does not crawl up from silence: the chip is already at EG_ATTACK_FLOOR one
        // millisecond in, so the climb starts there, or at the target if that is lower still.
        if (rising && cur < cal::EG_ATTACK_FLOOR) cur = std::min(target, cal::EG_ATTACK_FLOOR);
        rate = rising ? 1.0 - exp(-1.0 / (secs * cal::EG_ATTACK_K * SR + 1)) : 96.0 / (secs * SR + 1);
    }
    void release() { if (stage < 4) next(4); }
    inline double tick() {
        if (stage == 0) { if (--holdLeft <= 0) next(1); return cur; }
        if (stage > 4) return cur;
        if (rising) { cur += (cal::EG_OVERSHOOT - cur) * rate; if (cur >= target) { cur = target; if (stage < 3) next(stage + 1); else if (stage == 4) stage = 5; } }
        else { cur -= rate; if (cur <= target) { cur = target; if (stage < 3) next(stage + 1); else if (stage == 4) stage = 5; } }
        return cur;
    }
    // Ours, for the Fast steps (Device::setSpeed): n samples at once, the segment's own law across the block and
    // a segment's end at the block's end, which moves a boundary by at most n - 1 samples.
    inline double advance(int n) {
        if (stage == 0) { if ((holdLeft -= n) <= 0) next(1); return cur; }
        if (stage > 4) return cur;
        if (nN != n) { nN = n; rateN = rising ? 1.0 - pow(1.0 - rate, n) : rate * n; }
        if (rising) { cur += (cal::EG_OVERSHOOT - cur) * rateN; if (cur >= target) { cur = target; if (stage < 3) next(stage + 1); else if (stage == 4) stage = 5; } }
        else { cur -= rateN; if (cur <= target) { cur = target; if (stage < 3) next(stage + 1); else if (stage == 4) stage = 5; } }
        return cur;
    }
    bool done() const { return stage == 5 || (stage == 4 && cur <= -120); }
};
struct FreqEG {                  // init -> attack level -> 0. The level curve and the times are the FS1R's (FEGLVL and the EG rate conversion, registers 0x60-0x78).
    double cur = 0, target = 0, katt = 0, kdec = 0; int stage = 2, hold = 0;
    // The sysex byte is not linear in the register: FEGLVL maps 0..100 onto 0..255 with 128 the centre, and
    // half the displayed depth is only a fifth of the register offset. 128 register steps = FEG_SEMIS.
    static double feg_semis(int v) { return (FEGLVL[clampi(v + 50, 0, 100)] - 128) * cal::FEG_SEMIS / 128.0; }
    // A LINEAR RAMP in semitones, and the slope does not depend on the swing: four times the swing takes four times as long, which is what a ramp does and an approach cannot. At attack time 40 it runs at 4390 cents per second.
    //
    // The slope is the amplitude EG's own rate ladder read as pitch: a traverse at rate_secs(word) covers cal::FEG_TRAVERSE semitones, the 24 that a 96-unit scale quartered gives.
    // So the chip runs one stepper for both EGs and only the unit of the step differs, 0.375 dB on the amplitude side and a quarter tone here.
    // The chip's frequency EG plays rates 57 and 58, and 61 and 62, the other way round from the amplitude ladder, so attack times 1-2, 3, 7-8 and 9-10 land on the swapped rate. The amplitude EG keeps its own order.
    static int feg_rate(int t) { const int q = egrate(t); return q == 57 ? 58 : q == 58 ? 57 : q == 61 ? 62 : q == 62 ? 61 : q; }
    void start(int init, int att, int attT, int decT) {
        cur = feg_semis(init); target = feg_semis(att); stage = (init == 0 && att == 0) ? 2 : 0;
        katt = cal::FEG_TRAVERSE / (rate_secs(feg_rate(attT)) * SR);
        kdec = cal::FEG_TRAVERSE / (rate_secs(feg_rate(decT)) * SR);
        hold = cal::FEG_LATCH;
    }
    inline double tick() {
        if (hold > 0) { hold--; return cur; }
        if (stage == 0) {
            double d = target - cur;
            if (fabs(d) <= katt) { cur = target; stage = 1; } else cur += d < 0 ? -katt : katt;
        } else if (stage == 1) {
            if (fabs(cur) <= kdec) { cur = 0; stage = 2; } else cur += cur < 0 ? kdec : -kdec;
        }
        return cur;
    }
};
