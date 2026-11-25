#include "PluginEditor.h"
#include "ui/LabPage.h"
#include "ui/SeqPage.h"
#include "ui/ModPage.h"
#include "ui/FxPage.h"
#include "ui/PresetPage.h"
#include "presets/Presets.h"
#include "Randomizer.h"

namespace ztx
{
using namespace theme;

// =====================================================================================
//  ROOT - laid out at a fixed base size and scaled as a whole (resizable UI)
// =====================================================================================
class ZtxEditor::Root final : public juce::Component
{
public:
    explicit Root (ZtxProcessor& p)
        : proc (p), lab (p), seq (p), mod (p), fx (p), presets (p),
          rndAmtK (p, rndAmount, "Amount", true), evoDepthK (p, evolveAmt, "Depth", true), evoRateK (p, evolveRate, "Speed", true),
          masterK (p, masterVol, "Master", true), evoT (p, evolveOn, "EVOLVE", true), evoBarsC (p, evolveBars)
    {
        const char* tabNames[] = { "LAB", "SEQUENCER", "MODULATION", "DESTRUCTION", "PRESETS" };
        for (int i = 0; i < 5; ++i)
        {
            tabs[i].setButtonText (tabNames[i]);
            tabs[i].setRadioGroupId (1);
            tabs[i].setClickingTogglesState (true);
            tabs[i].onClick = [this, i] { showPage (i); };
            addAndMakeVisible (tabs[i]);
        }
        pages = { &lab, &seq, &mod, &fx, &presets };
        for (auto* pg : pages) addChildComponent (pg);

        const char* rndNames[] = { "SOUND", "RHYTHM", "MOD", "FX", "ALL" };
        const int lockIds[] = { lockSound, lockRhythm, lockMod, lockFx };
        for (int i = 0; i < 5; ++i)
        {
            rndBtn[i].setButtonText (rndNames[i]);
            rndBtn[i].setTooltip (juce::String ("Randomize ") + rndNames[i] + " (respects the LOCK toggles)");
            rndBtn[i].onClick = [this, i] { randomize (proc, (RandTarget) i); };
            addAndMakeVisible (rndBtn[i]);
            if (i < 4)
            {
                locks[i] = std::make_unique<ParamToggle> (p, lockIds[i], "L", true);
                locks[i]->getProperties().remove ("led");
                addAndMakeVisible (*locks[i]);
            }
        }
        rndBtn[4].getProperties().set ("accent", true);
        for (auto* k : { &rndAmtK, &evoDepthK, &evoRateK, &masterK }) { addAndMakeVisible (*k); headerKnobs.push_back (k); }
        addAndMakeVisible (evoT);
        evoT.setTooltip ("Exploration mode: the sound keeps drifting around its identity; the pattern mutates gently every N bars");
        addAndMakeVisible (evoBarsC);

        prevBtn.setButtonText ("<"); nextBtn.setButtonText (">");
        prevBtn.onClick = [this] { stepPreset (-1); };
        nextBtn.onClick = [this] { stepPreset (1); };
        addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);
        presets.onPresetLoaded = [this] { repaint(); };

        showPage (0);
    }

    void showPage (int i)
    {
        current = i;
        for (int k = 0; k < 5; ++k)
        {
            pages[(size_t) k]->setVisible (k == i);
            tabs[k].setToggleState (k == i, juce::dontSendNotification);
        }
        if (i == 4) presets.refresh();
    }

    void stepPreset (int d)
    {
        const int n = (int) factoryPresets().size();
        const int idx = proc.presetIndex < 0 ? (d > 0 ? 0 : n - 1) : (proc.presetIndex + d + n) % n;
        applyFactoryPreset (proc, idx);
        repaint();
    }

    void resized() override
    {
        background = makeBackground (getWidth(), getHeight());
        auto r = getLocalBounds();
        header = r.removeFromTop (104);
        auto h = header.reduced (12, 8);
        logo = h.removeFromLeft (250);
        auto row1 = h.removeFromTop (40);
        auto row2 = h.withTrimmedTop (6);
        for (int i = 0; i < 5; ++i) tabs[i].setBounds (row1.removeFromLeft (118).reduced (3, 4));
        row1.removeFromLeft (12);
        masterK.setBounds (row1.removeFromRight (56).withHeight (54));
        meterArea = row1.removeFromRight (16).reduced (2, 4);
        row1.removeFromRight (10);
        nextBtn.setBounds (row1.removeFromRight (30).reduced (2, 6));
        presetArea = row1.removeFromRight (230);
        prevBtn.setBounds (row1.removeFromRight (30).reduced (2, 6));

        rndLabel = row2.removeFromLeft (44);
        for (int i = 0; i < 5; ++i)
        {
            auto cell = row2.removeFromLeft (i < 4 ? 98 : 64);
            if (i < 4) locks[i]->setBounds (cell.removeFromRight (26).reduced (2, 8));
            rndBtn[i].setBounds (cell.reduced (2, 7));
        }
        rndAmtK.setBounds (row2.removeFromLeft (52));
        row2.removeFromLeft (16);
        evoT.setBounds (row2.removeFromLeft (96).reduced (2, 7));
        evoDepthK.setBounds (row2.removeFromLeft (50));
        evoRateK.setBounds (row2.removeFromLeft (50));
        evoBarsC.setBounds (row2.removeFromLeft (92).reduced (2, 10));
        statusArea = row2;

        for (auto* pg : pages) pg->setBounds (r);
    }

    void paint (juce::Graphics& g) override
    {
        g.drawImageAt (background, 0, 0);
        // header strip
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRect (header);
        g.setColour (acid.withAlpha (0.25f));
        g.drawHorizontalLine (header.getBottom() - 1, 0, (float) getWidth());
        auto l = logo.toFloat();
        drawGlitchText (g, "ZTX", l.removeFromTop (52), mono (46.0f, true), 1 + glitchSeed, juce::Justification::centredLeft, 1.4f);
        g.setColour (silver);
        g.setFont (label (13.0f));
        g.drawText ("ORGANIC  GLITCH  LAB", l.removeFromTop (18), juce::Justification::centredLeft);
        g.setColour (textDim);
        g.setFont (mono (9.5f));
        g.drawText ("experimental sound laboratory  v" JucePlugin_VersionString, l.removeFromTop (14), juce::Justification::centredLeft);

        // preset display
        auto pa = presetArea.toFloat().reduced (2, 4);
        g.setColour (panel2); g.fillRoundedRectangle (pa, 6);
        g.setColour (silverDim); g.drawRoundedRectangle (pa, 6, 1);
        g.setColour (text); g.setFont (mono (13.0f, true));
        g.drawText (proc.currentPresetName, pa.reduced (10, 0), juce::Justification::centred);

        g.setColour (silver); g.setFont (label (11.0f));
        g.drawText ("RND", rndLabel, juce::Justification::centredLeft);

        // output meter
        const float lvl = clamp01 (proc.outLevel.load());
        auto m = meterArea.toFloat();
        g.setColour (panel2); g.fillRoundedRectangle (m, 3);
        g.setColour (lvl > 0.85f ? orange : acid);
        g.fillRoundedRectangle (m.withTop (m.getBottom() - lvl * m.getHeight()), 3);

        // status
        g.setFont (mono (10.0f));
        auto sa = statusArea.reduced (4, 6);
        g.setColour (proc.seqRunning.load() ? acid : textDim);
        g.drawText (proc.seqRunning.load() ? "SEQ RUN" : "SEQ IDLE", sa.removeFromTop (sa.getHeight() / 2), juce::Justification::centredRight);
        g.setColour (proc.frozen ? orange : textDim);
        g.drawText (juce::String (proc.activeVoices.load()) + (proc.frozen ? " v  FROZEN" : " voices"), sa, juce::Justification::centredRight);
    }

    void tick()
    {
        pages[(size_t) current]->tick();
        for (auto* k : headerKnobs) k->updateMod();
        if (++frame % 47 == 0) glitchSeed = (glitchSeed + 1) % 5;
        repaint (header);
    }

private:
    ZtxProcessor& proc;
    LabPage lab; SeqPage seq; ModPage mod; FxPage fx; PresetPage presets;
    std::vector<Page*> pages;
    juce::TextButton tabs[5], rndBtn[5], prevBtn, nextBtn;
    std::unique_ptr<ParamToggle> locks[4];
    Knob rndAmtK, evoDepthK, evoRateK, masterK;
    std::vector<Knob*> headerKnobs;
    ParamToggle evoT;
    ParamChoice evoBarsC;
    juce::Image background;
    juce::Rectangle<int> header, logo, presetArea, meterArea, rndLabel, statusArea;
    int current = 0, frame = 0, glitchSeed = 0;
};

// =====================================================================================
ZtxEditor::ZtxEditor (ZtxProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
    root = std::make_unique<Root> (p);
    addAndMakeVisible (*root);
    root->setBounds (0, 0, kBaseW, kBaseH);
    setResizable (true, true);
    setResizeLimits (kBaseW * 3 / 4, kBaseH * 3 / 4, kBaseW * 2, kBaseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kBaseW / kBaseH);
    setSize (kBaseW, kBaseH);
    startTimerHz (30);
}

ZtxEditor::~ZtxEditor()
{
    stopTimer();
    root = nullptr;
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void ZtxEditor::paint (juce::Graphics& g) { g.fillAll (theme::bg); }

void ZtxEditor::resized()
{
    if (root == nullptr) return;
    const float s = std::min ((float) getWidth() / kBaseW, (float) getHeight() / kBaseH);
    root->setTransform (juce::AffineTransform::scale (s));
}

void ZtxEditor::timerCallback() { root->tick(); }
} // namespace ztx
