// fs1r/hardware.h - the FS1R's own fixed numbers.
//
// Matches the FS1R. Every value here is a fact about the hardware, not a choice and not a fit.
// Nothing in this file is a calibration knob, so if one of these disagrees with a real FS1R the value is wrong, not in need of tuning.
//
// Constants tuned to match a real FS1R are in fs1r/cal.h. Quantities that are ours rather than the hardware's are in engine/tuning.h.
#pragma once

static const int SR = 48000;   // the hardware rate; fsvr::Device resamples to the host
static const int NCHAN = 32;
static const int FILTER_CHANNELS = 16;          // the channels wired through the filter chip, 0 to 15
static const double PI = 3.14159265358979323846;
static const double CPU_HZ = 28000000.0;         // SCI BRR 27 gives exactly 31250 baud at 28 MHz
static const double TICK_HZ = CPU_HZ / 16.0 / 9099.0;   // MTU2 TGRA compare every 0x238B counts at clock/16 -> 192.3 Hz LFO/PEG/portamento tick
