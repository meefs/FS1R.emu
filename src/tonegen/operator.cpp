// tonegen/operator.cpp - the grain windows and the operator display. Tuned: the constants are in fs1r/cal.h.
#include "tonegen/operator.h"
#include <vector>
#include "fs1r/tables.h"

float g_win[2][8][1025];       // [asymmetric?][skirt]: the two window families
float g_winDC[2][8];           // each window's mean, which a grain under a DC carrier does not carry
uint16_t g_opMant[1024];       // the phase generator's mantissas (cal.h, op_increment)
double g_slewN[65];            // 1 - (1 - LEVEL_SLEW_K)^n: the slew's step over a block of n

void init_tables() {
    fm::init();
    // floor(2048 * 2^(i / 1024)) (cal.h). The nearest entry to a whole number past i = 0 is 6.8e-4 away (i = 905), so libm's exp2 floors every one the same on every platform.
    for (int i = 0; i < 1024; i++) g_opMant[i] = (uint16_t)std::floor(cal::OP_MANT_ONE * std::exp2(i / 1024.0));
    for (int n = 1; n <= 64; n++) g_slewN[n] = 1.0 - pow(1.0 - LEVEL_SLEW_K, n);
    // The grain window is sin^p with p = 2 * 2^skirt on every form, and there are two families. The "2" forms (all2, odd2, res2) are the symmetric window, whose line amplitudes are the binomial row C(p, p/2 - k).
    // The "1" forms (all1, odd1, res1) and the formant are the SAME exponent on the rising half only; the falling half stays sin^2 whatever the skirt, a rise that sharpens with the skirt on a fall that never moves.
    // The formant is that same asymmetric window stretched over its bandwidth.
    for (int f = 0; f < 2; f++)
        for (int s = 0; s < 8; s++) {
            double p = cal::WIN_SKIRT * pow(cal::WIN_SKIRT_STEP, s);
            double acc = 0;
            for (int i = 0; i <= 1024; i++) {
                double x = sin(PI * i / 1024.0);
                g_win[f][s][i] = (float)pow(x, f && i > 512 ? cal::WIN_SKIRT : p);
                if (i < 1024) acc += g_win[f][s][i];
            }
            g_winDC[f][s] = (float)(acc / 1024.0);
        }
}

// Ours, for a display only: the FS1R has no such readout. Voiced operator v alone, from the same op_sample the channels play, at a nominal 100 Hz with no modulation: n points over two periods, scaled to a peak of 1.
// The grain forms open their first window a period in, so a period runs first and is dropped. The formant's carrier is put four harmonics up so that its grains show, and its window and the res forms' ratio come from byte 6 as notes.cpp hands them to a channel.
void op_wave(const OpV& v, float* out, int n) {
    OpState s;
    const double f0 = 100.0;
    double pitch = f0, fop = v.form == 7 ? 4.0 * f0 : f0;
    if (v.form == 7) {
        int tw = TRANS[clampi(v.transpose + 24, 0, 48)];
        if (tw) pitch *= fm::exp2(tw / 1024.0);
    }
    s.wl7 = std::min(cal::FRMT_WL_MAX, pitch / (cal::FRMT_BW_HZ0 * fm::exp2(v.bw / cal::FRMT_BW_DB)));
    const int period = (int)(SR / f0);
    const uint32_t inc = (uint32_t)std::lround(fop / SR * (1u << cal::OP_PHASE_BITS));
    for (int i = 0; i < period; i++) op_sample(s, v, pitch, fop, inc, v.bw, 0, 1);
    std::vector<double> y(2 * (size_t)period);
    double peak = 1e-9;
    for (auto& x : y) peak = std::max(peak, std::fabs(x = op_sample(s, v, pitch, fop, inc, v.bw, 0, 1)));
    for (int i = 0; i < n; i++) {   // each point the largest sample of its share, so a narrow pulse still shows
        size_t a = (size_t)i * y.size() / std::max(n, 1), b = std::max(a + 1, (size_t)(i + 1) * y.size() / std::max(n, 1));
        double m = y[a];
        for (size_t k = a; k < b; k++)
            if (std::fabs(y[k]) > std::fabs(m)) m = y[k];
        out[i] = (float)(m / peak);   // the shape, full height
    }
}
