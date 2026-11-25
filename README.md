<p align="center"><img src="icon.png" width="128" alt="FSVR icon"></p>

<h1 align="center">FSVR</h1>

<p align="center"><b>A software recreation of the Yamaha FS1R synthesizer.</b><br>Rewritten in C++ and checked against real hardware.</p>

<p align="center"><a href="../../releases/latest"><b>Download for Windows, Linux or MacOS</b></a></p>

<p align="center"><img src="docs/fsvr_standalone.png" alt="FSVR standalone"></p>



# Introduction

<b>A millennial FM monster, reborn.</b>

**Discord:** https://discord.gg/6sXu3GmkNm

The Yamaha FS1R is one of the most sought-after FM synthesizers ever made, and one of the hardest to find.

In 2025, Zhiyuan Wan ([rgwan](https://github.com/rgwan)) started taking a real FS1R apart to try and figure out how this masterpiece worked. After all, the FS1R isn't analog, it's essentially a purpose built computer running on a SuperH SH-2 processor supported by DSP chips, and FSVR's sound is still checked against Zhiyuan's dumps and recordings of that unit.

In mid-2026, James Hansen ([jameshansen](https://github.com/jameshansen)) picked up Zhiyuan's work and used a custom AI-assisted tooling framework to understand the synthesizer and rebuild it in software, which is what became FSVR. We're open about how much AI has helped with this project - the first version of the engine was created with Claude Fable 5.1, and much of the analysis and engine work since has been done with the help of Claude Opus 5 and Opus 5.5.

The biggest challenge was the effects, which ran on a processor with very little documentation. The [VOP3](https://github.com/musicastudio/VOP3) emulation core built by Wouter van Nifterick ([WouterVanNifterick](https://github.com/WouterVanNifterick)) gave us a reference point to finally understand the FS1R's effects and filters and rewrite them in C++. Wouter has since contributed a lot more to FSVR, including the effect and filter hosts, polyphony up to 128 voices, the multi-lane rendering that keeps CPU use down, and fixes to the unvoiced operators.

FSVR has had help from FS1R owners and plug-in developers too. Mario Kruselj ([mkruselj](https://github.com/mkruselj)) has tested FSVR side by side with a real FS1R and reworked the editor's pages, Dezzo ([dezzo](https://github.com/dezzo)) fixed the VST3 editor so Cubase keeps its edits, and DEE GROM ([PatriotBY](https://github.com/PatriotBY)) tracked down the clicks some formant sequences were making.

FSVR isn't an emulation, it's a rewrite of the FS1R's logic in C++, so it runs natively on Windows, macOS and Linux. Alongside the synth engine, FSVR has a friendly editor that makes it easy to edit the factory sounds and create your own.

We hope you enjoy it. If you find a bug or make a sound you're proud of, come and tell us on the Discord.

## Demos

### FS1R DEMO SONG 1: Vokodrone

**FS1R Hardware Recording**

https://github.com/user-attachments/assets/8afa466b-e7c4-4427-a058-392120f1a15d

**FSVR (v0.8.0)**

https://github.com/user-attachments/assets/bd0fc75f-4aa0-4bf9-9219-08d003f38e84

### FS1R DEMO SONG 2: Full Tines

**FS1R Hardware Recording**

https://github.com/user-attachments/assets/fc235492-d053-4abb-af04-8e1041c32d46

**FSVR (v0.8.0)**

https://github.com/user-attachments/assets/db4cbe31-0bfa-451b-b8e1-563970ff559d

## FS1R vs FSVR

The synthesis engine matches close to 1:1 with real hardware.

A short list of places FSVR behaves differently **on purpose** is kept in [docs/Differences.md](docs/Differences.md). Most are additions the FS1R never had: polyphony up to 128 voices, 32 filtered voices, 14-bit pitch bend, a gliding filter cutoff, user libraries without the FS1R's memory limits and Import Audio. `fsvr_render` keeps to the FS1R's behaviour unless given `-fsvr`.

## Builds

[The latest release](https://github.com/musicastudio/FSVR/releases/latest), also on [musica.studio](https://musica.studio/code/fsvr), has an installer for Windows, macOS and Linux that puts every format in place, and each format as its own zip, `FSVR-<OS>-<format>.zip`: CLAP, VST3, VST2 and standalone everywhere, an AU for Logic Pro on macOS, and on Windows `FSVR-Windows-VST2-32-DXi.zip`, a 32-bit VST2 carrying a DXi (Cakewalk's older plug-in format, registered with `regsvr32`). The macOS builds are universal, Apple silicon and Intel in one binary. No dependencies. [docs/plugin_guide.md](docs/plugin_guide.md) is the user guide.

Every push to `main` builds the same set through [GitHub Actions](https://github.com/musicastudio/FSVR/actions), so a build for an OS you do not own is always one Actions run away, including on a fork. `.github/workflows/build.yml` is four jobs:

- **engine**, on `windows-latest`, `ubuntu-latest` and `macos-latest`. Plain CMake with no downloads, then `ctest`, which runs the effect and engine self checks everywhere. Uploads `FSVR-console-windows`.
- **plugin**, on Windows x64, Windows x86, Linux and macOS. Installs the X11, ALSA, JACK and PulseAudio headers on Linux, configures with `-DFSVR_BUILD_PLUGIN=ON`, runs the plug-in's own checks (`check_plugin`, `check_gui`) on the 64-bit legs, and uploads `FSVR-plugin-windows`, `FSVR-plugin-windows-x86`, `FSVR-plugin-linux` and `FSVR-plugin-macos`, each holding every format for that platform.
- **installer**, on Linux. Checks out [musicastudio/Installer](https://github.com/musicastudio/Installer), packs the plug-in artifacts into it and builds `FSVR-Windows-Installer.exe`, `FSVR-MacOS-Installer.zip`, `FSVR-Linux-Installer` and `FSVR-MacOS-Update` (the bare binary the macOS updater runs), at the version in `CMakeLists.txt`.
- **release**, on a `v*` tag only. Splits the plug-in artifacts into one zip per format, named as FM8.plus names its own (`FSVR-Windows-VST2-64.zip`, `FSVR-Windows-VST2-32-DXi.zip`, `FSVR-MacOS-AU.zip` and so on), adds the installers, the console and `SHA256SUMS.txt`, and attaches them to a GitHub release. The installers' updater never reads GitHub: musica.studio mirrors each release and serves the update from there.

Each night `.github/workflows/nightly.yml` takes the newest build of `main` that passed every check, without rebuilding it, and publishes its installers and plug-in zips as the [nightly](https://github.com/musicastudio/FSVR/releases/tag/nightly) pre-release, which the updater never offers. A night when `main` has not moved publishes nothing. New nightlies and releases are posted to the Discord channel with their download links, as are new issues and pull requests.

### Building it

CMake 3.24 or later everywhere, C++17. The engine, the console and the self checks need no dependencies:

```bash
cmake -B build/cmake -S .                 # fsvrLib, bin/fsvr_render, and bin/fsvr_console on Windows
cmake --build build/cmake --config Release
ctest --test-dir build/cmake -C Release   # the self checks
```

The plug-in adds one flag. Its first configure downloads the CLAP SDK and [clap-wrapper](https://github.com/free-audio/clap-wrapper), which fetches the VST3 and AU SDKs, RtAudio and RtMidi, and Import Audio's decoders ([dr_libs](https://github.com/mackron/dr_libs) and [stb_vorbis](https://github.com/nothings/stb)), so it needs the network once:

```bash
cmake -B build/plugin -S . -DFSVR_BUILD_PLUGIN=ON
cmake --build build/plugin --config Release
ctest --test-dir build/plugin -C Release -R plugin   # the plug-in's own check
```

On Linux install `libx11-dev libasound2-dev libjack-jackd2-dev libpipewire-0.3-dev libpulse-dev pkg-config` first (Debian and Ubuntu names). On Windows a second build with `-A Win32` makes the 32-bit VST2 with the DXi in it.

Every format lands in a folder of its own under `bin/`: `bin/CLAP`, `bin/VST3`, `bin/VST2` and `bin/Standalone`, `bin/AU` on macOS, which is the only plugin format Logic Pro loads, and `bin/VST2 32-bit and DXi` from the 32-bit build. The 1408 factory voices, the 384 factory performances and the 90 preset formant sequences are built into the plugin, so its browser needs no EPROM image.

On Windows `build.bat` is the shortcut, MSVC x64 out of the VS 2022 Professional vcvars64:

```bat
build.bat            bin\fsvr_console.exe, the console
build.bat test       build and run the self checks
build.bat plugin     every plug-in format, 64-bit and 32-bit, then check_plugin and check_gui
```

Everything meant to be run lands in `bin/`; objects and the test binaries go to `build/`.

### The console

`bin/fsvr_console` is the test harness on top of the engine library. It is Windows only, since it uses WinMM for MIDI and waveOut for audio, but its offline render path needs no devices at all.

```bat
fsvr_console -l                                     list MIDI ports
fsvr_console                                        asks for a MIDI input once, remembers it in bin\fsvr_console.ini
fsvr_console -m 0 -o 0 -v presets\native\000_Ballad_EP.syx
fsvr_console -v presets\dx7\004_Pianotone1.syx      DX7-format presets, converted as the FS1R converts them
fsvr_console -r <eprom.bin> -p 128                  ROM voice, 0-255 native and 256-1407 the DX7 banks
fsvr_console -r <eprom.bin> -P 0                    ROM performance 0-383 with its voices and Fseq
fsvr_console -r <eprom.bin> -P 0 -f 29              override the Fseq with preset Fseq 1-90
fsvr_console -v presets\native\128_BagPipe.syx -w test.wav -n 60 -d 3     offline render, no devices needed
```

Options. `-m` MIDI input, `-o` MIDI output for dump and parameter replies, `-c` force all parts onto one MIDI channel (the default is each part on its own receive channel), `-v` a sysex file holding FS1R voice, performance or Fseq bulk dumps or a DX7 VCED dump, with `-p` picking the n-th dump in the file, `-r` a 2 MB EPROM image with `-p` voice, `-P` performance and `-f` Fseq, `-g` output gain, `-w` offline render with `-n` note, `-n2` a second note a third of the way in, `-cc num=val` control changes sent before the note, `-d` seconds, `-mono` part 1 mono with full-time portamento, `-selftest` the engine self check. `FSVR_DEBUG=1` prints the computed register values at every note on.

MIDI. Notes, bend, both aftertouches, the control numbers the system table assigns to KN1-4, MC1-4, FC, BC, Formant and FM, CC5/65 portamento, CC7 volume, CC10 pan, CC11 expression, CC64 sustain, CC91/94 sends, CC120/121/123/126/127, RPN 0-2, the ten NRPN 01 xx part parameters, bank select and program change in both performance and multi modes, MIDI clock, active sensing, FS1R bulk dumps and parameter changes across the whole map, and dump and parameter requests answered. The performance's controller sets route the sources to the destinations.

The `-r` examples take a 2 MB v1.20 EPROM image dumped from your own FS1R, with its 16-bit words byte-swapped into CPU order. FSVR doesn't include one, and nothing but the `-r` paths needs it.

### Checking a change

```bash
build.bat test                          # the engine self check, check_formant and check_presets
python tests/check_wav.py test.wav 60   # pitch, harmonics and envelope of one render
python tests/regress.py                 # the whole fixed preset list against the stored reference
```

[AGENTS.md](AGENTS.md) has the full list of what to run before a change is done.

## Repo Layout

**Engine.** No Windows, no host, no GUI. This is what the plugin links, through `src/fsvr.h`, the public header and `fsvr::Device`, and the only header the plugin includes. Each `src/` folder has a `README.md` with its files and rules; the summary:

`src/fs1r/` is the hardware's values and nothing else: what presets and the engine read, never code.

- `hardware.h` the FS1R's own numbers, from the board. Facts, never tuned
- `tables.h` the ROM's conversion tables, by `tools/extract_tables.py`; `algorithms.h` the 88-algorithm routing table
- `rom_tables.h`, `rom_tables.cpp` the filter and effect hosts' ROM tables, by `tools/extract_vop3_rom.py`
- `vop3_regs.h` the DSP registers the hosts write
- `cal.h` the calibration surface, every tuned constant in one place: **Tuned**, changed by measuring against a real FS1R and never by taste

The engine, one folder a part. **Matches the FS1R** parts behave exactly as the FS1R does, so a difference from a real one is a bug there, not a calibration. **Tuned** parts are models, with their constants in `cal.h`.

- `src/engine/` **Matches the FS1R**: `synth.h` the engine's shared declarations, including `struct Synth`; `notes.cpp` note on and off, operator setup, pitch, portamento, and the 192.3 Hz tick; `controllers.cpp` the eight controller sets, the three scalers and the 48 destinations; `fseq.cpp` formant sequence playback; `midi.cpp` MIDI in and sysex in and out
- `src/presets/` **Matches the FS1R**: `patch.cpp` voice, performance and part data as the FS1R decodes it, with the DX7 conversion; `rom.cpp` ROM image and sysex loading
- `src/tonegen/` **Tuned**: the tone generator. `operator.h` one operator's waveforms and grains, `envelope.h` its EGs, `noise.h` the unvoiced band, `channel.h` eight of them on an algorithm
- `src/engine/render.cpp` the sample loop: channels, filter voices, part buses, effects
- `src/modulators/` **Tuned**: the EG generators and LFOs the filter and effect hosts program
- `src/filters/` the voice filter: `voice_filter.h` one voice, `filter.h` 32 of them, `filter_host.*` the filter host
- `src/effects/` the effects: `programs.h` their shared state and one file a program, `effects.h` the section, `host/` the effect host
- `src/dsp/` what the filters and effects share: `word.h` the DSPs' number format, `sink.h` how a host talks to its section

Ours, with no hardware counterpart:

- `src/device/` `fsvr::Device`, host-rate resampling and state as bulk dumps
- `src/math/` the sample loop's sine, 2^x and dB: a table (the default), CORDIC, a hybrid of the two, or raw libm, chosen by `FSVR_MATH`
- `src/kernel/` the scalar code of the filters and the tone generator a few lanes at a time, compiled once per instruction set
- `src/engine/tuning.h` cost knobs, the control-rate decimation and the queue cap. Changing one must not change the output
- `src/display/` how the FS1R shows values (transcribed from K_Take's [FS1R Editor](https://synth-voice.sakura.ne.jp/fs1r_editor_english.html), checked against the Data List and the ROM) and the editor's EG plots
- `src/patchfiles/` patch files of every kind FSVR imports, read into banks of performances, voices and Fseqs, with a report of what was skipped and why
- `src/io/smf.h` Standard MIDI File reading and offline rendering; `src/selftest/` the engine self check
- `src/console/main.cpp` the test console: WinMM MIDI in and out, waveOut, offline render

**Plugin.** The plug-in layer, which never models synthesis; it moves parameter values in and out of the engine as sysex, exactly as a hardware editor would.

- `plugin/plugin.cpp` the processor: every param a sysex parameter change into `fsvr::Device`, the engine's own bulk dumps read back into the params, program change, the monitor, and the Save and Import menus' and bank manager's requests
- `plugin/library.cpp` the bank manager's data: patch files as banks (read by `src/patchfiles/`), the factory bank and the user library folder; `audio_decode.cpp` and `audio_fseq.cpp` Import Audio
- `plugin/skin/` the editor, a [Hollow](hollow/) skin: views, params (`params.json`, 3,223 of them), images (at 1x in `images/`, at 0.5x, 0.75x, 1.5x and 2x in `scales/`) and its TrueType fonts, and `data/fs1r_sysex.json`, the address and packing of every param. This copy is the source: edit it with the web editor of the Hollow project (kept beside this repository, not published), which opens it by default and saves into it
- `blender/` the skin's artwork as Blender components (the window's chrome, buttons, panels, displays, glyphs, logos, pots, faders, wheels and keys), `blender/scripts/` what builds and renders them into the skin at every scale, and `blender/textures/` the blue mottled chrome they share ([docs/editor.md](docs/editor.md), The art: Blender)
- `plugin/generated/` the bundled factory banks `fs1r_presets.syx` with its index `fs1r_presets.csv`, `fs1r_performances.syx` and `fs1r_fseqs.syx`, packed from `presets/` by `tools/make_presets_blob.py`, and `parameterDescriptions_fs1r.json`, the 893 parameters with their sysex addresses and bit layouts, which the skin's param table was built from
- `hollow/` the plug-in framework: the formats (CLAP, and through clap-wrapper VST3, AU and the standalone; VST2 and the DXi of its own), the skin runtime and software renderer, and the editor window on Windows, macOS and X11. [hollow/docs/skin-format.md](hollow/docs/skin-format.md) is the skin's contract

**Tests.** What `build.bat test`, `build.bat plugin`, `ctest` and CI run.

- `fsvr_console -selftest` (`tests/engine_selftest.cpp` away from Windows) is the engine check, the effects included: dry unity, Hall 1, the LFO law, the type path, a part on the insertion
- `tests/dsp_check.cpp` pins the filters' and effects' output bit for bit
- `tests/regress.py` renders the fixed preset list and diffs it against `regress_ref.json`: pitch, harmonic peaks, envelope, stereo width, centroid
- `tests/abcheck.py` renders the regression cases and the Fat Line demo and compares them with a baseline sample for sample, which is the tier an optimisation commit names (`docs/performance.md`, Measuring). It loads the factory banks from `plugin/generated` (`-factory` on the renderers), so it needs no ROM image
- `tests/check_plugin.cpp` drives the plug-in's processor the way a host does: factory banks, a saved session, Import SysEx, the bank browser, Import Audio, program change, panic, the monitor, Save to Bank, .fsvr presets, the right-click menus, and the Init Patch and the standalone's Load Last Patch. `ctest` runs it in a plug-in build
- `tests/check_gui.cpp` drives the editor over the processor without a window, as a mouse and a keyboard would: the top bar's menus and toggles, every page by its tab or button, the operator panel and its waveform, the browser's right-click menus, every dialog, the modal's veil, Escape and close X, the close prompt, the Options page (every scale, both keyboards and each setting) and About, then every page's dials, faders, dropdowns and toggles. `ctest` runs it as "gui"
- `tests/check_wav.py`, `check_formant.py` and `check_presets.py`, the rest of what `build.bat test` runs; `check_isa.py` keeps each lane kernel inside its instruction set and `check_win7.py` keeps every Windows binary loadable on Windows 7

**Tools.**

- `tools/fsvr_render.cpp` (`bin/fsvr_render`) renders a MIDI file or a held note offline on any platform; `render_song.py` renders a demo song to listen to
- `tools/wavetrace.cpp` (`build.bat trace`) traces a voice's operators sample by sample ([AGENTS.md](AGENTS.md), Tracing a waveform)
- `tools/perfbench.cpp` (`build.bat perf`) times the engine on fixed scenarios through `fsvr::Device`, pinned to one core, and hashes its 16-bit output; `tools/dsp_bench.cpp` (`build.bat bench`) times the filters and effects
- `tools/patch.py` builds FS1R voices, performances, bulk dumps and Standard MIDI Files from Python, and `make_templates.py` writes the Templates bank with it; `pitch_track.py` prints a render's dominant frequency over time
- `tools/make_presets_blob.py` packs `presets/` into `plugin/generated/`, `extract_bonus.py` writes `presets/bonus/` from the archived Bonus Tool Pack, and `contributors.py` writes the About box's contributor line
- `tools/extract_tables.py`, `extract_presets.py`, `extract_demo.py` and `extract_vop3_rom.py` write the conversion tables, the factory banks, the demo songs and the hosts' tables from a ROM image you dump yourself

**Data.**

- `presets/` the factory voices, performances and Fseqs as `.syx` with `index.csv`, plus the DX7 bank and the Bonus Tool Pack
- `demosongs/` the FS1R's fifteen demo songs as Standard MIDI Files, one a song and `all.mid` back to back, which `perfbench`, `abcheck` and `render_song.py` play

**Docs.**

- [docs/Differences.md](docs/Differences.md) the short list of places FSVR knowingly behaves differently from the FS1R, and why
- [docs/plugin_guide.md](docs/plugin_guide.md) the plugin user guide, and [docs/editor.md](docs/editor.md) why the editor is the way it is: which of the FS1R's limits it drops, the bank manager and what the processor answers for the GUI
- [docs/performance.md](docs/performance.md) what the engine costs, and how a change to that cost is measured
- [hollow/docs/](hollow/docs/) the plug-in framework: `framework.md` how a product is built on it, `skin-format.md` the skin's contract

## License

FSVR is free software under the GNU General Public License v3.0, in [LICENSE](LICENSE), with the provenance of each part in [NOTICE.md](NOTICE.md).
