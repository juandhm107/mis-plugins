#include "Widgets.h"

namespace ztx
{
using namespace theme;

Knob::Knob (ZtxProcessor& p, int id, const juce::String& n, bool sm)
    : pid (id), proc (p), name (n.isEmpty() ? juce::String (info (id).name) : n), small (sm)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setTooltip (juce::String (info (id).name) + " - " + info (id).tip);
    slider.setDoubleClickReturnValue (true, info (id).def);
    slider.setMouseDragSensitivity (220);
    addAndMakeVisible (slider);
    att = std::make_unique<SliderAtt> (proc.apvts, info (id).id, slider);
    slider.setDoubleClickReturnValue (true, info (id).def);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    const int labelH = small ? 12 : 14;
    r.removeFromBottom (labelH);
    const int s = std::min (r.getWidth(), r.getHeight());
    slider.setBounds (r.withSizeKeepingCentre (s, s));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (textDim);
    g.setFont (label (small ? 9.5f : 10.5f));
    g.drawFittedText (name.toUpperCase(), getLocalBounds().removeFromBottom (small ? 12 : 14), juce::Justification::centred, 1, 0.6f);
}

void Knob::updateMod()
{
    const float m = proc.modDisplay[pid].load (std::memory_order_relaxed);
    if (std::abs (m - lastMod) > 0.003f)
    {
        lastMod = m;
        slider.getProperties().set ("mod", m);
        slider.repaint();
    }
}

ParamToggle::ParamToggle (ZtxProcessor& p, int pid, const juce::String& t, bool accent)
{
    setButtonText (t.isEmpty() ? juce::String (info (pid).name).toUpperCase() : t);
    setClickingTogglesState (true);
    getProperties().set ("led", true);
    if (accent) getProperties().set ("accent", true);
    setTooltip (info (pid).tip);
    att = std::make_unique<ButtonAtt> (p.apvts, info (pid).id, *this);
}

ParamChoice::ParamChoice (ZtxProcessor& p, int pid, const juce::String& cap) : caption (cap)
{
    const auto& inf = info (pid);
    const auto& dests = modDestinations();
    if (inf.choices.size() == (int) dests.size() && inf.choices[0] == "None" && juce::String (inf.id).endsWith ("Dst"))
    {
        // mod destinations grouped by section
        Sec last = Sec::Global;
        box.addItem ("None", 1);
        for (size_t i = 1; i < dests.size(); ++i)
        {
            const auto& di = info (dests[i]);
            if (i == 1 || di.sec != last)
            {
                static const char* secNames[] = { "GLOBAL", "RUBBER LAB", "MICRO GLITCH", "GRANULAR", "FM METALLIC", "ORGANIC PERC", "SEQUENCER", "MOD", "DESTRUCTION", "EVOLVE" };
                box.addSectionHeading (secNames[(int) di.sec]);
                last = di.sec;
            }
            box.addItem (di.name, (int) i + 1);
        }
    }
    else
        box.addItemList (inf.choices, 1);
    box.setTooltip (inf.tip);
    addAndMakeVisible (box);
    att = std::make_unique<ComboAtt> (p.apvts, inf.id, box);
}

void ParamChoice::resized()
{
    auto r = getLocalBounds();
    if (caption.isNotEmpty()) r.removeFromTop (13);
    box.setBounds (r);
}

void ParamChoice::paint (juce::Graphics& g)
{
    if (caption.isEmpty()) return;
    g.setColour (textDim);
    g.setFont (label (10.0f));
    g.drawText (caption.toUpperCase(), getLocalBounds().removeFromTop (13), juce::Justification::centredLeft);
}

NumBox::NumBox (double lo, double hi, double step)
{
    setSliderStyle (juce::Slider::LinearBar);
    setTextBoxStyle (juce::Slider::TextBoxAbove, false, 40, 18);
    setRange (lo, hi, step);
    setMouseDragSensitivity (160);
    setVelocityBasedMode (false);
}

void Scope::tick()
{
    frame += 1.0f;
    repaint();
}

void Scope::paint (juce::Graphics& g)
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;
    if (! trail.isValid() || trail.getWidth() != w || trail.getHeight() != h)
        trail = juce::Image (juce::Image::ARGB, w, h, true);

    {
        juce::Graphics tg (trail);
        // fade previous frames (phosphor)
        tg.setColour (juce::Colour (0xff0b0c0a).withAlpha (0.28f));
        tg.fillAll();
        const int N = ZtxProcessor::kScope;
        const int wp = proc.scopePos.load();
        // trigger: find rising zero-crossing in the last half buffer
        int start = (wp - N / 2 + N) % N;
        for (int k = 0; k < N / 4; ++k)
        {
            const int i0 = (wp - N / 2 - k + 2 * N) % N, i1 = (i0 + 1) % N;
            if (proc.scope[i0] <= 0.0f && proc.scope[i1] > 0.0f) { start = i0; break; }
        }
        const int span = N / 2;
        juce::Path p;
        float peak = 0.05f;
        for (int i = 0; i < span; ++i) peak = std::max (peak, std::abs (proc.scope[(start + i) % N]));
        const float gain = 0.42f / std::max (0.15f, peak);
        for (int x = 0; x < w; ++x)
        {
            const int i = (start + x * span / w) % N;
            const float y = (float) h * 0.5f - proc.scope[i] * gain * (float) h;
            if (x == 0) p.startNewSubPath (0, y); else p.lineTo ((float) x, y);
        }
        tg.setColour (acid.withAlpha (0.18f));
        tg.strokePath (p, juce::PathStrokeType (4.0f));
        tg.setColour (acid.withAlpha (0.95f));
        tg.strokePath (p, juce::PathStrokeType (1.3f));
    }
    g.drawImageAt (trail, 0, 0);
    g.setColour (silver.withAlpha (0.08f));
    for (int k = 1; k < 4; ++k) g.drawHorizontalLine (h * k / 4, 0, (float) w);
    for (int k = 1; k < 8; ++k) g.drawVerticalLine (w * k / 8, 0, (float) h);
}

void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap)
{
    const int n = (int) items.size();
    if (n == 0) return;
    const int w = (area.getWidth() - gap * (n - 1)) / n;
    for (auto* c : items) { c->setBounds (area.removeFromLeft (w)); area.removeFromLeft (gap); }
}

juce::String noteName (int note)
{
    static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (n[note % 12]) + juce::String (note / 12 - 2);
}
} // namespace ztx
