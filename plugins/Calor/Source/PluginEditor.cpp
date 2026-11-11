#include "PluginEditor.h"

using namespace calor;

//==============================================================================
ui::LookAndFeel::LookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, ui::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, ui::accent.withAlpha (0.4f));
    setColour (juce::Label::textColourId, ui::text);
    setColour (juce::ComboBox::backgroundColourId, ui::bg);
    setColour (juce::ComboBox::textColourId, ui::text);
    setColour (juce::ComboBox::outlineColourId, ui::panelEdge);
    setColour (juce::ComboBox::arrowColourId, ui::accent);
    setColour (juce::PopupMenu::backgroundColourId, ui::panel);
    setColour (juce::PopupMenu::textColourId, ui::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent);
    setColour (juce::PopupMenu::highlightedTextColourId, ui::bg);
    setColour (juce::TooltipWindow::backgroundColourId, ui::panel);
}

void ui::LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float start, float end, juce::Slider& s)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float lineW = juce::jmax (3.0f, radius * 0.14f);
    const float arcR = radius - lineW * 0.5f;
    const float angle = start + pos * (end - start);

    // Fondo de la perilla
    g.setColour (ui::bg);
    g.fillEllipse (juce::Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre));

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour (ui::panelEdge);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Los parametros bipolares (tono, envolvente) se pintan desde el centro
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? (start + end) * 0.5f : start;

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setGradientFill (juce::ColourGradient (ui::accent2, centre.x, centre.y - radius,
                                             ui::accent, centre.x, centre.y + radius, false));
    g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto tip = centre.getPointOnCircumference (radius * 0.52f, angle);
    g.setColour (ui::text);
    g.drawLine (juce::Line<float> (centre.getPointOnCircumference (radius * 0.2f, angle), tip), 2.0f);
}

void ui::LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? ui::accent : ui::bg);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (highlighted ? ui::accent2 : ui::panelEdge);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    g.setColour (on ? ui::bg : ui::dim);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

void ui::LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (ui::accent);
    g.fillPath (arrow);
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& title)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxTextColourId, ui::text);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (title, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, ui::dim);
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    slider.setNumDecimalPlacesToDisplay (1);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

//==============================================================================
XYPad::XYPad (CalorProcessor& p)
    : proc (p),
      heat (p.apvts.getParameter ("heat")),
      character (p.apvts.getParameter ("character"))
{
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    setTooltip (u8 ("Arrastra el punto: horizontal = más saturación, vertical = más brillo. Doble clic: centrar."));
}

juce::Rectangle<float> XYPad::circle() const
{
    auto r = getLocalBounds().toFloat().reduced (30.0f, 40.0f);
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    return juce::Rectangle<float> (d, d).withCentre (r.getCentre());
}

void XYPad::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (ui::panelEdge);
    g.drawRoundedRectangle (area.reduced (0.5f), 12.0f, 1.0f);

    const auto c = circle();
    const auto centre = c.getCentre();
    const float hx = heat->getValue();
    const float cy = character->getValue();
    const float level = juce::jlimit (0.0f, 1.0f, proc.outputLevel.load());
    const float mod = proc.modulationValue.load();

    // Brillo interior: crece con el calor y con el nivel de salida
    const float glow = 0.25f + 0.55f * hx + 0.2f * level;
    juce::ColourGradient grad (ui::accent2.withAlpha (glow), centre.x, centre.y,
                               ui::bg.withAlpha (0.0f), c.getRight(), centre.y, true);
    grad.addColour (0.45, ui::accent.withAlpha (glow * 0.6f));
    g.setGradientFill (grad);
    g.fillEllipse (c);

    // Anillos
    g.setColour (ui::panelEdge);
    for (float k : { 0.33f, 0.66f })
        g.drawEllipse (c.withSizeKeepingCentre (c.getWidth() * k, c.getHeight() * k), 1.0f);
    g.setColour (ui::accent.withAlpha (0.8f));
    g.drawEllipse (c, 1.5f + 6.0f * std::abs (mod));

    g.setColour (ui::panelEdge);
    g.drawLine (c.getX(), centre.y, c.getRight(), centre.y, 1.0f);
    g.drawLine (centre.x, c.getY(), centre.x, c.getBottom(), 1.0f);

    // Punto
    const juce::Point<float> dot (c.getX() + hx * c.getWidth(), c.getBottom() - cy * c.getHeight());
    g.setColour (ui::text.withAlpha (0.25f));
    g.fillEllipse (juce::Rectangle<float> (30.0f, 30.0f).withCentre (dot));
    g.setColour (ui::text);
    g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (dot));

    // Etiquetas
    g.setColour (ui::dim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (u8 ("BRILLANTE"), area.removeFromTop (34.0f), juce::Justification::centred);
    g.drawText (u8 ("OSCURO"), getLocalBounds().toFloat().removeFromBottom (34.0f), juce::Justification::centred);
    auto mid = getLocalBounds().toFloat().withSizeKeepingCentre (getWidth() - 8.0f, 20.0f);
    g.drawText (u8 ("FRÍO"), mid, juce::Justification::centredLeft);
    g.drawText (u8 ("CALIENTE"), mid, juce::Justification::centredRight);
}

void XYPad::setFromMouse (juce::Point<float> pos)
{
    const auto c = circle();
    auto v = (pos - c.getCentre()) / (c.getWidth() * 0.5f);
    const float len = std::sqrt (v.x * v.x + v.y * v.y);
    if (len > 1.0f)
        v /= len;

    heat->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, 0.5f + v.x * 0.5f));
    character->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, 0.5f - v.y * 0.5f));
    repaint();
}

void XYPad::mouseDown (const juce::MouseEvent& e)
{
    heat->beginChangeGesture();
    character->beginChangeGesture();
    setFromMouse (e.position);
}

void XYPad::mouseDrag (const juce::MouseEvent& e) { setFromMouse (e.position); }

void XYPad::mouseUp (const juce::MouseEvent&)
{
    heat->endChangeGesture();
    character->endChangeGesture();
}

void XYPad::mouseDoubleClick (const juce::MouseEvent&)
{
    heat->beginChangeGesture();
    character->beginChangeGesture();
    heat->setValueNotifyingHost (0.5f);
    character->setValueNotifyingHost (0.5f);
    heat->endChangeGesture();
    character->endChangeGesture();
}

//==============================================================================
StagePanel::StagePanel (juce::AudioProcessorValueTreeState& state, int i)
    : index (i),
      drive (state, "s" + juce::String (i + 1) + "_drive", u8 ("Saturación")),
      mix   (state, "s" + juce::String (i + 1) + "_mix", u8 ("Mezcla")),
      tone  (state, "s" + juce::String (i + 1) + "_tone", u8 ("Tono"))
{
    const auto prefix = "s" + juce::String (i + 1) + "_";

    onButton.setButtonText ("ON");
    addAndMakeVisible (onButton);
    onAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, prefix + "on", onButton);

    typeBox.addItemList (state.getParameter (prefix + "type")->getAllValueStrings(), 1);
    addAndMakeVisible (typeBox);
    typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, prefix + "type", typeBox);

    addAndMakeVisible (drive);
    addAndMakeVisible (mix);
    addAndMakeVisible (tone);

    title = u8 ("ETAPA ") + juce::String (i + 1);
}

void StagePanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (onButton.getToggleState() ? ui::accent.withAlpha (0.6f) : ui::panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);

    g.setColour (ui::text);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (title, getLocalBounds().reduced (14, 0).removeFromTop (40), juce::Justification::centredLeft);
}

void StagePanel::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto top = r.removeFromTop (28);
    onButton.setBounds (top.removeFromRight (48).reduced (0, 3));
    r.removeFromTop (8);
    typeBox.setBounds (r.removeFromTop (28));
    r.removeFromTop (10);
    drive.setBounds (r.removeFromTop (r.getHeight() / 2 + 20));
    r.removeFromTop (6);
    mix.setBounds (r.removeFromLeft (r.getWidth() / 2));
    tone.setBounds (r);
}

//==============================================================================
CalorEditor::CalorEditor (CalorProcessor& p)
    : AudioProcessorEditor (&p), proc (p), pad (p),
      xoverLo   (p.apvts, "xover_lo",   u8 ("Cruce graves")),
      xoverHi   (p.apvts, "xover_hi",   u8 ("Cruce agudos")),
      lfoRate   (p.apvts, "lfo_rate",   u8 ("LFO vel.")),
      lfoDepth  (p.apvts, "lfo_depth",  u8 ("LFO prof.")),
      envAmount (p.apvts, "env_amount", u8 ("Envolvente")),
      width     (p.apvts, "width",      u8 ("Anchura")),
      outTone   (p.apvts, "out_tone",   u8 ("Tono")),
      mix       (p.apvts, "mix",        u8 ("Mezcla")),
      output    (p.apvts, "output",     u8 ("Salida"))
{
    setLookAndFeel (&lnf);

    for (int i = 0; i < p.getNumPrograms(); ++i)
        presetBox.addItem (p.getProgramName (i), i + 1);
    presetBox.setSelectedId (p.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.getCurrentProgram())
            proc.setCurrentProgram (idx);
    };
    addAndMakeVisible (presetBox);

    modeBox.addItemList ({ u8 ("Modo: Serie"), u8 ("Modo: Bandas") }, 1);
    addAndMakeVisible (modeBox);
    modeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "mode", modeBox);

    addAndMakeVisible (pad);

    for (int i = 0; i < CalorProcessor::kNumStages; ++i)
    {
        stages[(size_t) i] = std::make_unique<StagePanel> (p.apvts, i);
        addAndMakeVisible (*stages[(size_t) i]);
    }

    for (auto* k : { &xoverLo, &xoverHi, &lfoRate, &lfoDepth, &envAmount, &width, &outTone, &mix, &output })
        addAndMakeVisible (*k);

    autoGain.setButtonText (u8 ("NIVEL AUTO"));
    autoGain.setTooltip (u8 ("Iguala el volumen de salida con el de entrada para comparar sin engañarte."));
    addAndMakeVisible (autoGain);
    autoGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "auto_gain", autoGain);

    setSize (960, 600);
    startTimerHz (30);
}

CalorEditor::~CalorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void CalorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);

    g.setColour (ui::accent);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("CALOR", 24, 12, 140, 40, juce::Justification::centredLeft);

    g.setColour (ui::dim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (u8 ("distorsión multietapa"), 150, 22, 220, 24, juce::Justification::centredLeft);

    // Panel inferior
    auto footer = juce::Rectangle<float> (16.0f, 462.0f, 928.0f, 126.0f);
    g.setColour (ui::panel);
    g.fillRoundedRectangle (footer, 12.0f);
    g.setColour (ui::panelEdge);
    g.drawRoundedRectangle (footer.reduced (0.5f), 12.0f, 1.0f);
}

void CalorEditor::resized()
{
    presetBox.setBounds (520, 18, 240, 30);
    modeBox.setBounds (776, 18, 168, 30);

    pad.setBounds (16, 66, 340, 384);

    const int stageX = 372, stageW = (944 - stageX - 2 * 10) / 3;
    for (int i = 0; i < CalorProcessor::kNumStages; ++i)
        stages[(size_t) i]->setBounds (stageX + i * (stageW + 10), 66, stageW, 384);

    auto footer = juce::Rectangle<int> (16, 462, 928, 126).reduced (10, 8);
    const int slot = footer.getWidth() / 10;
    for (auto* k : { &xoverLo, &xoverHi, &lfoRate, &lfoDepth, &envAmount, &width, &outTone, &mix, &output })
        k->setBounds (footer.removeFromLeft (slot));
    autoGain.setBounds (footer.withSizeKeepingCentre (footer.getWidth() - 6, 30));
}

void CalorEditor::timerCallback()
{
    pad.repaint();

    const bool bands = proc.apvts.getRawParameterValue ("mode")->load() > 0.5f;
    static const char* bandNames[] = { "GRAVES", "MEDIOS", "AGUDOS" };
    for (int i = 0; i < CalorProcessor::kNumStages; ++i)
    {
        stages[(size_t) i]->setTitle (bands ? u8 (bandNames[i]) : u8 ("ETAPA ") + juce::String (i + 1));
        stages[(size_t) i]->repaint();
    }

    xoverLo.setEnabled (bands);
    xoverHi.setEnabled (bands);
    xoverLo.setAlpha (bands ? 1.0f : 0.4f);
    xoverHi.setAlpha (bands ? 1.0f : 0.4f);

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}
