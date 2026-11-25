#pragma once
#include "Widgets.h"

namespace ztx
{
class ModPage final : public Page
{
public:
    explicit ModPage (ZtxProcessor&);
    ~ModPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick() override;

private:
    class Trace;
    class SlotRow;
    struct LfoUi
    {
        std::unique_ptr<Knob> rate;
        std::unique_ptr<ParamChoice> shape, div;
        std::unique_ptr<ParamToggle> sync;
        std::unique_ptr<Trace> trace;
    };
    LfoUi lfos[4];
    Knob meAK, meDK, meSK, meRK, rndRateK, rndSmoothK, efAttK, efRelK, genRateK, genChaosK;
    std::unique_ptr<Trace> envTrace, rndTrace, efTrace, genTrace, laneTrace;
    std::vector<std::unique_ptr<SlotRow>> slots;
    juce::Rectangle<int> lfoPods[4], srcPods[4], matrixPod;
};
} // namespace ztx
