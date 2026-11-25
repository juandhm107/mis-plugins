#include "FxPage.h"

namespace ztx
{
using namespace theme;

namespace
{
struct Def
{
    const char* title; const char* blurb; int on;
    std::vector<int> choices, toggles, knobs;
};
}

FxPage::FxPage (ZtxProcessor& p) : Page (p), order (p, fxOrder, "Chain order")
{
    const std::vector<Def> defs = {
        { "FILTER", "multimode / acid / vowel", fltOn, { fltMode }, {}, { fltCutoff, fltReso, fltDrive } },
        { "WAVEFOLDER", "triangle folds + bias", foldOn, {}, {}, { foldAmt, foldBias, foldMix } },
        { "SATURATION", "tanh / tube / rectify", satOn, { satType }, {}, { satDrive, satMix } },
        { "BITCRUSH", "bits + sample hold", crOn, {}, {}, { crBits, crRate, crMix } },
        { "FREQ SHIFT", "Bode shifter, barber pole", fsOn, {}, {}, { fsShift, fsFb, fsMix } },
        { "RING MOD", "sine -> square carrier", rmOn, {}, {}, { rmFreq, rmShape, rmMix } },
        { "COMB", "metallic / hollow tube", cbOn, {}, {}, { cbFreq, cbFb, cbDamp, cbMix } },
        { "GLITCH BUFFER", "tempo stutter / reverse", gbOn, { gbDiv }, {}, { gbProb, gbReverse, gbPitch, gbGate, gbMix } },
        { "SPECTRAL", "FFT freeze / blur", sfOn, {}, { sfFreeze }, { sfBlur, sfMix } },
        { "DELAY", "saturated feedback, warble", dlOn, { dlDiv }, { dlSync, dlPing }, { dlTime, dlFb, dlTone, dlWarble, dlMix } },
        { "REVERB", "dark modulated FDN", rvOn, {}, {}, { rvSize, rvDecay, rvDamp, rvMod, rvMix } },
        { "STEREO", "width + haas", -1, {}, {}, { stWidth, stHaas } },
    };
    for (const auto& d : defs)
    {
        auto m = std::make_unique<Module>();
        m->title = d.title; m->blurb = d.blurb; m->onPid = d.on;
        if (d.on >= 0) { m->power = std::make_unique<ParamToggle> (p, d.on, "ON"); addAndMakeVisible (*m->power); }
        for (int c : d.choices) { m->choices.push_back (std::make_unique<ParamChoice> (p, c)); addAndMakeVisible (*m->choices.back()); }
        for (int t : d.toggles)
        {
            juce::String nm = info (t).name;
            nm = nm.replace ("Delay ", "").replace ("Spectral ", "").toUpperCase();
            m->toggles.push_back (std::make_unique<ParamToggle> (p, t, nm, t == sfFreeze));
            addAndMakeVisible (*m->toggles.back());
        }
        for (int k : d.knobs)
        {
            juce::String nm = info (k).name;
            for (auto pre : { "Filter ", "Fold ", "Sat ", "Crush ", "Shift ", "Ring ", "Comb ", "Stutter ", "Glitch ", "Spectral ", "Delay ", "Reverb " })
                nm = nm.replace (pre, "");
            m->knobs.push_back (std::make_unique<Knob> (p, k, nm, true));
            addKnob (*m->knobs.back());
        }
        modules.push_back (std::move (m));
    }
    addAndMakeVisible (order);
}

FxPage::~FxPage() = default;

void FxPage::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    headerPod = r.removeFromTop (46);
    order.setBounds (headerPod.reduced (12, 4).removeFromRight (200));
    r.removeFromTop (8);
    const int cols = 4, rows = 3;
    const int cw = (r.getWidth() - (cols - 1) * 10) / cols, rh = (r.getHeight() - (rows - 1) * 10) / rows;
    for (int i = 0; i < (int) modules.size(); ++i)
    {
        auto& m = *modules[(size_t) i];
        m.area = { r.getX() + (i % cols) * (cw + 10), r.getY() + (i / cols) * (rh + 10), cw, rh };
        auto a = m.area.reduced (10, 8);
        auto head = a.removeFromTop (24);
        if (m.power) m.power->setBounds (head.removeFromRight (56).reduced (0, 1));
        a.removeFromTop (16);
        if (! m.choices.empty() || ! m.toggles.empty())
        {
            auto row = a.removeFromTop (26);
            for (auto& c : m.choices) { c->setBounds (row.removeFromLeft (96).reduced (2, 1)); }
            for (auto& t : m.toggles) { t->setBounds (row.removeFromLeft (76).reduced (2, 1)); }
            a.removeFromTop (4);
        }
        const int n = (int) m.knobs.size();
        const int kw = std::min (70, a.getWidth() / std::max (1, n));
        auto kr = a.withSizeKeepingCentre (kw * n, std::min (a.getHeight(), 84));
        for (auto& k : m.knobs) k->setBounds (kr.removeFromLeft (kw).reduced (1));
    }
}

void FxPage::paint (juce::Graphics& g)
{
    drawPod (g, headerPod.toFloat(), 60);
    drawGlitchText (g, "DESTRUCTION", headerPod.reduced (16, 6).toFloat(), mono (20.0f, true), 9);
    g.setColour (textDim); g.setFont (mono (11.0f));
    g.drawText ("12 chainable processors. Every orange arc is live modulation.", headerPod.reduced (16, 6).withTrimmedLeft (190).withTrimmedRight (210),
                juce::Justification::centredLeft);
    for (size_t i = 0; i < modules.size(); ++i)
    {
        auto& m = *modules[i];
        const bool on = m.onPid < 0 || proc.getParam (m.onPid) > 0.5f;
        drawPod (g, m.area.toFloat(), 70 + (int) i, on ? panel2 : panel, on ? 0.95f : 0.7f);
        auto a = m.area.reduced (12, 8);
        g.setColour (on ? text : silverDim);
        g.setFont (label (12.5f));
        g.drawText (m.title, a.removeFromTop (24), juce::Justification::centredLeft);
        g.setColour (textDim); g.setFont (mono (9.5f));
        g.drawText (m.blurb, a.removeFromTop (14), juce::Justification::centredLeft);
    }
}

void FxPage::tick()
{
    juce::uint32 mask = 0;
    for (size_t i = 0; i < modules.size(); ++i)
        if (modules[i]->onPid >= 0 && proc.getParam (modules[i]->onPid) > 0.5f) mask |= 1u << i;
    if (mask != lastMask) { lastMask = mask; repaint(); }
    tickKnobs();
}
} // namespace ztx
