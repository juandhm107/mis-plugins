#pragma once
#include "Widgets.h"

namespace ztx
{
class LabPage final : public Page
{
public:
    explicit LabPage (ZtxProcessor&);
    ~LabPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick() override;

private:
    class EngineCard;
    class SampleView;
    class AlgoView;
    class TouchPad;
    void showEngine (int e);

    std::vector<std::unique_ptr<EngineCard>> cards;
    std::vector<std::unique_ptr<Knob>> engineKnobs[NUM_ENGINES];
    std::unique_ptr<ParamToggle> freeze;
    std::unique_ptr<ParamChoice> algo;
    std::unique_ptr<SampleView> sampleView;
    std::unique_ptr<AlgoView> algoView;
    std::unique_ptr<TouchPad> pad;
    juce::TextButton loadBtn { "LOAD" }, capOutBtn { "RESAMPLE 4s" }, capInBtn { "CAPTURE IN" }, defaultBtn { "BUILT-IN" };
    std::unique_ptr<juce::FileChooser> chooser;

    Knob ampAK, ampDK, ampSK, ampRK, velK, pitchK;
    ParamChoice gateC;
    Scope scope;
    juce::MidiKeyboardComponent keyboard;

    int shownEngine = -1;
    juce::Rectangle<int> enginePod, ampPod, scopePod, cardsPod;
};
} // namespace ztx
