#include "ModPage.h"

namespace ztx
{
using namespace theme;

// Scrolling trace of a modulation source (real-time modulation visualisation).
class ModPage::Trace final : public juce::Component
{
public:
    Trace (ZtxProcessor& p, int s, bool uni = false) : proc (p), src (s), unipolar (uni) { hist.fill (0.0f); }
    void push()
    {
        float v = proc.srcDisplay[src].load();
        if (src == SrcModEnv) v = envLevel();
        hist[(size_t) pos] = v;
        pos = (pos + 1) % (int) hist.size();
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle (r, 4);
        g.setColour (silver.withAlpha (0.12f));
        g.drawHorizontalLine ((int) yFor (0.0f, r), r.getX(), r.getRight());
        juce::Path p;
        const int n = (int) hist.size();
        for (int i = 0; i < n; ++i)
        {
            const float v = hist[(size_t) ((pos + i) % n)];
            const float x = r.getX() + r.getWidth() * (float) i / (float) (n - 1);
            if (i == 0) p.startNewSubPath (x, yFor (v, r)); else p.lineTo (x, yFor (v, r));
        }
        g.setColour (acid.withAlpha (0.2f));
        g.strokePath (p, juce::PathStrokeType (3.5f));
        g.setColour (acid);
        g.strokePath (p, juce::PathStrokeType (1.2f));
        const float last = hist[(size_t) ((pos + n - 1) % n)];
        g.setColour (orange);
        g.fillEllipse (juce::Rectangle<float> (7, 7).withCentre ({ r.getRight() - 4, yFor (last, r) }));
    }
private:
    float envLevel() { return 0.0f; }
    float yFor (float v, juce::Rectangle<float> r) const
    {
        const float n = unipolar ? v : (v + 1.0f) * 0.5f;
        return r.getBottom() - 3 - clamp01 (n) * (r.getHeight() - 6);
    }
    ZtxProcessor& proc;
    int src;
    bool unipolar;
    std::array<float, 140> hist {};
    int pos = 0;
};

// One modulation-matrix slot.
class ModPage::SlotRow final : public juce::Component
{
public:
    SlotRow (ZtxProcessor& p, int s)
        : proc (p), slot (s), src (p, slotSrc (s)), dst (p, slotDst (s))
    {
        addAndMakeVisible (src); addAndMakeVisible (dst);
        amt.setSliderStyle (juce::Slider::LinearHorizontal);
        amt.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
        amt.setTooltip ("Modulation amount (bipolar). Double-click = 0");
        amt.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (amt);
        att = std::make_unique<SliderAtt> (p.apvts, info (slotAmt (s)).id, amt);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (2);
        r.removeFromLeft (22);
        src.setBounds (r.removeFromLeft (110)); r.removeFromLeft (6);
        dst.setBounds (r.removeFromLeft (150)); r.removeFromLeft (6);
        meter = r.removeFromRight (40);
        amt.setBounds (r);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (textDim); g.setFont (mono (11.0f, true));
        g.drawText (juce::String (slot + 1), getLocalBounds().removeFromLeft (20), juce::Justification::centred);
        // live value = source * amount
        const int s = (int) proc.getParam (slotSrc (slot));
        const float v = s > 0 ? proc.srcDisplay[s].load() * proc.getParam (slotAmt (slot)) : 0.0f;
        auto m = meter.toFloat().reduced (4, 7);
        g.setColour (panel2); g.fillRoundedRectangle (m, 2);
        const float cx = m.getCentreX();
        g.setColour (v >= 0 ? acid : orange);
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (std::min (cx, cx + v * m.getWidth() * 0.5f), m.getY(), std::max (cx, cx + v * m.getWidth() * 0.5f), m.getBottom()));
    }
private:
    ZtxProcessor& proc;
    int slot;
    ParamChoice src, dst;
    juce::Slider amt;
    std::unique_ptr<SliderAtt> att;
    juce::Rectangle<int> meter;
};

ModPage::ModPage (ZtxProcessor& p)
    : Page (p),
      meAK (p, meA, "Attack"), meDK (p, meD, "Decay"), meSK (p, meS, "Sustain"), meRK (p, meR, "Release"),
      rndRateK (p, rndRate, "Rate"), rndSmoothK (p, rndSmooth, "Smooth"), efAttK (p, efAtt, "Attack"), efRelK (p, efRel, "Release"),
      genRateK (p, genRate, "Rate"), genChaosK (p, genChaos, "Chaos")
{
    for (int l = 0; l < 4; ++l)
    {
        auto& u = lfos[l];
        u.rate = std::make_unique<Knob> (p, lfoRate (l), "Rate");
        u.shape = std::make_unique<ParamChoice> (p, lfoShape (l), "Shape");
        u.div = std::make_unique<ParamChoice> (p, lfoDiv (l), "Sync Div");
        u.sync = std::make_unique<ParamToggle> (p, lfoSync (l), "SYNC");
        u.trace = std::make_unique<Trace> (p, SrcLfo1 + l);
        addKnob (*u.rate);
        for (juce::Component* c : std::initializer_list<juce::Component*> { u.shape.get(), u.div.get(), u.sync.get(), u.trace.get() }) addAndMakeVisible (*c);
    }
    for (auto* k : { &meAK, &meDK, &meSK, &meRK, &rndRateK, &rndSmoothK, &efAttK, &efRelK, &genRateK, &genChaosK }) addKnob (*k);
    rndTrace = std::make_unique<Trace> (p, SrcRandom);
    efTrace = std::make_unique<Trace> (p, SrcFollow, true);
    genTrace = std::make_unique<Trace> (p, SrcGen);
    laneTrace = std::make_unique<Trace> (p, SrcStepLane);
    for (auto* t : { rndTrace.get(), efTrace.get(), genTrace.get(), laneTrace.get() }) addAndMakeVisible (*t);
    for (int s = 0; s < kNumSlots; ++s)
    {
        slots.push_back (std::make_unique<SlotRow> (p, s));
        addAndMakeVisible (*slots.back());
    }
}

ModPage::~ModPage() = default;

void ModPage::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    auto top = r.removeFromTop (220);
    const int lw = (top.getWidth() - 30) / 4;
    for (int l = 0; l < 4; ++l)
    {
        lfoPods[l] = top.removeFromLeft (lw);
        top.removeFromLeft (10);
        auto a = lfoPods[l].reduced (12, 8);
        a.removeFromTop (18);
        lfos[l].trace->setBounds (a.removeFromTop (70));
        a.removeFromTop (6);
        auto left = a.removeFromLeft (a.getWidth() * 4 / 10);
        lfos[l].rate->setBounds (left);
        lfos[l].shape->setBounds (a.removeFromTop (38).reduced (2, 0));
        lfos[l].div->setBounds (a.removeFromTop (38).reduced (2, 0));
        lfos[l].sync->setBounds (a.removeFromTop (28).reduced (2, 3));
    }
    r.removeFromTop (10);
    auto mid = r.removeFromTop (150);
    const int sw = (mid.getWidth() - 30) / 4;
    for (int s = 0; s < 4; ++s) { srcPods[s] = mid.removeFromLeft (sw); mid.removeFromLeft (10); }
    {
        auto a = srcPods[0].reduced (10, 8); a.removeFromTop (18);
        layoutRow (a, { &meAK, &meDK, &meSK, &meRK }, 2);
    }
    auto srcLayout = [] (juce::Rectangle<int> pod, Knob& k1, Knob& k2, juce::Component& tr)
    {
        auto a = pod.reduced (10, 8); a.removeFromTop (18);
        auto knobs = a.removeFromLeft (a.getWidth() / 2);
        layoutRow (knobs, { &k1, &k2 }, 2);
        tr.setBounds (a.reduced (4, 6));
    };
    srcLayout (srcPods[1], rndRateK, rndSmoothK, *rndTrace);
    srcLayout (srcPods[2], efAttK, efRelK, *efTrace);
    srcLayout (srcPods[3], genRateK, genChaosK, *genTrace);
    r.removeFromTop (10);
    matrixPod = r;
    auto m = matrixPod.reduced (12, 8);
    m.removeFromTop (18);
    auto laneArea = m.removeFromRight (180);
    laneTrace->setBounds (laneArea.reduced (6, 18));
    const int colW = m.getWidth() / 2;
    auto c1 = m.removeFromLeft (colW), c2 = m;
    const int rh = std::min (34, c1.getHeight() / 4);
    for (int s = 0; s < kNumSlots; ++s)
        slots[(size_t) s]->setBounds ((s < 4 ? c1 : c2).withHeight (rh).translated (0, (s % 4) * rh).reduced (4, 1));
}

void ModPage::paint (juce::Graphics& g)
{
    static const char* srcNames[] = { "MOD ENVELOPE (per voice ADSR)", "RANDOM / S&H", "ENVELOPE FOLLOWER", "GENERATIVE (attractor)" };
    for (int l = 0; l < 4; ++l)
    {
        drawPod (g, lfoPods[l].toFloat(), 30 + l);
        g.setColour (silver); g.setFont (label (11.0f));
        g.drawText ("LFO " + juce::String (l + 1), lfoPods[l].reduced (14, 8).removeFromTop (14), juce::Justification::left);
    }
    for (int s = 0; s < 4; ++s)
    {
        drawPod (g, srcPods[s].toFloat(), 40 + s);
        g.setColour (silver); g.setFont (label (11.0f));
        g.drawText (srcNames[s], srcPods[s].reduced (12, 8).removeFromTop (14), juce::Justification::left);
    }
    drawPod (g, matrixPod.toFloat(), 50);
    g.setColour (silver); g.setFont (label (11.0f));
    auto t = matrixPod.reduced (14, 8).removeFromTop (14);
    g.drawText ("MODULATION MATRIX  -  8 slots, any source to any destination", t, juce::Justification::left);
    g.drawText ("STEP LANE", t.removeFromRight (176), juce::Justification::left);
    g.setColour (textDim); g.setFont (mono (10.0f));
    g.drawText ("Velocity / Key / Mod Env are per-voice.  Orange arcs on knobs show live modulation.",
                matrixPod.reduced (14, 6).removeFromBottom (14), juce::Justification::left);
}

void ModPage::tick()
{
    for (auto& l : lfos) l.trace->push();
    for (auto* t : { rndTrace.get(), efTrace.get(), genTrace.get(), laneTrace.get() }) t->push();
    for (auto& s : slots) s->repaint();
    tickKnobs();
}
} // namespace ztx
