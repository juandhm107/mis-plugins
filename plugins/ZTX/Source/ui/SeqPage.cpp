#include "SeqPage.h"

namespace ztx
{
using namespace theme;

namespace
{
const juce::Colour kTrackCol[kTracks] = { juce::Colour (0xff9dbf3a), juce::Colour (0xffc8742c), juce::Colour (0xffb8bab2), juce::Colour (0xff6f9a74) };
const int kDivChoices[] = { 4, 5, 6, 7, 9, 10, 11, 12 };   // indices into ZTX_DIVS

class MiniKnob final : public juce::Component
{
public:
    MiniKnob (const juce::String& cap, double lo, double hi, const juce::String& tip) : caption (cap)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setRange (lo, hi, 0.0);
        s.setPopupDisplayEnabled (true, true, nullptr);
        s.setTooltip (tip);
        addAndMakeVisible (s);
    }
    void resized() override { auto r = getLocalBounds(); r.removeFromBottom (11); s.setBounds (r); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (textDim); g.setFont (label (8.5f));
        g.drawText (caption, getLocalBounds().removeFromBottom (11), juce::Justification::centred);
    }
    juce::Slider s;
    juce::String caption;
};
} // namespace

// =====================================================================================
class SeqPage::StepGrid final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid (ZtxProcessor& p) : proc (p)
    {
        setTooltip ("Click/drag to edit in the selected mode. Right-click a step for options. Alt-click resets a value.");
    }
    EditMode mode = EdGate;

    int columns() const
    {
        int c = proc.getParam (seqSteps) > 0.5f ? 32 : 16;
        for (auto& t : proc.pattern().tracks) c = std::max (c, t.length);
        return c;
    }
    juce::Rectangle<float> rowArea (int t) const
    {
        const float h = (float) getHeight() / kTracks;
        return { 0, h * (float) t, (float) getWidth(), h };
    }
    juce::Rectangle<float> cell (int t, int i) const
    {
        auto r = rowArea (t).reduced (0, 6);
        const float w = r.getWidth() / (float) columns();
        return { r.getX() + w * (float) i, r.getY(), w, r.getHeight() };
    }

    void paint (juce::Graphics& g) override
    {
        const auto& pat = proc.pattern();
        const int cols = columns();
        const bool running = proc.seqRunning.load();
        for (int t = 0; t < kTracks; ++t)
        {
            const Track& tr = pat.tracks[(size_t) t];
            const int play = proc.playStep[t].load();
            for (int i = 0; i < cols; ++i)
            {
                auto c = cell (t, i).reduced (1.5f, 0);
                const bool inLen = i < tr.length;
                const auto& st = tr.steps[(size_t) i];
                g.setColour ((i / 4) % 2 ? panel2 : panel2.brighter (0.05f));
                g.fillRoundedRectangle (c, 3);
                if (! inLen) { g.setColour (bg.withAlpha (0.7f)); g.fillRoundedRectangle (c, 3); continue; }
                const auto col = tr.mute ? silverDim : kTrackCol[t];
                if (st.on)
                {
                    const float hh = c.getHeight() * (0.25f + 0.75f * st.vel);
                    g.setColour (col.withAlpha (0.3f + 0.7f * st.prob));
                    g.fillRoundedRectangle (c.withTop (c.getBottom() - hh), 3);
                    if (st.ratchet > 1)
                    {
                        g.setColour (bg.withAlpha (0.8f));
                        for (int k = 1; k < st.ratchet; ++k)
                            g.fillRect (c.getX() + c.getWidth() * (float) k / (float) st.ratchet - 0.75f, c.getBottom() - hh, 1.5f, hh);
                    }
                    if (st.prob < 0.999f)
                    {
                        g.setColour (text.withAlpha (0.7f));
                        g.setFont (mono (8.5f));
                        g.drawText (juce::String ((int) std::round (st.prob * 100)), c.withHeight (11).translated (0, 1), juce::Justification::centred);
                    }
                    if (st.pitch != 0)
                    {
                        g.setColour (text);
                        g.setFont (mono (9.0f, true));
                        g.drawText ((st.pitch > 0 ? "+" : "") + juce::String (st.pitch), c.withTrimmedTop (c.getHeight() * 0.35f).withHeight (12), juce::Justification::centred);
                    }
                    if (st.micro > 0.01f)
                    {
                        g.setColour (orange);
                        g.fillRect (c.getX() + st.micro * c.getWidth(), c.getBottom() - 3, 3.0f, 3.0f);
                    }
                }
                if (mode == EdVel || mode == EdProb || mode == EdMicro)
                {
                    const float v = mode == EdVel ? st.vel : mode == EdProb ? st.prob : st.micro * 2.0f;
                    g.setColour (text.withAlpha (0.55f));
                    g.drawHorizontalLine ((int) (c.getBottom() - v * c.getHeight()), c.getX() + 2, c.getRight() - 2);
                }
                if (running && i == play)
                {
                    g.setColour (text.withAlpha (0.75f));
                    g.drawRoundedRectangle (c.expanded (0.5f, 1.5f), 3, 1.4f);
                }
            }
            const float flash = proc.trackFlash[t].load();
            if (flash > 0.02f)
            {
                g.setColour (kTrackCol[t].withAlpha (flash * 0.5f));
                g.fillRect (rowArea (t).withWidth (3).reduced (0, 8));
            }
        }
    }

    bool locate (juce::Point<float> p, int& t, int& i) const
    {
        t = juce::jlimit (0, kTracks - 1, (int) (p.y / ((float) getHeight() / kTracks)));
        const auto r = rowArea (t);
        i = (int) ((p.x - r.getX()) / (r.getWidth() / (float) columns()));
        return i >= 0 && i < proc.pattern().tracks[(size_t) t].length;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        int t, i;
        if (! locate (e.position, t, i)) return;
        auto& tr = proc.pattern().tracks[(size_t) t];
        auto& st = tr.steps[(size_t) i];
        proc.pushUndo();
        dragTrack = t;
        if (e.mods.isPopupMenu()) { showMenu (t, i); return; }
        if (e.mods.isAltDown())
        {
            if (mode == EdVel) st.vel = 0.8f; else if (mode == EdProb) st.prob = 1; else if (mode == EdPitch) st.pitch = 0;
            else if (mode == EdMicro) st.micro = 0; else if (mode == EdRatchet) st.ratchet = 1;
            changed(); return;
        }
        switch (mode)
        {
            case EdGate: paintState = ! st.on; st.on = paintState; break;
            case EdRatchet: st.ratchet = st.ratchet % 4 + 1; if (! st.on) st.on = true; break;
            case EdPitch: dragStartY = e.position.y; dragStartPitch = st.pitch; dragStep = i; break;
            default: applyValue (t, i, e.position.y); break;
        }
        changed();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragTrack < 0 || e.mods.isPopupMenu()) return;
        int t, i;
        const bool ok = locate ({ e.position.x, rowArea (dragTrack).getCentreY() }, t, i);
        if (! ok) return;
        auto& st = proc.pattern().tracks[(size_t) dragTrack].steps[(size_t) i];
        switch (mode)
        {
            case EdGate: if (st.on != paintState) { st.on = paintState; changed(); } break;
            case EdPitch:
            {
                auto& ps = proc.pattern().tracks[(size_t) dragTrack].steps[(size_t) dragStep];
                const int np = juce::jlimit (-24, 24, dragStartPitch + (int) ((dragStartY - e.position.y) / 5.0f));
                if (np != ps.pitch) { ps.pitch = np; changed(); }
                break;
            }
            case EdRatchet: break;
            default: applyValue (dragTrack, i, e.position.y); changed(); break;
        }
    }
    void mouseUp (const juce::MouseEvent&) override { dragTrack = -1; }

private:
    void applyValue (int t, int i, float y)
    {
        auto c = cell (t, i);
        const float v = clamp01 ((c.getBottom() - y) / c.getHeight());
        auto& st = proc.pattern().tracks[(size_t) t].steps[(size_t) i];
        if (mode == EdVel) st.vel = std::max (0.05f, v);
        else if (mode == EdProb) st.prob = std::max (0.05f, v);
        else if (mode == EdMicro) st.micro = v * 0.5f;
    }
    void changed() { proc.commitPattern(); repaint(); }
    void showMenu (int t, int i)
    {
        juce::PopupMenu m;
        auto& st = proc.pattern().tracks[(size_t) t].steps[(size_t) i];
        m.addItem (1, st.on ? "Step off" : "Step on");
        juce::PopupMenu rat, prob;
        for (int k = 1; k <= 4; ++k) rat.addItem (10 + k, juce::String (k) + "x", true, st.ratchet == k);
        for (int k : { 100, 75, 50, 25, 10 }) prob.addItem (100 + k, juce::String (k) + " %", true, std::abs (st.prob * 100 - (float) k) < 1);
        m.addSubMenu ("Ratchet", rat);
        m.addSubMenu ("Probability", prob);
        m.addItem (2, "Reset pitch / micro");
        m.addSeparator();
        m.addItem (3, "Clear track");
        m.addItem (4, "Shift track left");
        m.addItem (5, "Shift track right");
        m.showMenuAsync (juce::PopupMenu::Options(), [this, t, i] (int r)
        {
            auto& tr = proc.pattern().tracks[(size_t) t];
            auto& s = tr.steps[(size_t) i];
            if (r == 1) s.on = ! s.on;
            else if (r > 10 && r <= 14) { s.ratchet = r - 10; s.on = true; }
            else if (r >= 100) s.prob = (float) (r - 100) / 100.0f;
            else if (r == 2) { s.pitch = 0; s.micro = 0; }
            else if (r == 3) clearTrack (tr);
            else if (r == 4) std::rotate (tr.steps.begin(), tr.steps.begin() + 1, tr.steps.begin() + tr.length);
            else if (r == 5) std::rotate (tr.steps.begin(), tr.steps.begin() + tr.length - 1, tr.steps.begin() + tr.length);
            else return;
            changed();
        });
    }

    ZtxProcessor& proc;
    bool paintState = true;
    int dragTrack = -1, dragStep = 0, dragStartPitch = 0;
    float dragStartY = 0;
};

// =====================================================================================
class SeqPage::TrackStrip final : public juce::Component
{
public:
    TrackStrip (ZtxProcessor& p, int index)
        : proc (p), t (index),
          note (0, 127), len (1, 32), hits (0, 32), rot (0, 31),
          level ("LVL", 0, 1, "Track level"), pan ("PAN", -1, 1, "Track pan"), gate ("GATE", 0.05, 1, "Gate length (fraction of a step) - matters for Granular / Gated mode"),
          decay ("DCY", -1, 1, "Offsets this engine's decay/size macro for this track only"),
          colour ("CLR", -1, 1, "Offsets this engine's colour macro (tension / tone / position / FM amount) for this track only")
    {
        engine.addItemList (engineNames(), 1);
        engine.setTooltip ("Engine this track triggers");
        for (int d : kDivChoices) div.addItem (info (lfo1Div).choices[d], d + 1);
        div.setTooltip ("Step division - different divisions per track = polyrhythm");
        note.textFromValueFunction = [] (double v) { return noteName ((int) v); };
        len.textFromValueFunction = [] (double v) { return "LEN " + juce::String ((int) v); };
        hits.textFromValueFunction = [] (double v) { return "HIT " + juce::String ((int) v); };
        rot.textFromValueFunction = [] (double v) { return "ROT " + juce::String ((int) v); };
        for (auto* nb : { &note, &len, &hits, &rot }) nb->updateText();
        note.setTooltip ("Track note (drag)");
        len.setTooltip ("Track length - different lengths per track = polymeter");
        hits.setTooltip ("Euclidean hits"); rot.setTooltip ("Euclidean rotation");
        euc.setButtonText ("E"); euc.setTooltip ("Apply Euclidean rhythm (hits / rotation) to this track");
        mute.setButtonText ("M"); mute.setClickingTogglesState (true); mute.setTooltip ("Mute track");
        lock.setButtonText ("L"); lock.setClickingTogglesState (true); lock.setTooltip ("Lock: GENERATE / MUTATE leave this track alone");
        lock.getProperties().set ("accent", true);
        for (juce::Component* c : std::initializer_list<juce::Component*> { &engine, &div, &note, &len, &hits, &rot, &euc, &mute, &lock, &level, &pan, &gate, &decay, &colour })
            addAndMakeVisible (*c);

        engine.onChange = [this] { edit ([this] (Track& tr) { tr.engine = engine.getSelectedId() - 1; }); };
        div.onChange = [this] { edit ([this] (Track& tr) { tr.div = div.getSelectedId() - 1; }); };
        note.onValueChange = [this] { edit ([this] (Track& tr) { tr.note = (int) note.getValue(); }); };
        len.onValueChange = [this] { edit ([this] (Track& tr) { tr.length = (int) len.getValue(); }); };
        euc.onClick = [this] { proc.pushUndo(); edit ([this] (Track& tr) { applyEuclid (tr, (int) hits.getValue(), (int) rot.getValue()); }); };
        hits.onValueChange = [this] { edit ([this] (Track& tr) { tr.euclidHits = (int) hits.getValue(); }); };
        rot.onValueChange = [this] { edit ([this] (Track& tr) { tr.euclidRot = (int) rot.getValue(); }); };
        mute.onClick = [this] { edit ([this] (Track& tr) { tr.mute = mute.getToggleState(); }); };
        lock.onClick = [this] { edit ([this] (Track& tr) { tr.lock = lock.getToggleState(); }); };
        level.s.onValueChange = [this] { edit ([this] (Track& tr) { tr.level = (float) level.s.getValue(); }); };
        pan.s.onValueChange = [this] { edit ([this] (Track& tr) { tr.pan = (float) pan.s.getValue(); }); };
        gate.s.onValueChange = [this] { edit ([this] (Track& tr) { tr.gate = (float) gate.s.getValue(); }); };
        decay.s.onValueChange = [this] { edit ([this] (Track& tr) { tr.decayMod = (float) decay.s.getValue(); }); };
        colour.s.onValueChange = [this] { edit ([this] (Track& tr) { tr.colourMod = (float) colour.s.getValue(); }); };
    }

    void sync()
    {
        syncing = true;
        const Track& tr = proc.pattern().tracks[(size_t) t];
        const auto n = juce::dontSendNotification;
        engine.setSelectedId (tr.engine + 1, n);
        div.setSelectedId (tr.div + 1, n);
        note.setValue (tr.note, n); len.setValue (tr.length, n);
        hits.setValue (tr.euclidHits, n); rot.setValue (tr.euclidRot, n);
        mute.setToggleState (tr.mute, n); lock.setToggleState (tr.lock, n);
        level.s.setValue (tr.level, n); pan.s.setValue (tr.pan, n); gate.s.setValue (tr.gate, n);
        decay.s.setValue (tr.decayMod, n); colour.s.setValue (tr.colourMod, n);
        syncing = false;
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 4);
        r.removeFromLeft (16);
        auto top = r.removeFromTop (22);
        engine.setBounds (top.removeFromLeft (128)); top.removeFromLeft (4);
        div.setBounds (top.removeFromLeft (62)); top.removeFromLeft (4);
        mute.setBounds (top.removeFromLeft (26)); top.removeFromLeft (2);
        lock.setBounds (top.removeFromLeft (26)); top.removeFromLeft (6);
        euc.setBounds (top.removeFromRight (24));
        r.removeFromTop (4);
        auto bottom = r;
        auto nums = bottom.removeFromLeft (150);
        const int w = nums.getWidth() / 2;
        auto nrow1 = nums.removeFromTop (nums.getHeight() / 2);
        note.setBounds (nrow1.removeFromLeft (w).reduced (2)); len.setBounds (nrow1.reduced (2));
        hits.setBounds (nums.removeFromLeft (w).reduced (2)); rot.setBounds (nums.reduced (2));
        layoutRow (bottom, { &level, &pan, &gate, &decay, &colour }, 2);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2);
        g.setColour (kTrackCol[t]);
        g.fillRoundedRectangle (r.removeFromLeft (8).reduced (0, 6), 3);
    }

private:
    template <typename F> void edit (F&& f)
    {
        if (syncing) return;
        f (proc.pattern().tracks[(size_t) t]);
        proc.commitPattern();
    }
    ZtxProcessor& proc;
    int t;
    bool syncing = false;
    juce::ComboBox engine, div;
    NumBox note, len, hits, rot;
    juce::TextButton euc, mute, lock;
    MiniKnob level, pan, gate, decay, colour;
};

// =====================================================================================
class SeqPage::LaneEditor final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit LaneEditor (ZtxProcessor& p) : proc (p) { setTooltip ("Step-lane: a 16-step modulation source ('Step Lane' in the matrix). Drag to draw."); }
    void paint (juce::Graphics& g) override
    {
        const auto& lane = proc.pattern().lane;
        const float w = (float) getWidth() / kLaneSteps;
        const int cur = proc.laneStepDisplay.load();
        for (int i = 0; i < kLaneSteps; ++i)
        {
            auto c = juce::Rectangle<float> (w * (float) i, 0, w, (float) getHeight()).reduced (2, 0);
            g.setColour (panel2); g.fillRoundedRectangle (c, 2);
            g.setColour ((i == cur && proc.seqRunning.load() ? orange : acidDim).withAlpha (0.9f));
            g.fillRoundedRectangle (c.withTop (c.getBottom() - lane[(size_t) i] * c.getHeight()), 2);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override { proc.pushUndo(); mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, kLaneSteps - 1, (int) (e.position.x / ((float) getWidth() / kLaneSteps)));
        proc.pattern().lane[(size_t) i] = clamp01 (1.0f - e.position.y / (float) getHeight());
        proc.commitPattern();
        repaint();
    }
private:
    ZtxProcessor& proc;
};

// =====================================================================================
SeqPage::SeqPage (ZtxProcessor& p)
    : Page (p),
      densK (p, seqDensity, "Density"), compK (p, seqComplexity, "Complexity"), chaosK (p, seqChaos, "Chaos"),
      grooveK (p, seqGroove, "Groove"), repK (p, seqRepeat, "Repetition"), swingK (p, seqSwing, "Swing"),
      humanK (p, seqHuman, "Humanize"), probK (p, seqProb, "Probability"), bpmK (p, seqBpm, "Int. BPM"),
      stepsC (p, seqSteps, "Steps"), midiC (p, seqMidi, "MIDI Mode"), runT (p, seqRun, "RUN")
{
    grid = std::make_unique<StepGrid> (p);
    addAndMakeVisible (*grid);
    for (int t = 0; t < kTracks; ++t)
    {
        strips.push_back (std::make_unique<TrackStrip> (p, t));
        addAndMakeVisible (*strips.back());
    }
    lane = std::make_unique<LaneEditor> (p);
    addAndMakeVisible (*lane);
    for (auto* k : { &densK, &compK, &chaosK, &grooveK, &repK, &swingK, &humanK, &probK, &bpmK }) addKnob (*k);
    addAndMakeVisible (stepsC); addAndMakeVisible (midiC); addAndMakeVisible (runT);
    runT.setTooltip ("Run the sequencer on its own clock when the host transport is stopped (always follows the host when it plays)");

    genBtn.getProperties().set ("accent", true);
    genBtn.setTooltip ("Create a new pattern from Density / Complexity / Chaos / Groove / Repetition (locked tracks are kept)");
    mutBtn.setTooltip ("Modify the pattern a little without destroying its identity");
    freezeBtn.setTooltip ("Freeze the current result: GENERATE, MUTATE, RANDOMIZE RHYTHM and EVOLVE can't change it");
    undoBtn.setTooltip ("Undo the last pattern edit");
    clearBtn.setTooltip ("Clear all unlocked tracks");
    freezeBtn.setClickingTogglesState (true);
    freezeBtn.getProperties().set ("led", true);
    genBtn.onClick = [this] { proc.generate(); };
    mutBtn.onClick = [this] { proc.mutate(); };
    freezeBtn.onClick = [this] { proc.frozen = freezeBtn.getToggleState(); refreshButtons(); };
    undoBtn.onClick = [this] { proc.undo(); };
    clearBtn.onClick = [this]
    {
        proc.pushUndo();
        for (auto& tr : proc.pattern().tracks) if (! tr.lock) clearTrack (tr);
        proc.commitPattern();
    };
    for (auto* b : { &genBtn, &mutBtn, &freezeBtn, &undoBtn, &clearBtn }) addAndMakeVisible (*b);

    for (int v = 0; v < kVariations; ++v)
    {
        varBtn[v].setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + v)));
        varBtn[v].setTooltip ("Pattern variation " + varBtn[v].getButtonText() + ". Shift-click copies the current pattern here.");
        varBtn[v].onClick = [this, v]
        {
            if (juce::ModifierKeys::getCurrentModifiers().isShiftDown()) proc.copyVariationTo (v);
            else proc.selectVariation (v);
            refreshButtons();
        };
        addAndMakeVisible (varBtn[v]);
    }
    const char* modes[] = { "GATE", "VELOCITY", "PROB", "RATCHET", "PITCH", "MICRO" };
    for (int m = 0; m < 6; ++m)
    {
        modeBtn[m].setButtonText (modes[m]);
        modeBtn[m].setRadioGroupId (77);
        modeBtn[m].setClickingTogglesState (true);
        modeBtn[m].onClick = [this, m] { grid->mode = (EditMode) m; grid->repaint(); };
        addAndMakeVisible (modeBtn[m]);
    }
    modeBtn[0].setToggleState (true, juce::dontSendNotification);
    syncFromPattern();
    refreshButtons();
}

SeqPage::~SeqPage() = default;

void SeqPage::refreshButtons()
{
    freezeBtn.setToggleState (proc.frozen, juce::dontSendNotification);
    genBtn.setEnabled (! proc.frozen);
    mutBtn.setEnabled (! proc.frozen);
    for (int v = 0; v < kVariations; ++v) varBtn[v].setToggleState (proc.currentVariation() == v, juce::dontSendNotification);
}

void SeqPage::syncFromPattern()
{
    for (auto& s : strips) s->sync();
    grid->repaint();
    lane->repaint();
    refreshButtons();
}

void SeqPage::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    auto top = r.removeFromTop (124);
    genPod = top.removeFromLeft (top.getWidth() * 58 / 100);
    top.removeFromLeft (10);
    clockPod = top;
    {
        auto a = genPod.reduced (12, 8);
        a.removeFromTop (16);
        auto btns = a.removeFromRight (230);
        layoutRow (a, { &densK, &compK, &chaosK, &grooveK, &repK }, 2);
        auto b1 = btns.removeFromTop (btns.getHeight() / 2);
        genBtn.setBounds (b1.removeFromLeft (120).reduced (3));
        mutBtn.setBounds (b1.reduced (3));
        layoutRow (btns.reduced (0, 3), { &freezeBtn, &undoBtn, &clearBtn }, 4);
    }
    {
        auto a = clockPod.reduced (12, 8);
        a.removeFromTop (16);
        auto ctl = a.removeFromRight (150);
        layoutRow (a, { &swingK, &humanK, &probK, &bpmK }, 2);
        stepsC.setBounds (ctl.removeFromTop (34).reduced (2, 0));
        midiC.setBounds (ctl.removeFromTop (34).reduced (2, 0));
        runT.setBounds (ctl.removeFromTop (28).reduced (2, 3));
    }
    r.removeFromTop (8);
    bottomPod = r.removeFromBottom (92);
    r.removeFromBottom (8);
    gridPod = r;
    {
        auto g = gridPod.reduced (8, 6);
        auto left = g.removeFromLeft (440);
        const int h = left.getHeight() / kTracks;
        for (auto& s : strips) s->setBounds (left.removeFromTop (h));
        g.removeFromLeft (6);
        grid->setBounds (g);
    }
    {
        auto b = bottomPod.reduced (12, 8);
        auto row = b.removeFromTop (26);
        for (int m = 0; m < 6; ++m) modeBtn[m].setBounds (row.removeFromLeft (84).reduced (2, 0));
        row.removeFromLeft (40);
        for (int v = 0; v < kVariations; ++v) varBtn[v].setBounds (row.removeFromLeft (34).reduced (2, 0));
        b.removeFromTop (8);
        lane->setBounds (b.withTrimmedLeft (110));
    }
}

void SeqPage::paint (juce::Graphics& g)
{
    drawPod (g, genPod.toFloat(), 21);
    drawPod (g, clockPod.toFloat(), 22);
    drawPod (g, gridPod.toFloat(), 23, juce::Colour (0xff0f110d));
    drawPod (g, bottomPod.toFloat(), 24);
    g.setFont (label (11.0f));
    g.setColour (silver);
    g.drawText ("PATTERN GENERATOR", genPod.reduced (14, 8).removeFromTop (14), juce::Justification::left);
    g.drawText ("GROOVE / CLOCK", clockPod.reduced (14, 8).removeFromTop (14), juce::Justification::left);
    auto b = bottomPod.reduced (12, 8);
    b.removeFromTop (34);
    g.drawText ("STEP LANE", b.removeFromLeft (100), juce::Justification::centredLeft);
    auto row = bottomPod.reduced (12, 8).removeFromTop (26);
    g.setColour (textDim);
    g.drawText ("VAR", row.withTrimmedLeft (6 * 84 + 6).removeFromLeft (34), juce::Justification::centredLeft);
}

void SeqPage::tick()
{
    const auto ep = proc.patternEpoch.load();
    if (ep != lastEpoch) { lastEpoch = ep; syncFromPattern(); }
    for (int t = 0; t < kTracks; ++t) proc.trackFlash[t].store (proc.trackFlash[t].load() * 0.82f);
    grid->repaint();
    lane->repaint();
    tickKnobs();
}
} // namespace ztx
