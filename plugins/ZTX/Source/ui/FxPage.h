#pragma once
#include "Widgets.h"

namespace ztx
{
class FxPage final : public Page
{
public:
    explicit FxPage (ZtxProcessor&);
    ~FxPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick() override;

private:
    struct Module
    {
        juce::String title, blurb;
        int onPid = -1;
        std::unique_ptr<ParamToggle> power;
        std::vector<std::unique_ptr<ParamChoice>> choices;
        std::vector<std::unique_ptr<ParamToggle>> toggles;
        std::vector<std::unique_ptr<Knob>> knobs;
        juce::Rectangle<int> area;
    };
    std::vector<std::unique_ptr<Module>> modules;
    ParamChoice order;
    juce::Rectangle<int> headerPod;
    juce::uint32 lastMask = 0xffffffff;
};
} // namespace ztx
