#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace dembow::ui
{
    const juce::Colour bg        { 0xff120d1c };
    const juce::Colour panel     { 0xff1d1530 };
    const juce::Colour panelEdge { 0xff352a52 };
    const juce::Colour accent    { 0xffff3d8b };   // rosado neon
    const juce::Colour accent2   { 0xffffb03a };   // naranja
    const juce::Colour text      { 0xfff4eefc };
    const juce::Colour dim       { 0xffa497c2 };

    juce::Colour categoryColour (int c);

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float start, float end, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;
        void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (13.5f); }
        juce::Font getLabelFont (juce::Label& l) override
        {
            if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr) return juce::FontOptions (12.5f);
            return l.getFont();
        }
        juce::Font getPopupMenuFont() override { return juce::FontOptions (14.0f); }
    };
}

// Perilla: nombre arriba, valor abajo
struct Knob : public juce::Component
{
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
          const juce::String& title, const juce::String& tip = {});
    void resized() override;

    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Lista desplegable con su nombre encima
struct Choice : public juce::Component
{
    Choice (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
            const juce::String& title, const juce::String& tip = {});
    void resized() override;

    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

// Dibujo de la envolvente ADSR
class EnvelopeView : public juce::Component
{
public:
    EnvelopeView (juce::AudioProcessorValueTreeState& state, const juce::String& prefix, juce::Colour c);
    void paint (juce::Graphics&) override;
    bool needsRepaint();

private:
    std::atomic<float>* a; std::atomic<float>* d; std::atomic<float>* s; std::atomic<float>* r;
    juce::Colour colour;
    float last[4] = { -1, -1, -1, -1 };
};

// Seccion ADSR: dibujo + 4 deslizadores verticales grandes
class EnvSection : public juce::Component
{
public:
    EnvSection (juce::AudioProcessorValueTreeState& state, const juce::String& prefix, juce::Colour c);
    void resized() override;
    EnvelopeView view;

private:
    struct Fader
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::array<Fader, 4> faders;
};

// Navegador de sonidos por categorias
class PresetBrowser : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PresetBrowser (DembowProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();   // cuando cambia el preset desde fuera

private:
    int getNumRows() override { return (int) filtered.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void selectCategory (int c);
    void rebuild();

    DembowProcessor& proc;
    std::array<juce::TextButton, dembow::NumCategories> catButtons;
    juce::TextButton eraAll { "Todos" }, eraOld { "Old school" }, eraNew { "Moderno" };
    juce::ListBox list;
    std::vector<int> filtered;
    int category = 0;
    int era = 0;   // 0 todos, 1 old school, 2 moderno
};

class DembowEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DembowEditor (DembowProcessor&);
    ~DembowEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void stepPreset (int delta);
    void drawPanel (juce::Graphics&, juce::Rectangle<int> r, const juce::String& title, juce::Colour c);

    DembowProcessor& proc;
    dembow::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 500 };

    PresetBrowser browser;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::Label presetName, presetInfo;

    Choice o1wave, o2wave;
    Knob o1oct, o1semi, o1fine, o1level, o2oct, o2semi, o2fine, o2level;
    Knob sub, noise, pw, detune, fm, fmenv;

    Choice ftype;
    Knob cutoff, reso, fenv, keytrk, drive;
    EnvSection fltEnv, ampEnv;

    Choice lfowave, lfodest;
    Knob lforate, lfodepth, lfofade;

    Choice mode;
    Knob glide, velsens;

    Choice dlytime;
    Knob chorus, dlymix, dlyfb, revmix, revsize, volume;

    juce::MidiKeyboardComponent keyboard;

    // Zonas de los paneles (para pintarlos)
    juce::Rectangle<int> rOsc1, rOsc2, rMix, rFilter, rFltEnv, rAmpEnv, rLfo, rVoice, rFx, rBrowser;
    int lastProgramChanges = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DembowEditor)
};
