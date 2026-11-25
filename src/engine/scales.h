// engine/scales.h - the FS1R's byte-to-register scales. Matches the FS1R.
#pragma once
#include <algorithm>
#include "fs1r/tables.h"
#include "math/fastmath.h"

static inline int to127(int v) { return std::min(255, (((v & 0xFF) << 1) * 0xA5) >> 7); }   // 0..99 -> 0..127
static inline int eb70(int v) { return (((v & 0xFF) << 1) * 0xA5) >> 8; }                   // 0..99 -> 0..127 (bandwidth)
// Pan key scaling. At a PAN SCALING byte of 0, the extreme, the index is 64 - 4 * (note - 60) / 3, hard right at note 12 and hard left from note 108 up, which is 64 index steps per 48 semitones. In the FS1R's own 0..255 pan domain a byte of 0 is the whole 128 of one side.
static inline int pan_index(int base, int scaling, int note) {
    return base + ((clampi(scaling, 0, 100) - 50) * 64 / 50) * (note - 60) / 48;
}
static inline int keygroup(int n) { return std::max(0, (KEYFACT[clampi(n, 0, 127)] >> 2) - 3); }
// key tracking of fixed/formant frequencies, notescale 0..99, pm = pitch word - C3
static inline int keytrack(int ns, int pm) { int k = eb70(ns); return k ? ((k + 1) * pm) / 128 : 0; }
// frequency velocity sensitivity, b = raw sysex nibble (sens + 7)
static inline int fvs_term(int b, int vel) {
    b &= 0xF;
    int m = (b & 8) ? FVSTAB[1 + (b & 7)] : FVSTAB[7 - (b & 7)];
    int v = (vel - 64) * m; if (!(b & 8)) v = -v;
    return v >> 3;
}
// velocity attenuation, b = raw sysex nibble (sens + 7), 0.375 dB units
static inline int vel_att(int b, int vel) {
    b &= 0xF; int s, t;
    if (b & 8) { s = (b & 7) + 1; t = VELW[vel]; } else { s = 7 - (b & 7); t = VELW[127 - vel]; }
    return std::min(255, (15 - 2 * s) + ((s * 32 * t) >> 8));
}
