# src/presets: voices, performances, the EPROM and sysex

The FS1R's data formats and how they get in: the factory banks of a user's own EPROM image (`-r`), sysex bulk dumps and parameter changes. **Matches the FS1R**: it decodes the data as the FS1R does and the Data List describes, and a difference from the FS1R is a bug.

| File | What |
|---|---|
| `voice.h` | `Voice`, `OpV` (voiced operator), `OpU` (unvoiced), the decoders and the DX7 VCED/ACED conversion |
| `performance.h` | `Perf`, `Part` (with its controller state), `Fseq`; the 192 common bytes the effect host reads in place |
| `rom.h` | `Rom` and the loaders: EPROM banks, sysex, parameter change |
| `patch.cpp` | `decode_voice`, blank/default voices, `convert_dx7`, `init_perf`, `Part::reset_ctl` |
| `rom.cpp` | EPROM and sysex loading, bank selection, single-parameter writes, a part byte's handler run when it changes. Taking a performance cuts the sound first, as the FS1R does, and taking a voice silences its part |

## Rules
- The structs hold the raw bytes (`raw`, `p`, `c`, `fx`) next to the decoded fields: the engine and the effect host read the bytes as the FS1R does.
- A new parameter: decode it in `patch.cpp`, name its field in `voice.h` or `performance.h`, and cite the Data List.
