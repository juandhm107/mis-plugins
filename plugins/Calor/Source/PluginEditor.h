#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace calor::ui
{
    const juce::Colour bg        { 0xff15100e };
    const juce::Colour panel     { 0xff211915 };
    const juce::Colour panelEdge { 0xff3a2a22 };
    const juce::Colour accent    { 0xffff7a2f };
    const juce::Colour accent2   { 0xffffc15e };
    const juce::Colour text      { 0xfff3e6dc };
    const juce::Colour dim       { 0xff9c8578 };

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float start, float end, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
        void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (14.0f); }
        juce::Font getLabelFont (juce::Label&) override { return juce::FontOptions (12.5f); }
    };
}

// Perilla con nombre arriba y valor abajo
struct Knob : public juce::Component
{
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& title);
    void resized() override;

    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Pad circular XY: X = calor (saturacion global), Y = brillo (tono global)
class XYPad : public juce::Component, public juce::SettableTooltipClient
{
public:
    XYPad (CalorProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> circle() const;
    void setFromMouse (juce::Point<float> pos);

    CalorProcessor& proc;
    juce::RangedAudioParameter* heat;
    juce::RangedAudioParameter* character;
};

class StagePanel : public juce::Component
{
public:
    StagePanel (juce::AudioProcessorValueTreeState& state, int index);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setTitle (const juce::String& t) { if (t != title) { title = t; repaint(); } }

private:
    int index;
    juce::String title;
    juce::ToggleButton onButton;
    juce::ComboBox typeBox;
    Knob drive, mix, tone;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;
};

class CalorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CalorEditor (CalorProcessor&);
    ~CalorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    CalorProcessor& proc;
    calor::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    juce::ComboBox presetBox, modeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttach;

    XYPad pad;
    std::array<std::unique_ptr<StagePanel>, CalorProcessor::kNumStages> stages;

    Knob xoverLo, xoverHi, lfoRate, lfoDepth, envAmount, width, outTone, mix, output;
    juce::ToggleButton autoGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoGainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CalorEditor)
};
