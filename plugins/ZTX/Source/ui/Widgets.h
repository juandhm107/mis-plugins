#pragma once
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

namespace ztx
{
using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

// Rotary parameter knob with name, tooltip, drag value popup and a live modulation arc.
class Knob final : public juce::Component
{
public:
    Knob (ZtxProcessor& p, int pid, const juce::String& name = {}, bool small = false);
    void resized() override;
    void paint (juce::Graphics&) override;
    void updateMod();
    juce::Slider slider;
    const int pid;
private:
    ZtxProcessor& proc;
    juce::String name;
    bool small;
    float lastMod = 0;
    std::unique_ptr<SliderAtt> att;
};

// Toggle bound to a bool parameter (pill with LED).
class ParamToggle final : public juce::TextButton
{
public:
    ParamToggle (ZtxProcessor& p, int pid, const juce::String& text = {}, bool accent = false);
private:
    std::unique_ptr<ButtonAtt> att;
};

// ComboBox bound to a choice parameter, with an optional caption painted above.
class ParamChoice final : public juce::Component
{
public:
    ParamChoice (ZtxProcessor& p, int pid, const juce::String& caption = {});
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::ComboBox box;
private:
    juce::String caption;
    std::unique_ptr<ComboAtt> att;
};

// Small numeric drag box (not a host parameter) - used for pattern data.
class NumBox final : public juce::Slider
{
public:
    NumBox (double lo, double hi, double step = 1.0);
};

// Phosphor oscilloscope reading the processor's output ring buffer.
class Scope final : public juce::Component
{
public:
    explicit Scope (ZtxProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void tick();
private:
    ZtxProcessor& proc;
    juce::Image trail;
    float frame = 0;
};

// Every page gets a 30 Hz tick from the editor.
class Page : public juce::Component
{
public:
    explicit Page (ZtxProcessor& p) : proc (p) {}
    virtual void tick() {}
    void addKnob (Knob& k) { addAndMakeVisible (k); knobs.push_back (&k); }
    void tickKnobs() { for (auto* k : knobs) if (k->isShowing()) k->updateMod(); }
protected:
    ZtxProcessor& proc;
    std::vector<Knob*> knobs;
};

// Lays knobs out in a row inside an area.
void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap = 4);
juce::String noteName (int note);
} // namespace ztx
