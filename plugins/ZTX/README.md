# ZTX — ORGANIC GLITCH LAB

An experimental sound-design instrument and generative groovebox.
Five synthesis engines, a 4-track generative sequencer, an 8-slot modulation matrix,
twelve chainable destruction effects, an exploration ("Evolve") mode and 42 factory presets.

Formats: **AU** (macOS), **VST3**, **Standalone**. Framework: JUCE 8.0.4 / C++17 / CMake.
No dependencies beyond JUCE.

---

## 1. Build & install on macOS (Apple Silicon)

Requirements: macOS 11+, Xcode command-line tools (`xcode-select --install`), CMake 3.22+ (`brew install cmake`), git.

```bash
cd ZTX
./scripts/build_mac.sh
```

The script:
1. downloads JUCE 8.0.4 into `./JUCE` (only the first time),
2. builds AU + VST3 + Standalone + the test runner natively for `arm64`
   (`ZTX_UNIVERSAL=1 ./scripts/build_mac.sh` builds arm64 + x86_64),
3. ad-hoc code-signs the bundles,
4. installs them to `~/Library/Audio/Plug-Ins/Components` and `~/Library/Audio/Plug-Ins/VST3`,
   and copies the standalone app to `./dist`,
5. runs the offline DSP tests and **Apple's `auval`** (`auval -v aumu Zogl Ztxl`) and stops if validation fails.

Manual build instead of the script:

```bash
git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git JUCE
cmake -B build-mac -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-mac --config Release -j 8
# products: build-mac/ZTX_artefacts/Release/{AU,VST3,Standalone}
```

**Building in the cloud instead:** push this folder to a GitHub repository. The included workflow
`.github/workflows/build-macos.yml` builds on an Apple Silicon runner (`macos-14`), runs the tests and `auval`,
and uploads a zip with the `.component`, `.vst3` and `.app`. If you install a downloaded build, remove the quarantine flag first:

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"ZTX Organic Glitch Lab.component"
```

## 2. Opening it in Logic Pro

1. Quit Logic, install with the script (or copy the `.component` to `~/Library/Audio/Plug-Ins/Components`).
2. Open Logic. It scans new Audio Units at launch. If ZTX doesn't show up:
   **Logic Pro → Settings → Plug-in Manager**, find *ZTX Lab → ZTX Organic Glitch Lab*,
   click **Reset & Rescan Selection**.
3. Create a **Software Instrument** track → Instrument slot → **AU Instruments → ZTX Lab → ZTX Organic Glitch Lab → Stereo**.
4. Press play in Logic: the sequencer follows Logic's tempo and transport. Play MIDI to play the LAB engine on top.
5. To *capture audio from another track* into the granular engine: ZTX declares an optional stereo input bus; if Logic
   shows a **Side Chain** menu in the plug-in header, pick a source there, then press **CAPTURE IN** on the Granular page.
   **RESAMPLE 4s** (recording ZTX's own voices) always works.

Presets: ZTX has its own browser (PRESETS tab). Logic's own *Save as…* also works — it stores the full state, including
all four pattern variations.

## 3. Workflow (the 8-step loop)

1. **Open** → pick a preset or press INIT PATCH.
2. **LAB** → choose an engine (left cards).
3. **Generate a sound** → RND SOUND (top bar), or tap the TOUCH PAD / keyboard.
4. **Shape** → engine knobs; drag the TOUCH PAD (X = colour, Y = decay).
5. **Rhythm** → SEQUENCER → set Density/Complexity/Chaos/Groove/Repetition → **GENERATE**; **MUTATE** to vary; **FREEZE** to keep.
6. **Modulate** → MODULATION → pick a source and a destination in a matrix slot. Orange arcs on every knob show live modulation.
7. **Destroy** → DESTRUCTION → enable effects, choose the chain order.
8. **Save** → PRESETS → type a name → SAVE USER PRESET.

Turn on **EVOLVE** (top bar) for exploration mode: every sound parameter drifts in a bounded random walk around its
stored value (Depth, Speed), and the pattern mutates gently every N bars — the identity stays recognisable.

## 4. Features (implemented and tested)

### Engines (each with its own parameters)
| Engine | What it is | Parameters |
|---|---|---|
| **A · Rubber Lab** | Phase-modulated elastic core with pitch "stretch" (exponential pitch drop) and a damped bounce wobble, struck through a 4-mode modal body whose ratios morph rubber → wood → metal; stick-slip friction impulses and scrape noise; asymmetric drive | Elasticity, Tension, Damping, Resonance, Impact, Friction, Pitch, Decay, Material, Distortion |
| **B · Micro Glitch** | One trigger opens a burst window; a tempo-locked micro grid (1/8 … 1/128) fires clicks, crackles, blips, digital noise and *slices of its own past output* with probability, jitter and micro-stutter repeats; per-key event palette; bit + sample-rate reduction | Density, Grain size, Randomness, Repeat prob, Bits, Rate reduce, Buffer mangle, Event prob, Timing var, Pitch, Burst length, Tone |
| **C · Granular Organism** | 40-grain engine. Load WAV/AIFF/FLAC/MP3 (≤30 s, drag & drop), **resample ZTX's own output** or **capture the input/side-chain** (4 s), or use the built-in "organic debris" buffer | Position, Scan, Spray, Size, Density, Shape (perc→Hann→flat), Pitch, Pitch random, Reverse prob, Spread, **Freeze** |
| **D · FM / Metallic** | 4 operators, 8 routings (with diagram), op4 feedback, inharmonic ratio stretch, per-operator random drift, pitch-drop envelope, sine-fold drive | Algorithm, 4 ratios, 3 op levels, Amount, Feedback, Decay, Mod decay, Pitch env + time, Inharmonic, Instability, Drive, Pitch |
| **E · Organic Percussion** | Swept sine body + transient click + 4-mode material shell (skin/wood/hollow/metal) + band-passed noise + granular rattle | Pitch, Decay, Transient, Resonance, Noise, Body, Material, Sweep, Rattle, Tone |

### Generative sequencer
- 4 independent tracks; each picks any engine, note, level, pan, gate, **per-track decay/colour macro** offsets.
- 16 / 32-step modes; **per-track length** (polymeter) and **per-track division** 1/4…1/32, triplets, dotted (polyrhythm).
- Per step: on/off, velocity, **probability**, **ratchet** ×1–4, **micro-timing**, **pitch offset**. Edit modes GATE / VELOCITY / PROB / RATCHET / PITCH / MICRO, right-click menu, shift/rotate.
- **Swing**, **Humanize** (timing + velocity), global **Probability** (modulatable).
- **Euclidean** per track (hits, rotation, E button).
- **GENERATE** using Density, Complexity, Chaos, Groove and Repetition (role-aware: pulse / counter / micro / texture tracks; anchors the downbeat; repetition copies motifs).
- **MUTATE** (a few small edits per track; protects strong beats and the downbeat), **FREEZE**, **UNDO** (32 levels), **CLEAR**, per-track **LOCK**.
- 4 pattern **variations** A–D (shift-click to copy).
- A 16-step **Step Lane** that is a modulation source.
- **MIDI / host sync**: follows the host tempo and transport (ppq-locked, so loops and seeks never double-trigger);
  internal clock + RUN when the host is stopped; **MIDI Hold** (plays while a key is held) and **MIDI Hold + Transpose**.

### Modulation
- 4 LFOs: Sine, Triangle, Saw, Square, Smooth random, Sample & Hold, Drunk walk, Spike; free Hz or tempo-synced (phase-locked to host ppq).
- Per-voice **Mod Envelope** (ADSR) + the **Amp ADSR** (Gated mode).
- **Random** source (rate + smooth → stepped S&H to gliding).
- **Envelope follower** (of the voice mix).
- **Generative** source: Lorenz attractor blended with a quasi-periodic orbit ("Chaos").
- **Step Lane**, **Velocity**, **Key**, **Mod Wheel**.
- **8 matrix slots**, any source → any of **86 destinations** (every engine parameter, sequencer probability, filter, fold, saturation, crusher, shifter, ring, comb, delay, reverb, width, spectral, glitch buffer).
- Visual: orange live-modulation arcs on every knob, scrolling traces per source, live meter per slot.

### Destruction (12 modules, 4 selectable chain orders)
Multimode filter (LP/BP/HP/Notch/Acid 4-pole/**Vowel** formant), wavefolder with bias, saturation (tanh/tube/rectify),
bitcrusher, Bode **frequency shifter** with barber-pole feedback, ring modulator (sine→square), comb resonator (±feedback),
saturated **delay** (sync/free, tone, warble, ping-pong, self-oscillation above 1.0), dark modulated 8-line **FDN reverb**,
stereo width + Haas, **spectral freeze/blur** (STFT 2048), tempo-synced **glitch buffer** (stutter, reverse, pitch, gate).
A safety peak limiter sits at the output.

### Randomisation
RND **SOUND / RHYTHM / MOD / FX / ALL**, an **Amount** knob (how far it jumps), and a **LOCK** next to each section.
Randomisation is biased to musical ranges (useful FM ratios, sane decays, capped effect mixes, feedback limits).

### UI
Five sections — **LAB, SEQUENCER, MODULATION, DESTRUCTION, PRESETS** — dark laboratory look (near-black, muted acid green,
dirty silver, off-white, occasional muted orange), organic "pod" panels, glitched typography, phosphor oscilloscope,
animated sequencer playheads, FM algorithm diagram, sample view with grain window, touch pad, on-screen keyboard,
tooltips on every control, **resizable** (75 %–200 %, fixed aspect).

### Presets — 42 factory presets in 8 categories
Rubber & Elastic (6), Micro Percussion (5), Dark Textures (5), Organic Glitches (5), Metallic Objects (5),
Experimental Bass (5), Psychedelic Atmospheres (5), Generative Rhythms (6). Each preset ships 4 related pattern variations
(A = designed, B–D = progressively mutated). User presets are saved as `.ztxpreset` files in
`~/Library/Application Support/ZTX Organic Glitch Lab/Presets`.

## 5. How it was tested

Done in a Linux build environment (the only one available while writing it):
- **Compiles without errors** (VST3, Standalone, test runner) with JUCE 8.0.4 and JUCE's recommended warning flags (a few style warnings remain).
- **pluginval strictness 10 (maximum): SUCCESS** on the VST3 — open/close, editor, audio processing at many sample rates and
  block sizes, state save/restore, automation, **parameter thread safety**, bus layouts, parameter fuzzing.
  (Level 10 initially crashed in the thread-safety fuzz; the cause — a NaN from the chaotic attractor reaching a filter
  table index — was fixed and hardened.)
- **Offline test runner** (`ZTXTests`, also run by the mac script/workflow): renders every engine and all 42 presets
  (checks: finite output, audible, under the limiter ceiling), a stress test with every effect on and 24 voices
  (~17× faster than real time on one x86 core), 30 random "RANDOMIZE ALL" patches (all stable), LOCK behaviour,
  generator density control, mutate-keeps-identity, euclid correctness, full state round-trip of all 200 parameters +
  patterns, and spectral resynthesis gain (≈ 0 dB). Writes a WAV for every engine and preset.
- Standalone launched and ran without crashing; UI screenshots of every page were rendered and checked.

**Not yet verified by me:** the AU build, `auval`, and Logic Pro on Apple Silicon — there was no Mac available.
`scripts/build_mac.sh` and the GitHub workflow run `auval` automatically; please run one of them and check the result.

## 6. Known limitations

- AU / Logic: see above — built from the same code as the validated VST3, but not yet run on a Mac.
- Audio recorded with RESAMPLE / CAPTURE IN is not stored inside presets or the Logic project (loaded files are stored by path).
- Each engine's parameters are shared by every sequencer track that uses that engine; tracks differ by note, level, pan,
  gate and their decay/colour macros. (Two Perc tracks can't have two completely different materials.)
- Parameter modulation runs at control rate (every 32 samples ≈ 0.7 ms); glitch-buffer slice boundaries are evaluated at the same rate.
- Spectral freeze/blur delays its wet signal by 2048 samples (not reported as plugin latency — it's a texture effect).
- LFO shapes are fixed (8 shapes, incl. Drunk and Spike); no hand-drawn custom shapes yet.
- Sequencer pattern data (steps, track settings) is saved with the state but is not host-automatable.
- Granular is always gated by note length (sequencer gate or MIDI note length).
- Factory-preset loudness was balanced with an offline RMS measurement, not by ear.

## 7. Code map (modular — add an engine or effect in one place)

```
Source/
  Params.h/.cpp          single X-macro table → APVTS, mod destinations, randomiser (200 params)
  PluginProcessor.*      voices, clocking, MIDI, capture, state, lock-free UI feedback
  Voice.h                one voice = 5 engine instances + amp/mod ADSR + per-voice modulation
  engines/               Engine.h interface + RubberLab, MicroGlitch, Granular, FMLab, OrganicPerc
  seq/                   Pattern (data, generate, mutate, euclid, serialisation), SeqPlayer (ppq scheduler)
  mod/ModEngine.h        LFOs, random, follower, attractor, step lane, matrix, Evolve walk
  fx/Effects.*           12 processors + chain orders
  Randomizer.*           musical randomisation with locks
  presets/Presets.*      42 factory presets (builder DSL) + user preset files
  ui/                    look-and-feel, widgets, LAB / SEQUENCER / MODULATION / DESTRUCTION / PRESETS pages
Tests/OfflineTests.cpp   headless test runner + WAV demos + UI snapshots (--snapshots dir)
```

To add an engine: implement `EngineVoice` (prepare/start/render/finished), add its parameters to the X-macro
table, register it in `Voice` and `engineNames()`. To add an effect: add its parameters, a `FxId`, a case in
`FxChain::runModule`, and a module entry in `ui/FxPage.cpp`.
