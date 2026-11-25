#pragma once
#include "../PluginProcessor.h"

namespace ztx
{
struct PresetBuilder;
struct FactoryPreset
{
    const char* name;
    const char* category;
    const char* description;
    void (*build) (PresetBuilder&);
};

const std::vector<FactoryPreset>& factoryPresets();
juce::StringArray presetCategories();
void applyFactoryPreset (ZtxProcessor&, int index);

// User presets live in  ~/Library/Application Support/ZTX Organic Glitch Lab/Presets  (macOS)
juce::File userPresetFolder();
juce::Array<juce::File> userPresets();
bool saveUserPreset (ZtxProcessor&, const juce::String& name);
bool loadUserPreset (ZtxProcessor&, const juce::File&);

// Builder used by factory presets.
struct PresetBuilder
{
    explicit PresetBuilder (ZtxProcessor& p) : proc (p) {}
    ZtxProcessor& proc;
    PresetBuilder& set (int pid, float v) { proc.setParam (pid, v); return *this; }
    PresetBuilder& on (int pid) { return set (pid, 1.0f); }
    PresetBuilder& engine (int e) { return set (labEngine, (float) e); }
    PresetBuilder& lfo (int l, float rate, int shape, bool sync = false, int div = 4);
    PresetBuilder& mod (int slot, int src, int destPid, float amt);
    PresetBuilder& track (int t, int engine, int note, float level = 0.8f, float pan = 0.0f, float gate = 0.5f);
    PresetBuilder& macro (int t, float decayMod, float colourMod);
    PresetBuilder& steps (int t, const char* pattern, int div = 6);   // x=accent o=soft r=ratchet2 R=ratchet3 ?=50% .=off
    PresetBuilder& euclid (int t, int hits, int len, int rot = 0, int div = 6);
    PresetBuilder& generate (uint32_t seed, float density, float complexity, float chaos, float groove, float repetition);
    PresetBuilder& emptyPattern (int bassEngine, int bassNote);
    PresetBuilder& lockTrack (int t, bool l = true);
    Pattern& pat() { return proc.variations[0]; }
};
} // namespace ztx
