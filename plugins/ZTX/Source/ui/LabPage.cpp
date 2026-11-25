#include "LabPage.h"

namespace ztx
{
using namespace theme;

static const char* kEngineBlurb[NUM_ENGINES] = {
    "Elastic phase-modulated core + modal body. Stretch, bounce, scrape, snap.",
    "Tempo-locked micro events: clicks, crackles, blips, stutters of its own past.",
    "Grains from a loaded, resampled or built-in buffer. Freeze to hold a texture.",
    "Four operators, eight routings, feedback, inharmonic stretch and drift.",
    "Procedural drums: swept body, transient, material shell, granular rattle.",
};

// ------------------------------------------------------------------------------------
class LabPage::EngineCard final : public juce::Component, public juce::SettableTooltipClient
{
public:
    EngineCard (ZtxProcessor& p, int e) : proc (p), engine (e) { setTooltip (kEngineBlurb[e]); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void mouseDown (const juce::MouseEvent&) override { proc.setParam (labEngine, (float) engine); }
    void paint (juce::Graphics& g) override
    {
        const bool sel = (int) proc.getParam (labEngine) == engine;
        auto r = getLocalBounds().toFloat().reduced (2);
        drawPod (g, r, 40 + engine, sel ? panel2 : panel, sel ? 1.0f : 0.8f);
        if (sel) { g.setColour (acid.withAlpha (0.8f)); g.drawRoundedRectangle (r.reduced (1), 10, 1.5f); }
        auto glyph = r.removeFromLeft (r.getHeight()).reduced (12);
        drawGlyph (g, glyph, sel);
        g.setColour (sel ? text : silver);
        g.setFont (label (13.0f));
        g.drawText (engineShortNames()[engine], r.removeFromTop (r.getHeight() * 0.55f), juce::Justification::bottomLeft);
        g.setColour (textDim);
        g.setFont (mono (10.0f));
        static const char* sub[] = { "elastic / modal", "micro events", "grain cloud", "4-op metal", "procedural" };
        g.drawText (sub[engine], r, juce::Justification::topLeft);
    }
private:
    void drawGlyph (juce::Graphics& g, juce::Rectangle<float> r, bool sel)
    {
        g.setColour (sel ? acid : silverDim);
        juce::Path p;
        const auto c = r.getCentre();
        switch (engine)
        {
            case EngRubber:
                for (int i = 0; i <= 40; ++i)
                {
                    const float t = (float) i / 40.0f;
                    const float y = c.y + std::sin (t * 18.0f) * std::exp (-t * 3.0f) * r.getHeight() * 0.45f;
                    const float x = r.getX() + t * r.getWidth();
                    if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
                }
                g.strokePath (p, juce::PathStrokeType (2.0f));
                break;
            case EngGlitch:
            {
                Rng rr (7);
                for (int i = 0; i < 9; ++i)
                {
                    const float x = r.getX() + (float) i * r.getWidth() / 9.0f;
                    const float h = r.getHeight() * (0.15f + 0.8f * rr.uni());
                    g.fillRect (x, c.y - h * 0.5f + rr.bi() * 4.0f, r.getWidth() / 14.0f, h);
                }
                break;
            }
            case EngGran:
            {
                Rng rr (11);
                for (int i = 0; i < 40; ++i)
                {
                    const float a = rr.uni() * 6.283f, d = std::sqrt (rr.uni()) * r.getWidth() * 0.48f;
                    const float s = 1.5f + rr.uni() * 3.0f;
                    g.fillEllipse (c.x + std::cos (a) * d - s * 0.5f, c.y + std::sin (a) * d - s * 0.5f, s, s);
                }
                break;
            }
            case EngFM:
                for (int i = 1; i <= 4; ++i)
                    g.drawEllipse (juce::Rectangle<float> (r.getWidth() * 0.24f * (float) i, r.getWidth() * 0.24f * (float) i).withCentre (c), 1.6f);
                break;
            default:
                g.drawEllipse (r.reduced (r.getWidth() * 0.12f), 2.0f);
                g.fillEllipse (juce::Rectangle<float> (8, 8).withCentre (c.translated (-r.getWidth() * 0.15f, -r.getHeight() * 0.1f)));
                g.drawEllipse (r.reduced (r.getWidth() * 0.3f), 1.0f);
                break;
        }
    }
    ZtxProcessor& proc;
    int engine;
};

// ------------------------------------------------------------------------------------
class LabPage::SampleView final : public juce::Component, public juce::FileDragAndDropTarget, public juce::SettableTooltipClient
{
public:
    explicit SampleView (ZtxProcessor& p) : proc (p) { setTooltip ("Granular source. Drop an audio file here, LOAD one, or RESAMPLE the plugin's own output."); }
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& f, int, int) override { if (! f.isEmpty()) proc.loadSample (juce::File (f[0])); }
    void tick()
    {
        auto s = proc.getSample();
        if (s.get() != last) { last = s.get(); rebuild (s); }
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (r, 6);
        g.setColour (silver.withAlpha (0.55f));
        g.fillPath (wave, juce::AffineTransform::scale (r.getWidth(), r.getHeight() * 0.5f).translated (0, r.getCentreY()));
        // position + spray window
        const float pos = clamp01 (proc.getParam (grPos) + proc.modDisplay[grPos].load() * 1.0f);
        const float spray = proc.getParam (grSpray);
        const float x = r.getX() + pos * r.getWidth();
        g.setColour (acid.withAlpha (0.13f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x - spray * 0.5f * r.getWidth(), r.getY(), x + spray * 0.5f * r.getWidth(), r.getBottom()));
        g.setColour (proc.getParam (grFreeze) > 0.5f ? orange : acid);
        g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
        g.setColour (text);
        g.setFont (mono (11.0f));
        g.drawText (proc.isCapturing() ? "* RECORDING..." : proc.getSampleName(), r.reduced (8, 4), juce::Justification::topLeft);
    }
private:
    void rebuild (const std::shared_ptr<const SampleData>& s)
    {
        wave.clear();
        if (! s) return;
        const auto& b = s->buffer;
        const int n = b.getNumSamples(), cols = 400;
        wave.startNewSubPath (0, 0);
        std::vector<float> pk ((size_t) cols);
        for (int c = 0; c < cols; ++c)
        {
            const int a = c * n / cols, e = std::min (n, a + n / cols);
            float m = 0;
            for (int i = a; i < e; i += 4) m = std::max (m, std::abs (b.getSample (0, i)));
            pk[(size_t) c] = m;
            wave.lineTo ((float) c / cols, -m);
        }
        for (int c = cols - 1; c >= 0; --c) wave.lineTo ((float) c / cols, pk[(size_t) c]);
        wave.closeSubPath();
    }
    ZtxProcessor& proc;
    const SampleData* last = nullptr;
    juce::Path wave;
};

// ------------------------------------------------------------------------------------
class LabPage::AlgoView final : public juce::Component
{
public:
    explicit AlgoView (ZtxProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override
    {
        static const std::vector<std::pair<int, int>> mods[8] = {
            { { 4, 3 }, { 3, 2 }, { 2, 1 } }, { { 4, 2 }, { 3, 2 }, { 2, 1 } }, { { 4, 3 }, { 3, 1 }, { 2, 1 } }, { { 4, 3 }, { 2, 1 } },
            { { 4, 1 }, { 4, 2 }, { 4, 3 } }, { { 4, 3 }, { 3, 2 } }, {}, { { 2, 1 }, { 3, 1 }, { 4, 1 } } };
        static const std::vector<int> car[8] = { { 1 }, { 1 }, { 1 }, { 1, 3 }, { 1, 2, 3 }, { 1, 2 }, { 1, 2, 3, 4 }, { 1 } };
        const int a = juce::jlimit (0, 7, (int) proc.getParam (fmAlgo));
        int depth[5] = {};
        for (int pass = 0; pass < 4; ++pass)
            for (auto [s, d] : mods[a]) depth[s] = std::max (depth[s], depth[d] + 1);
        int maxD = 0; for (int o = 1; o <= 4; ++o) maxD = std::max (maxD, depth[o]);
        auto r = getLocalBounds().toFloat().reduced (6);
        juce::Point<float> pos[5];
        for (int d = 0; d <= maxD; ++d)
        {
            std::vector<int> ops;
            for (int o = 1; o <= 4; ++o) if (depth[o] == d) ops.push_back (o);
            for (size_t k = 0; k < ops.size(); ++k)
                pos[ops[k]] = { r.getX() + r.getWidth() * ((float) k + 0.5f) / (float) ops.size(),
                                r.getBottom() - 14 - (r.getHeight() - 28) * (maxD ? (float) d / (float) maxD : 0.0f) };
        }
        g.setColour (silver.withAlpha (0.6f));
        for (auto [s, d] : mods[a]) g.drawArrow ({ pos[s], pos[d].translated (0, -11) }, 1.4f, 6, 6);
        g.drawEllipse (juce::Rectangle<float> (16, 16).withCentre (pos[4].translated (14, -10)), 1.2f);  // feedback loop
        for (int o = 1; o <= 4; ++o)
        {
            const bool isCar = std::find (car[a].begin(), car[a].end(), o) != car[a].end();
            auto box = juce::Rectangle<float> (26, 20).withCentre (pos[o]);
            g.setColour (isCar ? acid.withAlpha (0.35f) : panel2);
            g.fillRoundedRectangle (box, 5);
            g.setColour (isCar ? acid : silver);
            g.drawRoundedRectangle (box, 5, 1.2f);
            g.setFont (mono (11.0f, true));
            g.drawText (juce::String (o), box, juce::Justification::centred);
        }
    }
private:
    ZtxProcessor& proc;
};


// ------------------------------------------------------------------------------------
// Touch pad: click = play the engine, X = colour macro, Y = decay macro of the engine.
class LabPage::TouchPad final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit TouchPad (ZtxProcessor& p) : proc (p)
    {
        setTooltip ("TOUCH PAD - click/hold to play the engine. Drag: X = colour (tension / tone / position / FM amount), Y = decay / size");
        setMouseCursor (juce::MouseCursor::CrosshairCursor);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        apply (e.position);
        note = 48;
        proc.keyboardState.noteOn (1, note, 0.6f + 0.4f * (1.0f - e.position.y / (float) getHeight()));
        ripples.push_back ({ e.position, 0.0f });
    }
    void mouseDrag (const juce::MouseEvent& e) override { apply (e.position); trail.push_back (e.position); if (trail.size() > 40) trail.erase (trail.begin()); }
    void mouseUp (const juce::MouseEvent&) override { proc.keyboardState.noteOff (1, note, 0.0f); }
    void tick()
    {
        for (auto& r : ripples) r.second += 0.05f;
        ripples.erase (std::remove_if (ripples.begin(), ripples.end(), [] (auto& r) { return r.second > 1.0f; }), ripples.end());
        if (! isMouseButtonDown() && ! trail.empty()) trail.erase (trail.begin());
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        const int e = (int) proc.getParam (labEngine);
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.32f));
        g.fillRoundedRectangle (r, 8);
        g.setColour (silver.withAlpha (0.07f));
        for (int k = 1; k < 8; ++k) g.drawVerticalLine ((int) (r.getWidth() * (float) k / 8.0f), r.getY() + 4, r.getBottom() - 4);
        for (int k = 1; k < 4; ++k) g.drawHorizontalLine ((int) (r.getHeight() * (float) k / 4.0f), r.getX() + 4, r.getRight() - 4);
        const int cx = engineColourParam (e), dy = engineDecayParam (e);
        const float xn = proc.param (cx)->getValue(), yn = proc.param (dy)->getValue();
        const juce::Point<float> pt (r.getX() + xn * r.getWidth(), r.getBottom() - yn * r.getHeight());
        for (size_t i = 1; i < trail.size(); ++i)
        {
            g.setColour (acid.withAlpha ((float) i / (float) trail.size() * 0.5f));
            g.drawLine ({ trail[i - 1], trail[i] }, 2.0f);
        }
        for (auto& rp : ripples)
        {
            g.setColour (orange.withAlpha (1.0f - rp.second));
            g.drawEllipse (juce::Rectangle<float> (rp.second * 120.0f, rp.second * 120.0f).withCentre (rp.first), 1.5f);
        }
        g.setColour (acid.withAlpha (0.25f));
        g.drawHorizontalLine ((int) pt.y, r.getX() + 4, r.getRight() - 4);
        g.drawVerticalLine ((int) pt.x, r.getY() + 4, r.getBottom() - 4);
        g.setColour (acid);
        g.fillEllipse (juce::Rectangle<float> (12, 12).withCentre (pt));
        g.setColour (acid.withAlpha (0.2f));
        g.fillEllipse (juce::Rectangle<float> (28, 28).withCentre (pt));
        g.setColour (textDim);
        g.setFont (label (10.0f));
        g.drawText ("TOUCH PAD  -  click to play", r.reduced (10, 6), juce::Justification::topLeft);
        g.drawText (juce::String ("X: ") + info (cx).name + "   Y: " + info (dy).name, r.reduced (10, 6), juce::Justification::bottomRight);
    }
private:
    void apply (juce::Point<float> p)
    {
        const int e = (int) proc.getParam (labEngine);
        proc.setParamNorm (engineColourParam (e), clamp01 (p.x / (float) getWidth()));
        proc.setParamNorm (engineDecayParam (e), clamp01 (1.0f - p.y / (float) getHeight()));
    }
    ZtxProcessor& proc;
    int note = 48;
    std::vector<juce::Point<float>> trail;
    std::vector<std::pair<juce::Point<float>, float>> ripples;
};

// ------------------------------------------------------------------------------------
LabPage::LabPage (ZtxProcessor& p)
    : Page (p),
      ampAK (p, ampA, "Attack"), ampDK (p, ampD, "Decay"), ampSK (p, ampS, "Sustain"), ampRK (p, ampR, "Release"),
      velK (p, velSens, "Vel"), pitchK (p, labPitch, "Pitch"), gateC (p, gateMode, "Gate"),
      scope (p), keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    for (int e = 0; e < NUM_ENGINES; ++e)
    {
        cards.push_back (std::make_unique<EngineCard> (p, e));
        addAndMakeVisible (*cards.back());
    }
    const std::vector<std::vector<int>> lists = {
        { rbElastic, rbTension, rbDamping, rbResonance, rbImpact, rbFriction, rbMaterial, rbDecay, rbDrive, rbPitch },
        { glDensity, glGrain, glProb, glTiming, glRandom, glRepeat, glBuffer, glBits, glRate, glTone, glDecay, glPitch },
        { grPos, grScan, grSpray, grSize, grDensity, grEnv, grPitch, grPitchRnd, grReverse, grSpread },
        { fmRatio1, fmRatio2, fmRatio3, fmRatio4, fmAmount, fmFeedback, fmIdx2, fmIdx3, fmIdx4, fmDecay, fmModDecay, fmDrive,
          fmPitchEnv, fmPitchDec, fmInharm, fmInstab, fmPitch },
        { pcPitch, pcDecay, pcBody, pcSweep, pcTransient, pcResonance, pcMaterial, pcNoise, pcTone, pcRattle },
    };
    for (int e = 0; e < NUM_ENGINES; ++e)
        for (int pid : lists[(size_t) e])
        {
            juce::String nm = info (pid).name;
            nm = nm.replace ("Rubber ", "").replace ("Glitch ", "").replace ("Gran ", "").replace ("FM ", "").replace ("Perc ", "");
            engineKnobs[e].push_back (std::make_unique<Knob> (p, pid, nm));
            addKnob (*engineKnobs[e].back());
        }
    freeze = std::make_unique<ParamToggle> (p, grFreeze, "FREEZE", true);
    addAndMakeVisible (*freeze);
    algo = std::make_unique<ParamChoice> (p, fmAlgo, "Algorithm");
    addAndMakeVisible (*algo);
    sampleView = std::make_unique<SampleView> (p);
    addAndMakeVisible (*sampleView);
    algoView = std::make_unique<AlgoView> (p);
    addAndMakeVisible (*algoView);
    pad = std::make_unique<TouchPad> (p);
    addAndMakeVisible (*pad);
    for (auto* b : { &loadBtn, &capOutBtn, &capInBtn, &defaultBtn }) addAndMakeVisible (*b);
    loadBtn.setTooltip ("Load WAV / AIFF / FLAC / MP3 (up to 30 s) into the granular buffer");
    capOutBtn.setTooltip ("Record 4 s of ZTX's own voices into the grain buffer (resampling)");
    capInBtn.setTooltip ("Record 4 s of the plugin's audio input / sidechain into the grain buffer");
    defaultBtn.setTooltip ("Back to the built-in organic debris buffer");
    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load audio into Granular Organism", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) proc.loadSample (fc.getResult()); });
    };
    capOutBtn.onClick = [this] { proc.startCapture (false); };
    capInBtn.onClick = [this] { proc.startCapture (true); };
    defaultBtn.onClick = [this] { proc.useDefaultSample(); };

    for (auto* k : { &ampAK, &ampDK, &ampSK, &ampRK, &velK, &pitchK }) addKnob (*k);
    addAndMakeVisible (gateC);
    addAndMakeVisible (scope);
    keyboard.setAvailableRange (24, 96);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (16.0f);
    addAndMakeVisible (keyboard);
    showEngine ((int) p.getParam (labEngine));
}

LabPage::~LabPage() = default;

void LabPage::showEngine (int e)
{
    shownEngine = e;
    for (int k = 0; k < NUM_ENGINES; ++k)
        for (auto& kn : engineKnobs[k]) kn->setVisible (k == e);
    const bool gran = e == EngGran, fm = e == EngFM;
    freeze->setVisible (gran); sampleView->setVisible (gran);
    for (auto* b : { &loadBtn, &capOutBtn, &capInBtn, &defaultBtn }) b->setVisible (gran);
    algo->setVisible (fm); algoView->setVisible (fm);
    for (auto& c : cards) c->repaint();
    resized();
    repaint();
}

void LabPage::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    keyboard.setBounds (r.removeFromBottom (58));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 43.2f);
    r.removeFromBottom (8);

    cardsPod = r.removeFromLeft (200);
    {
        auto c = cardsPod.reduced (0, 0);
        const int h = c.getHeight() / NUM_ENGINES;
        for (auto& card : cards) card->setBounds (c.removeFromTop (h));
    }
    r.removeFromLeft (10);
    auto right = r.removeFromRight (310);
    r.removeFromRight (10);
    enginePod = r;

    ampPod = right.removeFromTop (230);
    right.removeFromTop (10);
    scopePod = right;
    {
        auto a = ampPod.reduced (14, 12);
        a.removeFromTop (22);
        auto row1 = a.removeFromTop (88);
        layoutRow (row1, { &ampAK, &ampDK, &ampSK, &ampRK });
        a.removeFromTop (6);
        auto row2 = a.removeFromTop (88);
        gateC.setBounds (row2.removeFromLeft (110).withSizeKeepingCentre (110, 40));
        row2.removeFromLeft (10);
        layoutRow (row2, { &velK, &pitchK });
    }
    scope.setBounds (scopePod.reduced (12).withTrimmedTop (18));

    auto e = enginePod.reduced (16, 12);
    e.removeFromTop (58);
    if (shownEngine < 0) return;
    auto& ks = engineKnobs[shownEngine];
    auto place = [] (juce::Rectangle<int> area, std::vector<std::unique_ptr<Knob>>& v, int cols)
    {
        const int count = (int) v.size();
        const int rows = (count + cols - 1) / cols;
        const int h = area.getHeight() / std::max (1, rows);
        const int w = area.getWidth() / cols;
        for (int i = 0; i < count; ++i)
            v[(size_t) i]->setBounds (juce::Rectangle<int> (area.getX() + (i % cols) * w, area.getY() + (i / cols) * h, w, h).reduced (5));
    };
    if (shownEngine == EngGran)
    {
        auto top = e.removeFromTop (120);
        auto btns = top.removeFromRight (130);
        sampleView->setBounds (top);
        for (auto* b : { &loadBtn, &capOutBtn, &capInBtn, &defaultBtn }) { b->setBounds (btns.removeFromTop (28).reduced (6, 2)); }
        e.removeFromTop (8);
        auto fr = e.removeFromBottom (34);
        freeze->setBounds (fr.removeFromLeft (120).reduced (4));
        e.removeFromBottom (4);
        place (e.removeFromTop (2 * 112), ks, 5);
        e.removeFromTop (6);
        pad->setBounds (e);
    }
    else if (shownEngine == EngFM)
    {
        auto left = e.removeFromLeft (170);
        algo->setBounds (left.removeFromTop (44).reduced (4));
        algoView->setBounds (left.removeFromTop (230).reduced (4, 10));
        pad->setBounds (e.removeFromBottom (std::max (60, e.getHeight() - 3 * 112 - 6)));
        e.removeFromBottom (6);
        place (e, ks, 6);
    }
    else
    {
        place (e.removeFromTop (2 * 124), ks, shownEngine == EngGlitch ? 6 : 5);
        e.removeFromTop (8);
        pad->setBounds (e);
    }
}

void LabPage::paint (juce::Graphics& g)
{
    drawPod (g, enginePod.toFloat(), 3);
    drawPod (g, ampPod.toFloat(), 5);
    drawPod (g, scopePod.toFloat(), 6, juce::Colour (0xff0d0e0b));
    if (shownEngine >= 0)
    {
        auto t = enginePod.reduced (18, 12).removeFromTop (54);
        drawGlitchText (g, engineNames()[shownEngine].toUpperCase(), t.removeFromTop (30).toFloat(), mono (24.0f, true), shownEngine + 3);
        g.setColour (textDim);
        g.setFont (mono (11.5f));
        g.drawText (kEngineBlurb[shownEngine], t, juce::Justification::topLeft);
    }
    g.setColour (silver);
    g.setFont (label (11.0f));
    g.drawText ("AMP / VOICE", ampPod.reduced (16, 10).removeFromTop (16), juce::Justification::left);
    g.drawText ("OUTPUT  -  LIVE WAVEFORM", scopePod.reduced (14, 6).removeFromTop (14), juce::Justification::left);
}

void LabPage::tick()
{
    const int e = (int) proc.getParam (labEngine);
    if (e != shownEngine) showEngine (e);
    if (shownEngine == EngGran) sampleView->tick();
    if (shownEngine == EngFM) algoView->repaint();
    scope.tick();
    pad->tick();
    tickKnobs();
}
} // namespace ztx
