#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// =====================================================================================
//  ZTX parameter table.  One X-macro drives the enum, the APVTS layout, the mod
//  destination list and the randomiser - add a parameter here and it appears everywhere.
//  F(id, name, min, max, default, skew, section, tooltip)
//  C(id, name, "choiceA|choiceB", defaultIndex, section, tooltip)
//  B(id, name, default, section, tooltip)
// =====================================================================================
namespace ztx
{
enum class Sec { Global, Rubber, Glitch, Gran, FM, Perc, Seq, Mod, FX, Evolve };

#define ZTX_ENGINES  "Rubber Lab|Micro Glitch|Granular Organism|FM Metallic|Organic Perc"
#define ZTX_DIVS     "4 Bars|2 Bars|1 Bar|1/2|1/4|1/8|1/16|1/32|1/4T|1/8T|1/16T|1/8D|1/16D"
#define ZTX_SHAPES   "Sine|Triangle|Saw|Square|Smooth Rnd|Sample+Hold|Drunk|Spike"
#define ZTX_MODSRC   "None|LFO 1|LFO 2|LFO 3|LFO 4|Mod Env|Velocity|Key|Random|Step Lane|Env Follow|Generative|Mod Wheel"

#define ZTX_LFO(F, C, B, n, defRate, defShape) \
    F(lfo##n##Rate,  "LFO" #n " Rate", 0.01f, 30.f, defRate, 0.3f, Sec::Mod, "LFO rate in Hz (free mode)") \
    B(lfo##n##Sync,  "LFO" #n " Sync", false, Sec::Mod, "Lock LFO to host tempo") \
    C(lfo##n##Div,   "LFO" #n " Division", ZTX_DIVS, 4, Sec::Mod, "Tempo-synced cycle length") \
    C(lfo##n##Shape, "LFO" #n " Shape", ZTX_SHAPES, defShape, Sec::Mod, "LFO waveform")

#define ZTX_SLOT(F, C, B, n) \
    C(m##n##Src, "Mod" #n " Source", ZTX_MODSRC, 0, Sec::Mod, "Modulation source") \
    C(m##n##Dst, "Mod" #n " Dest", "@DEST", 0, Sec::Mod, "Modulation destination") \
    F(m##n##Amt, "Mod" #n " Amount", -1.f, 1.f, 0.f, 1.f, Sec::Mod, "Bipolar modulation depth")

#define ZTX_PARAMS(F, C, B) \
    F(masterVol, "Master", -48.f, 6.f, -6.f, 1.f, Sec::Global, "Output level in dB") \
    C(labEngine, "Engine", ZTX_ENGINES, 0, Sec::Global, "Engine played by MIDI / the LAB pad") \
    F(labPitch, "Global Pitch", -24.f, 24.f, 0.f, 1.f, Sec::Global, "Transposes every engine (semitones)") \
    C(gateMode, "Gate Mode", "One-shot|Gated", 0, Sec::Global, "One-shot lets sounds ring; Gated follows note length (ADSR)") \
    F(ampA, "Amp Attack", 0.0005f, 2.f, 0.001f, 0.3f, Sec::Global, "Amp envelope attack (s)") \
    F(ampD, "Amp Decay", 0.005f, 4.f, 0.4f, 0.3f, Sec::Global, "Amp envelope decay (s)") \
    F(ampS, "Amp Sustain", 0.f, 1.f, 1.f, 1.f, Sec::Global, "Amp envelope sustain (Gated mode)") \
    F(ampR, "Amp Release", 0.005f, 6.f, 0.25f, 0.3f, Sec::Global, "Amp envelope release (s)") \
    F(velSens, "Velocity Sens", 0.f, 1.f, 0.7f, 1.f, Sec::Global, "How much velocity changes level") \
    /* ---------------- ENGINE A : RUBBER LAB ---------------- */ \
    F(rbElastic, "Elasticity", 0.f, 1.f, 0.55f, 1.f, Sec::Rubber, "Pitch stretch + bounce wobble of the elastic core") \
    F(rbTension, "Tension", 0.f, 1.f, 0.45f, 1.f, Sec::Rubber, "Stiffness: faster snap, faster wobble, more self-FM") \
    F(rbDamping, "Damping", 0.f, 1.f, 0.4f, 1.f, Sec::Rubber, "Kills high modes and the bounce") \
    F(rbResonance, "Resonance", 0.f, 1.f, 0.5f, 1.f, Sec::Rubber, "Level and ring time of the modal body") \
    F(rbImpact, "Impact", 0.f, 1.f, 0.6f, 1.f, Sec::Rubber, "Strike hardness: click + noise exciter") \
    F(rbFriction, "Friction", 0.f, 1.f, 0.1f, 1.f, Sec::Rubber, "Stick-slip scraping while it sounds") \
    F(rbPitch, "Rubber Pitch", -24.f, 24.f, 0.f, 1.f, Sec::Rubber, "Engine transpose (semitones)") \
    F(rbDecay, "Rubber Decay", 0.02f, 6.f, 0.5f, 0.35f, Sec::Rubber, "Ring time (s)") \
    F(rbMaterial, "Material", 0.f, 1.f, 0.3f, 1.f, Sec::Rubber, "Rubber -> wood -> metal mode ratios") \
    F(rbDrive, "Rubber Drive", 0.f, 1.f, 0.2f, 1.f, Sec::Rubber, "Asymmetric nonlinear distortion") \
    /* ---------------- ENGINE B : MICRO GLITCH ---------------- */ \
    F(glDensity, "Glitch Density", 0.f, 1.f, 0.5f, 1.f, Sec::Glitch, "Event grid resolution (1/8 .. 1/128)") \
    F(glGrain, "Grain Size", 0.3f, 80.f, 6.f, 0.4f, Sec::Glitch, "Length of each micro-event (ms)") \
    F(glRandom, "Randomness", 0.f, 1.f, 0.4f, 1.f, Sec::Glitch, "Variation of event type, pitch and level") \
    F(glRepeat, "Repeat Prob", 0.f, 1.f, 0.3f, 1.f, Sec::Glitch, "Chance an event micro-stutters") \
    F(glBits, "Bit Depth", 2.f, 16.f, 16.f, 1.f, Sec::Glitch, "Bit reduction") \
    F(glRate, "Rate Reduce", 0.f, 1.f, 0.f, 1.f, Sec::Glitch, "Sample-rate reduction") \
    F(glBuffer, "Buffer Mangle", 0.f, 1.f, 0.3f, 1.f, Sec::Glitch, "Replays/reverses slices of its own output") \
    F(glProb, "Event Prob", 0.f, 1.f, 0.7f, 1.f, Sec::Glitch, "Chance each grid slot fires") \
    F(glTiming, "Timing Var", 0.f, 1.f, 0.2f, 1.f, Sec::Glitch, "Off-grid jitter of events") \
    F(glPitch, "Glitch Pitch", -24.f, 24.f, 0.f, 1.f, Sec::Glitch, "Engine transpose (semitones)") \
    F(glDecay, "Burst Length", 0.03f, 4.f, 0.6f, 0.4f, Sec::Glitch, "How long one trigger keeps spitting debris (s)") \
    F(glTone, "Glitch Tone", 0.f, 1.f, 0.5f, 1.f, Sec::Glitch, "Dark <-> brittle") \
    /* ---------------- ENGINE C : GRANULAR ORGANISM ---------------- */ \
    F(grSize, "Grain Size", 5.f, 1000.f, 80.f, 0.35f, Sec::Gran, "Grain length (ms)") \
    F(grDensity, "Grain Density", 1.f, 120.f, 25.f, 0.4f, Sec::Gran, "Grains per second") \
    F(grPos, "Position", 0.f, 1.f, 0.3f, 1.f, Sec::Gran, "Read position in the buffer") \
    F(grSpray, "Spray", 0.f, 1.f, 0.1f, 1.f, Sec::Gran, "Random position scatter") \
    F(grScan, "Scan", -1.f, 1.f, 0.f, 1.f, Sec::Gran, "Position drift speed (ignored when frozen)") \
    F(grPitch, "Gran Pitch", -24.f, 24.f, 0.f, 1.f, Sec::Gran, "Engine transpose (semitones)") \
    F(grPitchRnd, "Pitch Random", 0.f, 1.f, 0.f, 1.f, Sec::Gran, "Per-grain pitch scatter (up to +-12 st)") \
    F(grReverse, "Reverse Prob", 0.f, 1.f, 0.f, 1.f, Sec::Gran, "Chance a grain plays backwards") \
    F(grSpread, "Stereo Spread", 0.f, 1.f, 0.5f, 1.f, Sec::Gran, "Grain panning scatter") \
    F(grEnv, "Grain Shape", 0.f, 1.f, 0.5f, 1.f, Sec::Gran, "Percussive -> Hann -> flat window") \
    B(grFreeze, "Freeze", false, Sec::Gran, "Freeze position: texture holds") \
    /* ---------------- ENGINE D : FM / METALLIC ---------------- */ \
    C(fmAlgo, "FM Algorithm", "Stack|Twin Mod|Branch|Dual Pair|Fan Out|Sub+Stack|Additive|Metal Fan", 0, Sec::FM, "Operator routing") \
    F(fmRatio1, "Ratio 1", 0.25f, 16.f, 1.f, 0.4f, Sec::FM, "Operator 1 (carrier) frequency ratio") \
    F(fmRatio2, "Ratio 2", 0.25f, 16.f, 2.f, 0.4f, Sec::FM, "Operator 2 ratio") \
    F(fmRatio3, "Ratio 3", 0.25f, 16.f, 3.51f, 0.4f, Sec::FM, "Operator 3 ratio") \
    F(fmRatio4, "Ratio 4", 0.25f, 16.f, 1.41f, 0.4f, Sec::FM, "Operator 4 ratio (has feedback)") \
    F(fmIdx2, "Op2 Level", 0.f, 1.f, 0.4f, 1.f, Sec::FM, "Operator 2 output / mod index") \
    F(fmIdx3, "Op3 Level", 0.f, 1.f, 0.3f, 1.f, Sec::FM, "Operator 3 output / mod index") \
    F(fmIdx4, "Op4 Level", 0.f, 1.f, 0.25f, 1.f, Sec::FM, "Operator 4 output / mod index") \
    F(fmAmount, "FM Amount", 0.f, 1.f, 0.6f, 1.f, Sec::FM, "Master modulation index") \
    F(fmFeedback, "Feedback", 0.f, 1.f, 0.1f, 1.f, Sec::FM, "Op4 self-feedback (noise/metal)") \
    F(fmDecay, "FM Decay", 0.01f, 6.f, 0.6f, 0.35f, Sec::FM, "Carrier decay (s)") \
    F(fmModDecay, "Mod Decay", 0.05f, 2.f, 0.5f, 0.5f, Sec::FM, "Modulator decay relative to carrier") \
    F(fmPitchEnv, "Pitch Env", 0.f, 48.f, 0.f, 0.5f, Sec::FM, "Initial pitch drop (semitones)") \
    F(fmPitchDec, "Pitch Env Time", 0.002f, 1.f, 0.05f, 0.4f, Sec::FM, "Pitch drop time (s)") \
    F(fmInharm, "Inharmonic", 0.f, 1.f, 0.f, 1.f, Sec::FM, "Stretches ratios away from harmonic") \
    F(fmInstab, "Instability", 0.f, 1.f, 0.f, 1.f, Sec::FM, "Random pitch drift per operator") \
    F(fmDrive, "FM Drive", 0.f, 1.f, 0.f, 1.f, Sec::FM, "Nonlinear fold/saturation") \
    F(fmPitch, "FM Pitch", -24.f, 24.f, 0.f, 1.f, Sec::FM, "Engine transpose (semitones)") \
    /* ---------------- ENGINE E : ORGANIC PERCUSSION ---------------- */ \
    F(pcPitch, "Perc Pitch", -36.f, 36.f, 0.f, 1.f, Sec::Perc, "Engine transpose (semitones)") \
    F(pcDecay, "Perc Decay", 0.01f, 3.f, 0.35f, 0.4f, Sec::Perc, "Body decay (s)") \
    F(pcTransient, "Transient", 0.f, 1.f, 0.5f, 1.f, Sec::Perc, "Click / attack amount") \
    F(pcResonance, "Perc Resonance", 0.f, 1.f, 0.4f, 1.f, Sec::Perc, "Modal ring") \
    F(pcNoise, "Noise", 0.f, 1.f, 0.2f, 1.f, Sec::Perc, "Filtered noise layer") \
    F(pcBody, "Body", 0.f, 1.f, 0.7f, 1.f, Sec::Perc, "Pitched body (sine with sweep)") \
    F(pcMaterial, "Perc Material", 0.f, 1.f, 0.f, 1.f, Sec::Perc, "Skin -> wood -> hollow -> metal") \
    F(pcSweep, "Sweep", 0.f, 1.f, 0.3f, 1.f, Sec::Perc, "Pitch drop of the body") \
    F(pcRattle, "Rattle", 0.f, 1.f, 0.f, 1.f, Sec::Perc, "Shaker-like granular rattle") \
    F(pcTone, "Perc Tone", 0.f, 1.f, 0.5f, 1.f, Sec::Perc, "Noise colour dark <-> bright") \
    /* ---------------- SEQUENCER ---------------- */ \
    F(seqDensity, "Density", 0.f, 1.f, 0.5f, 1.f, Sec::Seq, "GENERATE: how many hits") \
    F(seqComplexity, "Complexity", 0.f, 1.f, 0.4f, 1.f, Sec::Seq, "GENERATE: layered euclids, ratchets, odd lengths") \
    F(seqChaos, "Chaos", 0.f, 1.f, 0.2f, 1.f, Sec::Seq, "GENERATE/MUTATE: random flips, probabilities, micro-timing") \
    F(seqGroove, "Groove", 0.f, 1.f, 0.4f, 1.f, Sec::Seq, "GENERATE: syncopation and accent placement") \
    F(seqRepeat, "Repetition", 0.f, 1.f, 0.6f, 1.f, Sec::Seq, "GENERATE: how strongly motifs repeat") \
    F(seqSwing, "Swing", 0.f, 0.75f, 0.f, 1.f, Sec::Seq, "Delays every second step") \
    F(seqHuman, "Humanize", 0.f, 1.f, 0.f, 1.f, Sec::Seq, "Random timing and velocity drift") \
    F(seqProb, "Seq Probability", 0.f, 1.f, 1.f, 1.f, Sec::Seq, "Scales every step probability (modulatable)") \
    B(seqRun, "Seq Run", false, Sec::Seq, "Run the sequencer from its own clock when the host is stopped") \
    F(seqBpm, "Internal BPM", 40.f, 240.f, 120.f, 1.f, Sec::Seq, "Tempo when the host gives none") \
    C(seqMidi, "MIDI Mode", "Free Run|MIDI Hold|MIDI Hold+Transpose", 0, Sec::Seq, "Free: follows transport. Hold: plays only while a MIDI note is held") \
    C(seqSteps, "Steps", "16|32", 0, Sec::Seq, "Max pattern length") \
    /* ---------------- MODULATION ---------------- */ \
    ZTX_LFO(F, C, B, 1, 0.5f, 0) ZTX_LFO(F, C, B, 2, 2.f, 1) ZTX_LFO(F, C, B, 3, 0.13f, 6) ZTX_LFO(F, C, B, 4, 6.f, 5) \
    F(meA, "ModEnv Attack", 0.001f, 4.f, 0.005f, 0.3f, Sec::Mod, "Mod envelope attack (s)") \
    F(meD, "ModEnv Decay", 0.005f, 6.f, 0.3f, 0.3f, Sec::Mod, "Mod envelope decay (s)") \
    F(meS, "ModEnv Sustain", 0.f, 1.f, 0.f, 1.f, Sec::Mod, "Mod envelope sustain") \
    F(meR, "ModEnv Release", 0.005f, 8.f, 0.3f, 0.3f, Sec::Mod, "Mod envelope release (s)") \
    F(rndRate, "Random Rate", 0.05f, 30.f, 2.f, 0.3f, Sec::Mod, "Random source speed (Hz)") \
    F(rndSmooth, "Random Smooth", 0.f, 1.f, 0.5f, 1.f, Sec::Mod, "0 = stepped, 1 = gliding") \
    F(efAtt, "Follower Attack", 0.001f, 0.5f, 0.01f, 0.4f, Sec::Mod, "Envelope follower attack (s)") \
    F(efRel, "Follower Release", 0.01f, 2.f, 0.2f, 0.4f, Sec::Mod, "Envelope follower release (s)") \
    F(genRate, "Gen Rate", 0.f, 1.f, 0.3f, 1.f, Sec::Mod, "Speed of the chaotic attractor") \
    F(genChaos, "Gen Chaos", 0.f, 1.f, 0.5f, 1.f, Sec::Mod, "Periodic orbit -> strange attractor") \
    ZTX_SLOT(F, C, B, 1) ZTX_SLOT(F, C, B, 2) ZTX_SLOT(F, C, B, 3) ZTX_SLOT(F, C, B, 4) ZTX_SLOT(F, C, B, 5) ZTX_SLOT(F, C, B, 6) ZTX_SLOT(F, C, B, 7) ZTX_SLOT(F, C, B, 8) \
    /* ---------------- EVOLVE / RANDOMISE ---------------- */ \
    B(evolveOn, "Evolve", false, Sec::Evolve, "Exploration mode: sound drifts around its identity") \
    F(evolveAmt, "Evolve Depth", 0.f, 1.f, 0.3f, 1.f, Sec::Evolve, "How far the sound may wander") \
    F(evolveRate, "Evolve Speed", 0.f, 1.f, 0.3f, 1.f, Sec::Evolve, "How fast it wanders") \
    C(evolveBars, "Evolve Mutate", "Off|1 Bar|2 Bars|4 Bars|8 Bars|16 Bars", 3, Sec::Evolve, "Gently mutate the pattern every N bars while evolving") \
    F(rndAmount, "Random Amount", 0.f, 1.f, 0.7f, 1.f, Sec::Evolve, "How far RANDOMIZE jumps from the current sound") \
    B(lockSound, "Lock Sound", false, Sec::Evolve, "Protect engines from RANDOMIZE") \
    B(lockRhythm, "Lock Rhythm", false, Sec::Evolve, "Protect the pattern from RANDOMIZE") \
    B(lockMod, "Lock Mod", false, Sec::Evolve, "Protect modulation from RANDOMIZE") \
    B(lockFx, "Lock FX", false, Sec::Evolve, "Protect effects from RANDOMIZE") \
    /* ---------------- DESTRUCTION ---------------- */ \
    C(fxOrder, "FX Order", "Classic|Destroy Late|Space First|Glitch First", 0, Sec::FX, "Effect chain order") \
    B(fltOn, "Filter On", true, Sec::FX, "Experimental multimode filter") \
    C(fltMode, "Filter Mode", "Low Pass|Band Pass|High Pass|Notch|Acid 4-Pole|Vowel", 0, Sec::FX, "Filter type") \
    F(fltCutoff, "Cutoff", 20.f, 20000.f, 14000.f, 0.25f, Sec::FX, "Cutoff / formant position (Hz)") \
    F(fltReso, "Filter Reso", 0.f, 1.f, 0.15f, 1.f, Sec::FX, "Resonance") \
    F(fltDrive, "Filter Drive", 0.f, 1.f, 0.f, 1.f, Sec::FX, "Pre-filter drive") \
    B(foldOn, "Fold On", false, Sec::FX, "Wavefolder") \
    F(foldAmt, "Fold", 0.f, 1.f, 0.3f, 1.f, Sec::FX, "Folding gain") \
    F(foldBias, "Fold Bias", -1.f, 1.f, 0.f, 1.f, Sec::FX, "Asymmetry (even harmonics)") \
    F(foldMix, "Fold Mix", 0.f, 1.f, 1.f, 1.f, Sec::FX, "Dry/wet") \
    B(satOn, "Sat On", false, Sec::FX, "Saturation") \
    C(satType, "Sat Type", "Tanh|Tube|Rectify", 0, Sec::FX, "Saturation curve") \
    F(satDrive, "Sat Drive", 0.f, 1.f, 0.3f, 1.f, Sec::FX, "Drive") \
    F(satMix, "Sat Mix", 0.f, 1.f, 1.f, 1.f, Sec::FX, "Dry/wet") \
    B(crOn, "Crush On", false, Sec::FX, "Bitcrusher") \
    F(crBits, "Crush Bits", 1.f, 16.f, 8.f, 1.f, Sec::FX, "Bit depth") \
    F(crRate, "Crush Rate", 1.f, 64.f, 4.f, 0.4f, Sec::FX, "Downsample factor") \
    F(crMix, "Crush Mix", 0.f, 1.f, 1.f, 1.f, Sec::FX, "Dry/wet") \
    B(fsOn, "Shift On", false, Sec::FX, "Frequency shifter (Bode style)") \
    F(fsShift, "Shift", -1000.f, 1000.f, 30.f, 1.f, Sec::FX, "Shift (Hz)") \
    F(fsFb, "Shift Feedback", 0.f, 0.95f, 0.3f, 1.f, Sec::FX, "Barber-pole feedback") \
    F(fsMix, "Shift Mix", 0.f, 1.f, 0.5f, 1.f, Sec::FX, "Dry/wet") \
    B(rmOn, "Ring On", false, Sec::FX, "Ring modulator") \
    F(rmFreq, "Ring Freq", 0.5f, 5000.f, 200.f, 0.3f, Sec::FX, "Carrier frequency (Hz)") \
    F(rmShape, "Ring Shape", 0.f, 1.f, 0.f, 1.f, Sec::FX, "Sine -> square carrier") \
    F(rmMix, "Ring Mix", 0.f, 1.f, 0.5f, 1.f, Sec::FX, "Dry/wet") \
    B(cbOn, "Comb On", false, Sec::FX, "Comb resonator") \
    F(cbFreq, "Comb Freq", 20.f, 4000.f, 220.f, 0.3f, Sec::FX, "Resonant frequency (Hz)") \
    F(cbFb, "Comb Feedback", -0.98f, 0.98f, 0.7f, 1.f, Sec::FX, "Negative = hollow/odd harmonics") \
    F(cbDamp, "Comb Damp", 0.f, 1.f, 0.3f, 1.f, Sec::FX, "Damping in the loop") \
    F(cbMix, "Comb Mix", 0.f, 1.f, 0.5f, 1.f, Sec::FX, "Dry/wet") \
    B(dlOn, "Delay On", false, Sec::FX, "Feedback delay") \
    B(dlSync, "Delay Sync", true, Sec::FX, "Tempo-synced time") \
    C(dlDiv, "Delay Division", ZTX_DIVS, 11, Sec::FX, "Synced time") \
    F(dlTime, "Delay Time", 1.f, 2000.f, 300.f, 0.4f, Sec::FX, "Free time (ms)") \
    F(dlFb, "Delay Feedback", 0.f, 1.1f, 0.45f, 1.f, Sec::FX, "Feedback; above 1 = saturated self-oscillation") \
    F(dlTone, "Delay Tone", 0.f, 1.f, 0.5f, 1.f, Sec::FX, "Dark <-> thin repeats") \
    F(dlWarble, "Delay Warble", 0.f, 1.f, 0.1f, 1.f, Sec::FX, "Tape-like time modulation") \
    B(dlPing, "Ping Pong", false, Sec::FX, "Cross-feed repeats") \
    F(dlMix, "Delay Mix", 0.f, 1.f, 0.3f, 1.f, Sec::FX, "Dry/wet") \
    B(rvOn, "Reverb On", false, Sec::FX, "Dark modulated FDN reverb") \
    F(rvSize, "Reverb Size", 0.f, 1.f, 0.5f, 1.f, Sec::FX, "Room size") \
    F(rvDecay, "Reverb Decay", 0.2f, 20.f, 2.5f, 0.3f, Sec::FX, "T60 (s)") \
    F(rvDamp, "Reverb Damp", 0.f, 1.f, 0.5f, 1.f, Sec::FX, "High damping") \
    F(rvMod, "Reverb Mod", 0.f, 1.f, 0.3f, 1.f, Sec::FX, "Line modulation (wobble)") \
    F(rvMix, "Reverb Mix", 0.f, 1.f, 0.25f, 1.f, Sec::FX, "Dry/wet") \
    F(stWidth, "Width", 0.f, 2.f, 1.f, 1.f, Sec::FX, "Stereo width (M/S)") \
    F(stHaas, "Haas", 0.f, 30.f, 0.f, 1.f, Sec::FX, "Right channel delay (ms)") \
    B(sfOn, "Spectral On", false, Sec::FX, "Spectral freeze / blur") \
    B(sfFreeze, "Spectral Freeze", false, Sec::FX, "Hold the current spectrum") \
    F(sfBlur, "Spectral Blur", 0.f, 1.f, 0.5f, 1.f, Sec::FX, "Spectral smearing over time") \
    F(sfMix, "Spectral Mix", 0.f, 1.f, 0.5f, 1.f, Sec::FX, "Dry/wet") \
    B(gbOn, "Glitch Buf On", false, Sec::FX, "Tempo-synced stutter buffer") \
    C(gbDiv, "Glitch Slice", "1/4|1/8|1/16|1/32|1/64|1/8T|1/16T", 2, Sec::FX, "Slice length") \
    F(gbProb, "Stutter Prob", 0.f, 1.f, 0.3f, 1.f, Sec::FX, "Chance per slice to stutter") \
    F(gbReverse, "Stutter Reverse", 0.f, 1.f, 0.3f, 1.f, Sec::FX, "Chance a stutter plays backwards") \
    F(gbPitch, "Stutter Pitch", -12.f, 12.f, 0.f, 1.f, Sec::FX, "Playback speed of stutters (st)") \
    F(gbGate, "Stutter Gate", 0.05f, 1.f, 1.f, 1.f, Sec::FX, "Chop each repeat shorter") \
    F(gbMix, "Glitch Mix", 0.f, 1.f, 1.f, 1.f, Sec::FX, "Dry/wet")

#define ZTX_ENUM_F(id, ...) id,
#define ZTX_ENUM_C(id, ...) id,
#define ZTX_ENUM_B(id, ...) id,
enum PID : int { ZTX_PARAMS(ZTX_ENUM_F, ZTX_ENUM_C, ZTX_ENUM_B) NUM_PARAMS };

enum class PType { Float, Choice, Bool };

struct ParamInfo
{
    const char* id;
    const char* name;
    PType type;
    float min, max, def, skew;
    Sec sec;
    const char* tip;
    juce::StringArray choices;
    juce::NormalisableRange<float> range;
};

const ParamInfo& info (int index);
juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

// --- modulation destinations (curated list of modulatable float params) -------------
const std::vector<int>& modDestinations();          // index 0 = None (-1)
juce::String modDestName (int destIndex);

enum ModSrc { SrcNone, SrcLfo1, SrcLfo2, SrcLfo3, SrcLfo4, SrcModEnv, SrcVelocity, SrcKey,
              SrcRandom, SrcStepLane, SrcFollow, SrcGen, SrcModWheel, NUM_SRC };

constexpr int kNumSlots = 8;
inline int slotSrc (int s) { return m1Src + s * 3; }
inline int slotDst (int s) { return m1Dst + s * 3; }
inline int slotAmt (int s) { return m1Amt + s * 3; }
inline int lfoRate  (int l) { return lfo1Rate  + l * 4; }
inline int lfoSync  (int l) { return lfo1Sync  + l * 4; }
inline int lfoDiv   (int l) { return lfo1Div   + l * 4; }
inline int lfoShape (int l) { return lfo1Shape + l * 4; }

double divisionInBeats (int divIndex);

enum Engine { EngRubber, EngGlitch, EngGran, EngFM, EngPerc, NUM_ENGINES };
juce::StringArray engineNames();
juce::StringArray engineShortNames();
Sec engineSection (int engine);

// Per-track "decay" / "colour" macro targets for each engine.
int engineDecayParam (int engine);
int engineColourParam (int engine);
} // namespace ztx
