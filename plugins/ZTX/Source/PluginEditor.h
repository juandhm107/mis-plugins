#pragma once
#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/Widgets.h"

namespace ztx
{
class LabPage; class SeqPage; class ModPage; class FxPage; class PresetPage;

class ZtxEditor final : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit ZtxEditor (ZtxProcessor&);
    ~ZtxEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kBaseW = 1240, kBaseH = 800;

    void timerCallback() override;

private:
    class Root;

    ZtxProcessor& proc;
    ZtxLookAndFeel lnf;
    std::unique_ptr<Root> root;
    juce::TooltipWindow tooltips { this, 600 };
};
} // namespace ztx
