# Working on FSVR

FSVR is a software recreation of the Yamaha FS1R. The engine is a C++ rewrite of everything the FS1R does between a MIDI message and the sound: its control logic, its tone generators, and its filters and effects. Most of it behaves exactly as the FS1R does. The tone generator's sound is a model whose constants are tuned to match a real FS1R, and they all live in one file.

That split decides how you should answer almost any question here.

## Read this first

- **The folder READMEs.** Every `src/` folder has one, saying what the folder is, whether it matches the FS1R or is tuned, each file's job and the folder's rules
- **[docs/Differences.md](docs/Differences.md)** the places FSVR differs from the FS1R on purpose
- **[docs/plugin_guide.md](docs/plugin_guide.md)** the plug-in as a user sees it, and **[docs/editor.md](docs/editor.md)** how the editor is built
- **[docs/performance.md](docs/performance.md)** what the engine costs and how a change to that cost is measured

## Names in the repo

Contributors go by their GitHub handles, not their chat display names, in commits, release notes and docs, so one person is one name everywhere and the history stays greppable. The two collaborators are **rgwan**, who owns the hardware and the dumps, and **jameshansen**, who owns the project.

## Where things live

The path tells you what a difference from a real FS1R means.

| Path | What it is | A difference from the FS1R means |
|---|---|---|
| `src/fs1r/` | the FS1R's values: `hardware.h` from the board, the ROM's tables (generated), the DSP register map, and `cal.h` | a value is wrong, or for `cal.h` not yet tuned. Nothing else lives here |
| `src/fs1r/cal.h` | **Tuned**, the calibration surface | this is the file you edit to tune |
| `src/engine/`, `src/presets/`, `src/effects/host/`, `src/filters/filter_host.*` | **Matches the FS1R** | you have a bug |
| `src/tonegen/`, `src/modulators/` | **Tuned**, a model whose constants are in `cal.h` | it needs tuning |
| `src/filters/`, `src/effects/`, `src/dsp/` (the DSP) | **Matches the FS1R**, bit for bit what its DSP programs compute | you changed the sound; `dsp_check` says so |
| `src/device/`, `src/math/`, `src/display/`, `src/io/`, `src/selftest/`, `src/console/` | **Ours**, no hardware counterpart | nothing. The hardware has no opinion |
| `src/kernel/` | **Ours**: the scalar code of `filters/` (and in time `tonegen/`) a few lanes at a time, compiled once per instruction set | nothing; a hash that moves means a lane is not the scalar code, and `dsp_check` says so |
| `src/engine/tuning.h` | our cost knobs | nothing, and changing one must not alter the output |
| `plugin/` | the plug-in: processor, bank manager, Import Audio | it never models synthesis, see below |
| `plugin/skin/` | the editor, edited in place with Hollow's web editor | nothing; the hardware has no opinion (`docs/editor.md`) |
| `hollow/` | the plug-in framework, Hollow's `framework/` | nothing. Keep it in step with Hollow's copy |
| `tests/` | the checks `build.bat test`, `build.bat plugin` and CI run | |
| `tools/` | developer utilities, and the generators of the tracked tables and banks | |
| `presets/`, `demosongs/` | the factory banks, and the FS1R's fifteen demo songs as MIDI files | |

## The rules

**Every `src/` folder has a `README.md`: read it first, keep it true.** It says what the folder is, its status (Matches the FS1R, Tuned or Ours), each file's job and the folder's rules. Before changing a folder, read its README; it is faster than reading the code to find where something lives. In the same commit as any change that adds, removes, renames or moves a file, or changes what a file does, update that folder's README (and the table above if a folder's status changes). A new folder under `src/` gets a README in the commit that creates it. A README that disagrees with the code is a bug.

**Comments say what the code does.** A comment explains the behaviour and the reason for it in the synth's own terms. Firmware addresses, decompiler names, capture sessions and research notes stay out of this repository, and CI fails a build that carries a `FUN_` or `DAT_` label. The research that established a behaviour is kept by the maintainers outside the tree.

**Never invent a constant.** If a number is not the FS1R's own or measured on a real FS1R, it does not go in. Every tuned constant lives in `src/fs1r/cal.h`.

**A number fitted against one recording is not measured.** A model fitted against a single recording until the error stops moving has been wrong here more than once, and the modulation index shipped twice at the wrong value that way. Two datasets that agree, or it is not settled.

**The plugin never models synthesis.** It moves parameter values in and out of the engine as sysex, exactly as a hardware editor would, and learns state back from the engine's own bulk dumps. If you find yourself computing audio in `plugin/`, you are in the wrong layer. The morph square blends voice bytes into a voice bulk and Import Audio writes Fseq bytes on the scales `fsvr::Device` gives it; neither renders anything.

**Generated files are generated.** Do not hand-edit the banks in `plugin/generated/`, nor `src/fs1r/tables.h` or `rom_tables.*`. Change the generator or its source and re-run it; `plugin/generated/README.md` names each generator. `plugin/skin/` is not generated: it is the source, edited in place with the web editor of the Hollow project beside this repository (`python hollow.py editor` there opens it by default). Its params and `data/fs1r_sysex.json` were made from `plugin/generated/parameterDescriptions_fs1r.json` once, so a change to a parameter reaches the skin only by editing it there.

**Scale is not fidelity.** `src/engine/tuning.h` holds knobs that trade CPU for nothing else. If changing one moves a measurement, the value is wrong, not the hardware's.

## The editor's layout

The skin is placed by the pixel, so check it by the pixel. Render the page with `hollow-render` and look at it enlarged before a layout change is done; `build/x64/hollow/Release/hollow-render.exe plugin/skin page_operator out.png` draws one view.

- **Nothing is cut off.** A caption never runs under its widget's lamp, arrow or steppers, nor past the widget's edge. Key Sync was 66 px and its caption ran under the lamp, so it is 94. Widen the widget or shorten the caption rather than shrinking the font. `python ../Hollow/tools/overflow.py plugin/skin` measures every caption, every choice a dropdown can show and the widest value of every number field with the skin's own fonts, and it must report none.
- **What a widget draws stays inside its rect.** The widget clips, so a ring, glow or shadow drawn past the rect loses that side. The selected matrix box's ring lost its right side, and a column of its left and top, until it was drawn inside the box.
- **A value is centred on its control by its pixels.** A glyph's advance carries spacing after it, so text centred in a rect lands about a pixel left of a knob or fader centred on the same rect. Level's 99 sat 1 px left of the fader until its box moved 1 px right. Measure the render.
- **A section's controls share rows and columns.** Captions stand on one line (Amp Vel's with Level's), values on one line (Level's with OP Att's). A column under a button or tab is centred on it, its caption taking the button's width and its knob or fader centred on the same line: Level and its fader stand under Noise, and Amp Vel, EG Bias and OP Att under Voiced. They sat 6 and 7 px off until they were measured.
- **A number edited by step carries the steppers, as its neighbours do.** The `panels/label_plate` fill brings them (`"steppers"` in `skin.json`).
- **A parameter sits with the controls it acts on, under its panel name.** OP Att trims an operator's output, so it is in Amplitude beside Level, not in the Freq EG row. It reads OP Att, the owner's manual's name, rather than the Data List's "carrier level correction", and shows the unit's dB through the param's `labels`.

## Releases

Every release tag carries release notes: an annotated tag (`git tag -a vX.Y.Z -F notes.md`) whose body says what changed, the numbers, and which reports it answers. The Actions job's `generate_release_notes` alone leaves only a compare link, which is not release notes. Skip them only when told to for that release.

The release job reads that body off the tag *object* through the API, not from a checkout: `actions/checkout` peels a tag ref to its commit, so `git tag -l --format='%(contents)'` in the job returned the commit message and v0.4.7 shipped with it as the body (patched by hand afterwards). A lightweight tag has no object and fails the job on purpose. Check the release page's body after the run, not just the zips.

**The release page is the Download table, a rule, then the tag's notes.** The release job writes the table of the three installers (Windows, macOS, Linux, each linked on the tag, with how to run it) and the paragraph on per-format zips above the tag's message, as v0.5.0's page first had them by hand; so the tag holds only the notes. The notes follow v0.5.0's and v0.5.1's shape: a first line `FSVR vX.Y.Z: <what the round is>.`, a paragraph naming whose reports it answers, then one paragraph per change led by its point in bold, with the numbers, and a closing **Checks.** paragraph saying what passed and what was not run.

**Both collaborators are credited on every release.** The commit a release tag points at (the version bump, or the last commit before the tag) ends with a `Co-authored-by` line for whichever of the two is not its author:

- authored by jameshansen: `Co-authored-by: rgwan <h@iloli.bid>`
- authored by rgwan: `Co-authored-by: jameshansen <1550976+jameshansen@users.noreply.github.com>`

The line goes in that commit, never in the tag's message or the release notes, which the release page shows as they are. Any other commit credits the people who contributed to it, one `Co-authored-by` line each: the author of a pull request or commit it builds on or fixes, and anyone else who took part in the work. Never one for Claude or any other tool, on any commit. Put it in the commit before pushing, since adding it afterwards means rewriting a pushed commit.

**One release per request.** A release is cut when asked for, then the next one waits for the next ask: more changes pushed later in the same conversation go to `main` and stay there until the user says to release them. Do not chain point releases on your own because the previous one has already built.

**Increment the last digit only, unless the user names the version.** `0.4.7` goes to `0.4.8`, never to `0.5.0`, however much the round changed. The minor and major digits are the user's call and not an inference from the size of the diff: v0.5.0 was cut for four engine fixes and had to be retagged as v0.4.8. Ask, or bump the patch digit.

## Before you say it works

```bash
build.bat test
```

That builds the engine and the console, then runs the engine self check, which covers the effect chip, and `check_formant` and `check_presets`. All of them must pass.

If you touched the plug-in (`plugin/`, `hollow/` or the skin):

```bash
build.bat plugin
```

That builds every format, 64-bit and 32-bit, into `bin/<format>` and runs `check_plugin`, which drives the processor the way a host does, then `check_gui`, which drives the editor over it the way a mouse and a keyboard do: every menu, dialog, page and control. Anywhere else, `cmake -B build/plugin -S . -DFSVR_BUILD_PLUGIN=ON`, build, and `ctest -R "plugin|gui"`.

If you touched anything the audio path reaches, also run the render regression:

```bash
python tests/regress.py
```

Nineteen fixed cases against `tests/regress_ref.json`. It compares pitch, harmonic peaks, envelope, stereo width and centroid. **If it fails, that is a finding, not an inconvenience.** Only run `--update` when you meant to change the output and can say why in the commit message.

`regress.py` and `check_formant.py` drive `bin/fsvr_console.exe` on Windows and `bin/fsvr_render` anywhere else, picked by `tests/render.py` (the console is in `bin/` on every checkout but fails before `main()` under WSL, so the choice is made by running it, not by looking for the file). The stored reference holds on every platform: the random note phase, part pan 0 and the S&H LFO draw from each instance's own generator (`Synth::draw`), not from the C library's `rand()`, whose sequence differed between Windows and Linux and made every case fail on a fresh Linux checkout until 2026-10-26.

For a change that should not alter the output at all, prove it rather than assume it. Render a fixed case before and after and compare the file hashes:

```bash
bin/fsvr_console.exe -w before.wav -n 60 -d 3
```

For a change meant to make the engine cheaper, that proof is the gate and the number comes with it: `python tests/abcheck.py --build-base upstream/main` builds the baseline apart and renders it, `python tests/abcheck.py --ref build/ab/<sha> --tier A` says whether the nineteen cases and the Fat Line demo still render the same 16-bit audio (`--tier B` allows one step), and `bin\perfbench.exe --set standard --ref build/perf-<sha> --save build/perf-head` measures and gives the same verdict on its scenarios. A timing only means something against the other binary in the same session, alternated; `docs/performance.md` (Measuring) has the protocol and the record.

### Tracing a waveform

A change to how operators run (phase, key sync, ratios, the EGs, feedback, a form) is checked by tracing them, not by listening. `tools/wavetrace.cpp` plays one voice on a fresh engine at several notes, tone generator only, and prints every sounding operator's frequency, ratio to the fundamental, phase and **lead**: the phase less frequency times time since key-on, what the operator gained or lost against an oscillator left alone. A key-synced operator at a steady pitch leads by 0 at every note. A lead that moves with the note is something sweeping the pitch, which is how Bassline 2's frequency EG was told apart from a key sync bug. `-png` draws a few periods of the fundamental per note, the channel and then each operator, aligned on the fundamental's cycle, so notes that agree draw the same wave. Look at it enlarged.

```bash
build.bat trace
```

```bash
bin/wavetrace.exe -v presets/native/036_Bassline_2.syx -set 7:8=50 -n 36,48,60,72 -png build/trace.png
```

`-set 7:8=50` is operator 7's byte 8 (Freq EG Init back to 0); `-set 0x2C=16` a common byte; `-r` and `-p` take a voice from a ROM image. Elsewhere it is the CMake target `wavetrace`. When a trace settles something, pin it in `src/selftest/selftest.cpp` as the key sync checks are, so a later change cannot move it silently.

## Finding work

Open work, bugs and fidelity items alike, is in the issues:

```bash
gh issue list -R musicastudio/FSVR
```

## Things that will trip you up

- The ROM image is not in this repo. The console's and `fsvr_render`'s `-r`, the ROM cases of `tests/regress.py` and the `tools/extract_*.py` generators need your own v1.20 dump, found through `$FSVR_ROM` or `../FS1R_DISASM/roms/`. Without one, `regress.py --factory` takes the ROM cases from the factory banks in `plugin/generated`
- The engine always runs at 48 kHz whatever the host rate. `fsvr::Device` resamples on the way out, so patch timing is identical everywhere
- `TICK_HZ` is 192.3 Hz. Only `render`, `render_chan` and `refresh_ctl` run per sample; everything `tick()` reaches is control rate
- The tables `init_tables()` fills once (the grain windows, their means and the phase generator's mantissas) have external linkage on purpose. Making them file-static again would give every translation unit its own zeroed copy and the engine would render silence
- The plug-in's processor runs on two threads: the audio thread sends param changes and MIDI and renders, a worker loads patches and files and reads the engine back into the params. `ctl` guards what both touch, and the audio thread only ever try-locks it. `saving()` finishes whatever the worker has not reached before the host saves
- Commits here never carry a `Co-Authored-By` line for Claude or any tool; the one co-author line is the other collaborator's, on the commit a release tag points at (Releases, above)
