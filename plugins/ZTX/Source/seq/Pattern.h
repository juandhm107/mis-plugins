#pragma once
#include <array>
#include <juce_data_structures/juce_data_structures.h>
#include "../dsp/DspUtils.h"

namespace ztx
{
constexpr int kTracks = 4;
constexpr int kMaxSteps = 32;
constexpr int kLaneSteps = 16;
constexpr int kVariations = 4;

struct Step
{
    bool on = false;
    float vel = 0.8f;      // 0..1
    float prob = 1.0f;     // 0..1
    int ratchet = 1;       // 1..4
    float micro = 0.0f;    // 0..0.5 of a step (late)
    int pitch = 0;         // semitones
};

struct Track
{
    std::array<Step, kMaxSteps> steps {};
    int length = 16;
    int div = 6;            // index into ZTX_DIVS (6 = 1/16)
    int engine = 4;
    int note = 48;
    float gate = 0.5f;      // fraction of a step
    float level = 0.8f;
    float pan = 0.0f;
    float decayMod = 0.0f;  // -1..1 offsets this engine's decay macro
    float colourMod = 0.0f; // -1..1 offsets this engine's colour macro
    bool mute = false;
    bool lock = false;      // protected from GENERATE / MUTATE
    int euclidHits = 4;
    int euclidRot = 0;
};

struct Pattern
{
    std::array<Track, kTracks> tracks {};
    std::array<float, kLaneSteps> lane {};
    Pattern();
};

struct GenSettings
{
    float density = 0.5f, complexity = 0.4f, chaos = 0.2f, groove = 0.4f, repetition = 0.6f;
    int maxSteps = 16;
};

// Musical, controllable generation. All respect Track::lock.
void generatePattern (Pattern&, const GenSettings&, Rng&);
void mutatePattern (Pattern&, const GenSettings&, Rng&, float strength = 1.0f);
void applyEuclid (Track&, int hits, int rotation);
void clearTrack (Track&);
std::array<bool, kMaxSteps> euclid (int hits, int length, int rotation);

juce::ValueTree patternToTree (const Pattern&, const juce::Identifier& type);
void patternFromTree (Pattern&, const juce::ValueTree&);
} // namespace ztx
