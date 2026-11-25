// presets/performance.h - performance, part and Fseq data. Matches the FS1R.
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "presets/voice.h"

struct Fseq {
    bool valid = false; char name[9] = "";
    int nframes = 0, loopStart = 0, loopEnd = 0, loopMode = 0, speedAdj = 64, velTempo = 0, pitchMode = 0, noteAssign = 60, tuning = 63, delay = 0, endStep = 0;
    uint8_t frame[512][50];   // pitch hi/lo, 8 voiced freq hi, 8 lo, 8 voiced level, 8 unvoiced hi, 8 lo, 8 unvoiced level
    void from_bytes(const uint8_t* h, const uint8_t* f, int frames) {
        memcpy(name, h, 8); name[8] = 0;
        loopStart = h[0x10] << 7 | h[0x11]; loopEnd = h[0x12] << 7 | h[0x13]; loopMode = h[0x14] & 1; speedAdj = h[0x15] & 0x7F;
        velTempo = h[0x16] & 7; pitchMode = h[0x17] & 1; noteAssign = h[0x18] & 0x7F; tuning = h[0x19] & 0x7F; delay = h[0x1A];
        nframes = std::min(frames, 128 * ((h[0x1B] & 3) + 1)); endStep = std::min(nframes - 1, h[0x1E] << 7 | h[0x1F]);
        memcpy(frame, f, (size_t)nframes * 50); valid = true;
    }
};
// Performance banks in the EPROM table of 384: the three preset banks, 128 each, in order A, B, C, matching the Data List's performance lists entry for entry ("Zap !" is Preset A 1, "Sweepy Voice" Preset B 1 and "UprightPiano" Preset C 1).
// Preset C is the bank for the G50 guitar controller, "the maximum MIDI receive channel for these voices is 6, and the pitch bend range is -12 ... +12" (owner's manual page 21), which is exactly what entries 256-383 hold and nothing else does.
// INTERNAL is the user's own battery-backed bank, not ROM, so a bank select for it lands on Preset A, which is what the FS1R ships holding.

struct Part {
    uint8_t p[52]; Voice voice;
    // controller state (MIDI)
    int bend = 0;            // (msb - 64) * 16 + (lsb >> 3)
    int bendLsb = 0;         // ours: the whole LSB, for a 14-bit bend (bend_word)
    // The same pair as each MIDI channel last sent it. A part that receives a range of channels bends each
    // note by its own channel's (Data List 2.2.4, "can sound separately according to Pitch Bend Change on
    // different channels"); bend and bendLsb stay the last received, which the PB source reads.
    int bendAt[16] = {}, bendLsbAt[16] = {};
    int expr = 254;
    // The per-part controller reset: expression back to 0xFE, bend and every controller source to zero. Reset All Controllers runs it, and so does loading a performance, which walks the four parts calling it, so a performance bulk after an expression change plays at full level.
    void reset_ctl();
    // Controller sources in the FS1R's own bit order: KN1-4, MC1, MC2, PB, CAT, PAT, FC, BC, MC3, MW, MC4.
    // The knobs and MIDI controls are stored bipolar as (v - 64) * 2, the physical controllers as the raw value, so the range is -128..127 either way.
    int src[14] = {};
    int held[32]; int nheld = 0; int lastPitch = -1;   // mono handling and portamento start
    int rpnM = 127, rpnL = 127; bool nrpnSel = false;   // RPN / NRPN selection state
    bool sustain = false;
    // Controller destinations 17 to 33 edit part bytes, and each writes its byte as a copy of the byte plus the scaled controller, leaving the copy alone. The copy is 17 bytes a part, in destination order, filled from the part at a load and by every ordinary parameter change, a MIDI CC's included.
    // So the byte reads as the FS1R's edit buffer does, a controller never compounds on itself, and an edit moves the base the controller works from.
    static constexpr uint8_t CTL_BYTE[17] = {0x0B, 0x0E, 0x13, 0x12, 0x18, 0x19, 0x1F, 0x1A, 0x1B, 0x1C, 0x20, 0x21, 0x22, 0x23, 0x0A, 0x1D, 0x1E};
    uint8_t ctlBase[17] = {};
    void fill_ctl_base() { for (int k = 0; k < 17; k++) ctlBase[k] = p[CTL_BYTE[k]]; }
    void ctl_edit(int at) { for (int k = 0; k < 17; k++) if (CTL_BYTE[k] == at) ctlBase[k] = p[at]; }   // an ordinary edit of byte at
    int rcv() const { return p[1] == 0 ? 0x7F : p[4]; }   // Voice Bank "off" (byte 1) receives nothing, whatever the channel (owner's manual p.63)
    // Rcv Max (byte 3), parts 1 and 2 only: the last channel of the range from Rcv Ch, or -1 for none. Off (0x7F),
    // a Max at or below Rcv Ch, and pf all mean the single channel (owner's manual p.25, Data List 2.2.3.2).
    int rcvMax(int part) const { const int rc = rcv(); return part < 2 && rc <= 0x0F && p[3] <= 0x0F && p[3] > rc ? p[3] : -1; }
};
struct Perf {
    uint8_t c[80]; uint8_t fx[112]; Part part[4];
    char name[13];
};
static_assert(offsetof(Perf, fx) == 80, "the 192 common bytes, as the effect host reads them in place");

int bank_perf_index(int lsb, int prog);
void init_perf(Perf& P);
