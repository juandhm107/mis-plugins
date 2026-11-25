#pragma once
#include "Widgets.h"

namespace ztx
{
class SeqPage final : public Page
{
public:
    explicit SeqPage (ZtxProcessor&);
    ~SeqPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick() override;

    enum EditMode { EdGate, EdVel, EdProb, EdRatchet, EdPitch, EdMicro };

private:
    class StepGrid;
    class TrackStrip;
    class LaneEditor;
    void syncFromPattern();
    void refreshButtons();

    std::unique_ptr<StepGrid> grid;
    std::vector<std::unique_ptr<TrackStrip>> strips;
    std::unique_ptr<LaneEditor> lane;

    Knob densK, compK, chaosK, grooveK, repK, swingK, humanK, probK, bpmK;
    ParamChoice stepsC, midiC;
    ParamToggle runT;
    juce::TextButton genBtn { "GENERATE" }, mutBtn { "MUTATE" }, freezeBtn { "FREEZE" }, undoBtn { "UNDO" }, clearBtn { "CLEAR" };
    juce::TextButton varBtn[kVariations];
    juce::TextButton modeBtn[6];
    juce::uint32 lastEpoch = 0;
    juce::Rectangle<int> genPod, clockPod, gridPod, bottomPod;
};
} // namespace ztx
