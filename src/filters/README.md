# src/filters: the voice filters

The FS1R's per-voice resonant filter: one 4-pole ladder per tone-generator channel with LP24/LP18/LP12, HP, BP and notch taps, plus the filter host that programs it. **Matches the FS1R**: it computes exactly what the FS1R's filter program computes, so a difference is a bug, and `dsp_check` says so. The FS1R has 16 filtered voices, FSVR one per channel, up to 128 (`docs/Differences.md`).

| File | What |
|---|---|
| `voice_filter.h` | `VoiceFilter`: one voice's ladder, its coefficients and taps, and our cutoff glide (`glide`, 0 is the FS1R's step; `docs/Differences.md`): the reference the lane kernel is written from. Start here |
| `lanes.h` | `Lanes`: the sixteen voices of a bank side by side, a number a row, as `kernel/isa/bank_kernel.cpp` runs them; `Shared`, the program's constants |
| `filter.h`, `.cpp` | `Filter`: a bank of 16 voices for every sixteen channels the engine can hold, eight in all, each with its filter host and `Modulators`; turns the host's writes into the lanes' coefficients and runs a bank's sample on the lane kernel. Ours: a bank past the second rests while it has nothing to do (`Bank::resting`, `docs/Differences.md`) |
| `filter_host.h`, `.cpp` | the filter host (**Matches the FS1R**): the part's filter parameters into register writes |

## Rules
- Bit-exact: built without FMA contraction (`CMakeLists.txt`, the `vop3host` library). `dsp_check` pins the output; ctest runs it.
- The host talks to the section only through `dsp/sink.h`.
