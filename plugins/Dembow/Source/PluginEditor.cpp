#include "PluginEditor.h"

using namespace dembow;

//==============================================================================
juce::Colour ui::categoryColour (int c)
{
    switch (c)
    {
        case Plucks:   return juce::Colour (0xff3ee0e8);
        case Bajos:    return juce::Colour (0xffa66bff);
        case Leads:    return juce::Colour (0xffff3d8b);
        case Brass:    return juce::Colour (0xffffb03a);
        case Pads:     return juce::Colour (0xff5b8cff);
        case Teclados: return juce::Colour (0xff59e38a);
        default:       return ui::accent;
    }
}

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
    setColour (juce::TooltipWindow::textColourId, ui::text);
    setColour (juce::TooltipWindow::outlineColourId, ui::panelEdge);
    setColour (juce::TextButton::buttonColourId, ui::bg);
    setColour (juce::TextButton::buttonOnColourId, ui::accent);
    setColour (juce::TextButton::textColourOffId, ui::dim);
    setColour (juce::TextButton::textColourOnId, ui::bg);
    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId, ui::panelEdge);
}

void ui::LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float start, float end, juce::Slider& s)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float lineW = juce::jmax (3.0f, radius * 0.16f);
    const float arcR = radius - lineW * 0.5f;
    const float angle = start + pos * (end - start);
    const auto col = s.findColour (juce::Slider::rotarySliderFillColourId);

    g.setColour (ui::bg);
    g.fillEllipse (juce::Rectangle<float> (radius * 1.45f, radius * 1.45f).withCentre (centre));

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour (ui::panelEdge);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? (start + end) * 0.5f : start;
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (col);
    g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (ui::text);
    g.drawLine (juce::Line<float> (centre.getPointOnCircumference (radius * 0.18f, angle),
                                   centre.getPointOnCircumference (radius * 0.55f, angle)), 2.0f);
}

void ui::LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                        juce::Slider::SliderStyle, juce::Slider& s)
{
    // Deslizador vertical grueso (envolventes)
    const auto col = s.findColour (juce::Slider::trackColourId);
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float trackW = 10.0f;
    auto track = r.withSizeKeepingCentre (trackW, r.getHeight());
    g.setColour (ui::bg);
    g.fillRoundedRectangle (track, trackW * 0.5f);

    auto filled = track.withTop (pos);
    g.setColour (col);
    g.fillRoundedRectangle (filled, trackW * 0.5f);

    auto thumb = juce::Rectangle<float> (juce::jmin (34.0f, r.getWidth() - 4.0f), 12.0f).withCentre ({ r.getCentreX(), pos });
    g.setColour (ui::text);
    g.fillRoundedRectangle (thumb, 4.0f);
    g.setColour (col);
    g.drawLine (thumb.getX() + 6, thumb.getCentreY(), thumb.getRight() - 6, thumb.getCentreY(), 2.0f);
}

void ui::LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void ui::LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const auto onCol = b.findColour (juce::TextButton::buttonOnColourId);
    g.setColour (on ? onCol : (down ? ui::panelEdge : ui::bg));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (on ? onCol : (highlighted ? onCol.withAlpha (0.7f) : ui::panelEdge));
    g.drawRoundedRectangle (r, 7.0f, 1.2f);
}

void ui::LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool on = b.getToggleState();
    g.setColour (on ? ui::bg : ui::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
            const juce::String& title, const juce::String& tip)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 15);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, ui::accent);
    slider.setTooltip (tip);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (title, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, ui::dim);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    label.setMinimumHorizontalScale (0.7f);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (15));
    slider.setBounds (r);
}

Choice::Choice (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
                const juce::String& title, const juce::String& tip)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId)))
        box.addItemList (p->choices, 1);
    box.setTooltip (tip);
    addAndMakeVisible (box);

    label.setText (title, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, ui::dim);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, paramId, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    if (label.getText().isNotEmpty())
        label.setBounds (r.removeFromLeft (juce::jmin (70, r.getWidth() / 3)));
    box.setBounds (r.withSizeKeepingCentre (r.getWidth(), juce::jmin (26, r.getHeight())));
}

//==============================================================================
EnvelopeView::EnvelopeView (juce::AudioProcessorValueTreeState& st, const juce::String& p, juce::Colour c)
    : a (st.getRawParameterValue (p + "a")), d (st.getRawParameterValue (p + "d")),
      s (st.getRawParameterValue (p + "s")), r (st.getRawParameterValue (p + "r")), colour (c)
{
    setInterceptsMouseClicks (false, false);
}

bool EnvelopeView::needsRepaint()
{
    const float now[4] = { a->load(), d->load(), s->load(), r->load() };
    bool changed = false;
    for (int i = 0; i < 4; ++i)
        if (now[i] != last[i]) { last[i] = now[i]; changed = true; }
    return changed;
}

void EnvelopeView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (ui::bg);
    g.fillRoundedRectangle (b, 8.0f);
    b = b.reduced (8.0f, 10.0f);

    // Escala visual: raiz de los tiempos para que se vea todo
    auto vis = [] (float t) { return std::sqrt (juce::jmax (0.0f, t)); };
    const float A = vis (a->load()), D = vis (d->load()), R = vis (r->load());
    const float S = s->load();
    const float hold = 0.6f;
    const float total = A + D + hold + R;
    const float sx = b.getWidth() / juce::jmax (0.001f, total);

    auto px = [&] (float t) { return b.getX() + t * sx; };
    auto py = [&] (float v) { return b.getBottom() - v * b.getHeight(); };

    juce::Path p;
    p.startNewSubPath (px (0), py (0));
    // Ataque
    p.quadraticTo (px (A * 0.4f), py (0.75f), px (A), py (1.0f));
    // Decaimiento (curva exponencial)
    p.quadraticTo (px (A + D * 0.25f), py (S), px (A + D), py (S));
    // Sosten
    p.lineTo (px (A + D + hold), py (S));
    // Liberacion
    p.quadraticTo (px (A + D + hold + R * 0.2f), py (0), px (total), py (0));

    juce::Path fill (p);
    fill.lineTo (px (0), py (0));
    fill.closeSubPath();
    g.setColour (colour.withAlpha (0.18f));
    g.fillPath (fill);
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (ui::dim.withAlpha (0.8f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    const float ly = b.getBottom() - 2.0f;
    auto mark = [&] (const char* t, float x0, float x1)
    {
        if (x1 - x0 > 10.0f)
            g.drawText (t, juce::Rectangle<float> (x0, ly - 12.0f, x1 - x0, 12.0f), juce::Justification::centred);
    };
    mark ("A", px (0), px (A));
    mark ("D", px (A), px (A + D));
    mark ("S", px (A + D), px (A + D + hold));
    mark ("R", px (A + D + hold), px (total));
}

EnvSection::EnvSection (juce::AudioProcessorValueTreeState& st, const juce::String& prefix, juce::Colour c)
    : view (st, prefix, c)
{
    addAndMakeVisible (view);
    const char* names[] = { "Ataque", "Decaimiento", u8"Sostén", u8"Liberación" };
    const char* ids[] = { "a", "d", "s", "r" };
    const char* tips[] =
    {
        u8"Ataque: cuánto tarda el sonido en llegar al máximo. Corto = golpe seco; largo = entra suave.",
        u8"Decaimiento: cuánto tarda en bajar desde el máximo hasta el nivel de sostén.",
        u8"Sostén: el nivel que se mantiene mientras dejas la tecla pisada. En 0 = sonido corto (pluck).",
        u8"Liberación: cuánto dura la cola después de soltar la tecla."
    };

    for (int i = 0; i < 4; ++i)
    {
        auto& f = faders[(size_t) i];
        f.slider.setSliderStyle (juce::Slider::LinearVertical);
        f.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        f.slider.setColour (juce::Slider::trackColourId, c);
        f.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        f.slider.setTooltip (juce::String::fromUTF8 (tips[i]));
        addAndMakeVisible (f.slider);
        f.label.setText (juce::String::fromUTF8 (names[i]), juce::dontSendNotification);
        f.label.setJustificationType (juce::Justification::centred);
        f.label.setColour (juce::Label::textColourId, ui::dim);
        f.label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        f.label.setMinimumHorizontalScale (0.7f);
        addAndMakeVisible (f.label);
        f.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, prefix + ids[i], f.slider);
    }
}

void EnvSection::resized()
{
    auto r = getLocalBounds();
    view.setBounds (r.removeFromLeft ((int) (r.getWidth() * 0.38f)).reduced (0, 4));
    r.removeFromLeft (8);
    const int w = r.getWidth() / 4;
    for (auto& f : faders)
    {
        auto c = r.removeFromLeft (w);
        f.label.setBounds (c.removeFromTop (15));
        f.slider.setBounds (c);
    }
}

//==============================================================================
PresetBrowser::PresetBrowser (DembowProcessor& p) : proc (p)
{
    for (int c = 0; c < NumCategories; ++c)
    {
        auto& b = catButtons[(size_t) c];
        b.setButtonText (categoryName (c));
        b.setClickingTogglesState (false);
        b.setColour (juce::TextButton::buttonOnColourId, ui::categoryColour (c));
        b.onClick = [this, c] { selectCategory (c); };
        addAndMakeVisible (b);
    }

    auto setupEra = [this] (juce::TextButton& b, int e, const char* tip)
    {
        b.setColour (juce::TextButton::buttonOnColourId, ui::text);
        b.setTooltip (juce::String::fromUTF8 (tip));
        b.onClick = [this, e] { era = e; rebuild(); };
        addAndMakeVisible (b);
    };
    setupEra (eraAll, 0, "Muestra todos los sonidos de la categoría");
    setupEra (eraOld, 1, u8"Solo sonidos del reggaetón de antes (2000 - 2010)");
    setupEra (eraNew, 2, u8"Solo sonidos del reggaetón actual");

    list.setModel (this);
    list.setRowHeight (30);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    const int cur = proc.getCurrentProgram();
    if (juce::isPositiveAndBelow (cur, (int) presets().size()))
        category = presets()[(size_t) cur].category;
    rebuild();
}

void PresetBrowser::selectCategory (int c)
{
    category = c;
    rebuild();
}

void PresetBrowser::rebuild()
{
    for (int c = 0; c < NumCategories; ++c)
        catButtons[(size_t) c].setToggleState (c == category, juce::dontSendNotification);
    eraAll.setToggleState (era == 0, juce::dontSendNotification);
    eraOld.setToggleState (era == 1, juce::dontSendNotification);
    eraNew.setToggleState (era == 2, juce::dontSendNotification);

    filtered.clear();
    const auto& all = presets();
    for (int i = 0; i < (int) all.size(); ++i)
    {
        const auto& p = all[(size_t) i];
        if (p.category != category) continue;
        if (era == 1 && ! p.oldSchool) continue;
        if (era == 2 && p.oldSchool) continue;
        filtered.push_back (i);
    }
    list.updateContent();
    list.repaint();
    repaint();
}

void PresetBrowser::refresh()
{
    const int cur = proc.getCurrentProgram();
    if (juce::isPositiveAndBelow (cur, (int) presets().size()) && presets()[(size_t) cur].category != category)
    {
        category = presets()[(size_t) cur].category;
        era = 0;
    }
    rebuild();
}

void PresetBrowser::paint (juce::Graphics& g)
{
    g.setColour (ui::dim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("1. ELIGE EL TIPO DE SONIDO", getLocalBounds().removeFromTop (18), juce::Justification::centredLeft);

    auto lr = list.getBounds().toFloat().expanded (2.0f);
    g.setColour (ui::bg);
    g.fillRoundedRectangle (lr, 8.0f);
}

void PresetBrowser::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (22);
    const int bw = (r.getWidth() - 6) / 2;
    for (int row = 0; row < 3; ++row)
    {
        auto line = r.removeFromTop (36);
        catButtons[(size_t) (row * 2)].setBounds (line.removeFromLeft (bw));
        line.removeFromLeft (6);
        catButtons[(size_t) (row * 2 + 1)].setBounds (line.removeFromLeft (bw));
        r.removeFromTop (6);
    }
    r.removeFromTop (4);
    auto eraLine = r.removeFromTop (26);
    const int ew = (eraLine.getWidth() - 8) / 3;
    eraAll.setBounds (eraLine.removeFromLeft (ew)); eraLine.removeFromLeft (4);
    eraOld.setBounds (eraLine.removeFromLeft (ew)); eraLine.removeFromLeft (4);
    eraNew.setBounds (eraLine);
    r.removeFromTop (10);
    list.setBounds (r.reduced (2));
}

void PresetBrowser::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool)
{
    if (! juce::isPositiveAndBelow (row, (int) filtered.size())) return;
    const int idx = filtered[(size_t) row];
    const auto& p = presets()[(size_t) idx];
    const bool current = idx == proc.getCurrentProgram();
    const auto col = ui::categoryColour (p.category);

    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (3.0f, 2.0f);
    if (current)
    {
        g.setColour (col.withAlpha (0.25f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (col);
        g.fillRoundedRectangle (r.withWidth (4.0f), 2.0f);
    }

    auto tagR = r.removeFromRight (78.0f).reduced (2.0f, 5.0f);
    g.setColour (p.oldSchool ? ui::accent2.withAlpha (0.2f) : ui::categoryColour (Pads).withAlpha (0.2f));
    g.fillRoundedRectangle (tagR, tagR.getHeight() * 0.5f);
    g.setColour (p.oldSchool ? ui::accent2 : ui::categoryColour (Pads).brighter (0.3f));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText (p.oldSchool ? "OLD SCHOOL" : "MODERNO", tagR, juce::Justification::centred);

    g.setColour (current ? ui::text : ui::text.withAlpha (0.85f));
    g.setFont (juce::FontOptions (13.5f, current ? juce::Font::bold : juce::Font::plain));
    g.drawText (u8 (p.name), r.withTrimmedLeft (10.0f), juce::Justification::centredLeft, true);
}

void PresetBrowser::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (juce::isPositiveAndBelow (row, (int) filtered.size()))
    {
        proc.setCurrentProgram (filtered[(size_t) row]);
        proc.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
        list.repaint();
    }
}

//==============================================================================
DembowEditor::DembowEditor (DembowProcessor& p)
    : AudioProcessorEditor (&p), proc (p), browser (p),
      o1wave (p.apvts, "o1wave", "Onda", u8 ("La forma de onda básica del oscilador 1.")),
      o2wave (p.apvts, "o2wave", "Onda", u8 ("La forma de onda del oscilador 2. También es el modulador cuando usas FM.")),
      o1oct (p.apvts, "o1oct", "Octava"), o1semi (p.apvts, "o1semi", "Semitono"),
      o1fine (p.apvts, "o1fine", "Fino", u8 ("Desafina un poquito: junto al oscilador 2 engorda el sonido.")),
      o1level (p.apvts, "o1level", "Nivel"),
      o2oct (p.apvts, "o2oct", "Octava"), o2semi (p.apvts, "o2semi", "Semitono"),
      o2fine (p.apvts, "o2fine", "Fino"),
      o2level (p.apvts, "o2level", "Nivel", u8 ("Volumen del oscilador 2. En 0 solo sirve como modulador FM.")),
      sub (p.apvts, "sub", "Sub", u8 ("Un seno una octava por debajo: más graves.")),
      noise (p.apvts, "noise", "Ruido", u8 ("Ruido blanco: soplido, aire o golpe.")),
      pw (p.apvts, "pw", "Pulso", u8 ("Ancho de la onda Pulso: cambia el color de nasal a gordo.")),
      detune (p.apvts, "detune", "Desafinar", u8 ("Cuánto se abren las 7 sierras de la onda Súper sierra.")),
      fm (p.apvts, "fm", "FM", u8 ("El oscilador 2 modula al 1: sonidos de campana, marimba y piano eléctrico.")),
      fmenv (p.apvts, "fmenv", "FM golpe", u8 ("FM extra que sigue a la envolvente del filtro: brillo solo en el ataque.")),
      ftype (p.apvts, "ftype", "Tipo", u8 ("Graves = quita agudos (lo normal). Banda = sonido hueco. Agudos = quita graves.")),
      cutoff (p.apvts, "cutoff", "Corte", u8 ("Brillo: hacia la izquierda más oscuro, hacia la derecha más brillante.")),
      reso (p.apvts, "reso", "Resonancia", u8 ("Realza la zona del corte: sonido chillón, tipo acid.")),
      fenv (p.apvts, "fenv", "Envolvente", u8 ("Cuánto abre el filtro la envolvente del filtro (el 'pluck' del sonido).")),
      keytrk (p.apvts, "keytrk", "Teclado", u8 ("Las notas agudas suenan más brillantes que las graves.")),
      drive (p.apvts, "drive", u8 ("Saturación"), u8 ("Distorsión cálida, ideal para 808.")),
      fltEnv (p.apvts, "f", ui::categoryColour (Plucks)),
      ampEnv (p.apvts, "a", ui::accent),
      lfowave (p.apvts, "lfowave", "", u8 ("Forma del LFO.")),
      lfodest (p.apvts, "lfodest", "", u8 ("Qué mueve el LFO: tono = vibrato, volumen = trémolo.")),
      lforate (p.apvts, "lforate", "Velocidad"),
      lfodepth (p.apvts, "lfodepth", "Cantidad"),
      lfofade (p.apvts, "lfofade", "Entrada", u8 ("Tiempo que tarda el LFO en aparecer (vibrato que entra despacio).")),
      mode (p.apvts, "mode", "", u8 ("Poli = acordes. Mono = una nota a la vez. Legato = una nota y sin re-ataque al ligar (ideal 808).")),
      glide (p.apvts, "glide", "Glide", u8 ("Deslizamiento de una nota a otra (el slide del 808).")),
      velsens (p.apvts, "velsens", "Fuerza", u8 ("Cuánto cambia el sonido según lo fuerte que tocas.")),
      dlytime (p.apvts, "dlytime", "Tiempo", u8 ("Tiempo del eco, sincronizado al tempo de Logic.")),
      chorus (p.apvts, "chorus", "Chorus", u8 ("Ensancha y engorda el sonido.")),
      dlymix (p.apvts, "dlymix", "Eco", u8 ("Cantidad de eco (delay ping-pong).")),
      dlyfb (p.apvts, "dlyfb", "Repeticiones"),
      revmix (p.apvts, "revmix", "Reverb", u8 ("Cantidad de reverberación (espacio).")),
      revsize (p.apvts, "revsize", u8 ("Tamaño")),
      volume (p.apvts, "volume", "Volumen"),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (browser);

    for (auto* b : { &prevButton, &nextButton })
    {
        b->setColour (juce::TextButton::buttonOnColourId, ui::accent);
        addAndMakeVisible (b);
    }
    prevButton.setTooltip ("Sonido anterior");
    nextButton.setTooltip ("Sonido siguiente");
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };

    presetName.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    presetName.setJustificationType (juce::Justification::centred);
    presetInfo.setFont (juce::FontOptions (12.5f));
    presetInfo.setColour (juce::Label::textColourId, ui::dim);
    presetInfo.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (presetName);
    addAndMakeVisible (presetInfo);

    auto colourKnobs = [] (std::initializer_list<Knob*> ks, juce::Colour c)
    {
        for (auto* k : ks) k->slider.setColour (juce::Slider::rotarySliderFillColourId, c);
    };
    colourKnobs ({ &o1oct, &o1semi, &o1fine, &o1level, &o2oct, &o2semi, &o2fine, &o2level }, ui::categoryColour (Bajos));
    colourKnobs ({ &sub, &noise, &pw, &detune, &fm, &fmenv }, ui::categoryColour (Pads));
    colourKnobs ({ &cutoff, &reso, &fenv, &keytrk, &drive }, ui::categoryColour (Plucks));
    colourKnobs ({ &lforate, &lfodepth, &lfofade }, ui::categoryColour (Teclados));
    colourKnobs ({ &glide, &velsens }, ui::accent2);
    colourKnobs ({ &chorus, &dlymix, &dlyfb, &revmix, &revsize }, ui::categoryColour (Brass));
    colourKnobs ({ &volume }, ui::accent);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &o1wave, &o2wave, &o1oct, &o1semi, &o1fine, &o1level, &o2oct, &o2semi, &o2fine, &o2level,
             &sub, &noise, &pw, &detune, &fm, &fmenv,
             &ftype, &cutoff, &reso, &fenv, &keytrk, &drive, &fltEnv, &ampEnv,
             &lfowave, &lfodest, &lforate, &lfodepth, &lfofade, &mode, &glide, &velsens,
             &dlytime, &chorus, &dlymix, &dlyfb, &revmix, &revsize, &volume })
        addAndMakeVisible (c);

    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, ui::accent.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, ui::accent.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfff1ecf7));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, ui::bg);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, ui::panelEdge);
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setAvailableRange (24, 108);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    setSize (1220, 810);
    startTimerHz (30);
    timerCallback();
}

DembowEditor::~DembowEditor()
{
    setLookAndFeel (nullptr);
}

void DembowEditor::stepPreset (int delta)
{
    const int n = (int) presets().size();
    const int next = ((proc.getCurrentProgram() + delta) % n + n) % n;
    proc.setCurrentProgram (next);
    proc.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
}

void DembowEditor::timerCallback()
{
    const int changes = proc.programChanges.load();
    if (changes != lastProgramChanges)
    {
        lastProgramChanges = changes;
        const int cur = proc.getCurrentProgram();
        if (juce::isPositiveAndBelow (cur, (int) presets().size()))
        {
            const auto& p = presets()[(size_t) cur];
            presetName.setText (u8 (p.name), juce::dontSendNotification);
            presetName.setColour (juce::Label::textColourId, ui::categoryColour (p.category));
            presetInfo.setText (u8 (categoryName (p.category)) + "  |  " + (p.oldSchool ? "OLD SCHOOL" : "MODERNO")
                                    + "  |  " + u8 (p.info), juce::dontSendNotification);
        }
        browser.refresh();
    }

    if (fltEnv.view.needsRepaint()) fltEnv.view.repaint();
    if (ampEnv.view.needsRepaint()) ampEnv.view.repaint();
}

void DembowEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, juce::Colour c)
{
    auto f = r.toFloat();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (f, 10.0f);
    g.setColour (ui::panelEdge);
    g.drawRoundedRectangle (f.reduced (0.5f), 10.0f, 1.0f);

    if (title.isEmpty()) return;
    g.setColour (c);
    g.fillRoundedRectangle (juce::Rectangle<float> (f.getX() + 12.0f, f.getY() + 11.0f, 4.0f, 12.0f), 2.0f);
    g.setColour (ui::text);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (title, r.getX() + 22, r.getY() + 6, r.getWidth() - 30, 22, juce::Justification::centredLeft);
}

void DembowEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);

    // Barra superior
    auto top = getLocalBounds().removeFromTop (62).toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a1440), 0, 0, juce::Colour (0xff160f26), top.getRight(), 0, false));
    g.fillRect (top);

    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.setGradientFill (juce::ColourGradient (ui::accent, 18, 0, ui::accent2, 190, 0, false));
    g.drawText ("DEMBOW", 18, 6, 200, 34, juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (u8 ("SINTETIZADOR DE REGGAETÓN"), 19, 38, 220, 16, juce::Justification::centredLeft);

    drawPanel (g, rBrowser, {}, ui::accent);
    drawPanel (g, rOsc1, "OSCILADOR 1", ui::categoryColour (Bajos));
    drawPanel (g, rOsc2, "OSCILADOR 2", ui::categoryColour (Bajos));
    drawPanel (g, rMix, "MEZCLA Y COLOR", ui::categoryColour (Pads));
    drawPanel (g, rFilter, u8 ("FILTRO (BRILLO)"), ui::categoryColour (Plucks));
    drawPanel (g, rFltEnv, u8 ("ENVOLVENTE DEL FILTRO  ·  cómo cambia el brillo en el tiempo"), ui::categoryColour (Plucks));
    drawPanel (g, rAmpEnv, u8 ("ENVOLVENTE DE VOLUMEN  ·  ataque, decaimiento, sostén, liberación"), ui::accent);
    drawPanel (g, rLfo, "LFO (MOVIMIENTO)", ui::categoryColour (Teclados));
    drawPanel (g, rVoice, "VOZ", ui::accent2);
    drawPanel (g, rFx, "EFECTOS", ui::categoryColour (Brass));
}

void DembowEditor::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop (62);

    // Barra superior: navegacion de presets y volumen
    {
        auto t = top.reduced (10, 8);
        volume.setBounds (t.removeFromRight (70).withTrimmedTop (-4));
        auto mid = t.withTrimmedLeft (250).withTrimmedRight (40);
        mid = mid.withSizeKeepingCentre (juce::jmin (640, mid.getWidth()), mid.getHeight());
        prevButton.setBounds (mid.removeFromLeft (40).withSizeKeepingCentre (40, 36));
        nextButton.setBounds (mid.removeFromRight (40).withSizeKeepingCentre (40, 36));
        presetName.setBounds (mid.removeFromTop (26));
        presetInfo.setBounds (mid);
    }

    area.reduce (12, 8);
    keyboard.setBounds (area.removeFromBottom (76));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 50.0f);
    keyboard.setLowestVisibleKey (24);
    area.removeFromBottom (8);

    rBrowser = area.removeFromLeft (252);
    browser.setBounds (rBrowser.reduced (10, 10));
    area.removeFromLeft (8);

    const int gap = 8;
    auto content = [] (juce::Rectangle<int> r) { return r.withTrimmedTop (30).reduced (10, 0).withTrimmedBottom (8); };

    auto layKnobs = [] (juce::Rectangle<int> r, std::initializer_list<juce::Component*> ks)
    {
        const int w = r.getWidth() / (int) ks.size();
        for (auto* k : ks) k->setBounds (r.removeFromLeft (w).reduced (2, 0));
    };

    // Fila A: osciladores y mezcla
    auto rowA = area.removeFromTop (150); area.removeFromTop (gap);
    rOsc1 = rowA.removeFromLeft (276); rowA.removeFromLeft (gap);
    rOsc2 = rowA.removeFromLeft (276); rowA.removeFromLeft (gap);
    rMix = rowA;
    {
        auto c = content (rOsc1);
        o1wave.setBounds (c.removeFromTop (28)); c.removeFromTop (4);
        layKnobs (c, { &o1oct, &o1semi, &o1fine, &o1level });
        c = content (rOsc2);
        o2wave.setBounds (c.removeFromTop (28)); c.removeFromTop (4);
        layKnobs (c, { &o2oct, &o2semi, &o2fine, &o2level });
        c = content (rMix).withTrimmedTop (14);
        layKnobs (c, { &sub, &noise, &pw, &detune, &fm, &fmenv });
    }

    // Fila B: filtro y su envolvente
    auto rowB = area.removeFromTop (190); area.removeFromTop (gap);
    rFilter = rowB.removeFromLeft (420); rowB.removeFromLeft (gap);
    rFltEnv = rowB;
    {
        auto c = content (rFilter);
        ftype.setBounds (c.removeFromTop (28).withTrimmedRight (160)); c.removeFromTop (10);
        layKnobs (c.withSizeKeepingCentre (c.getWidth(), juce::jmin (c.getHeight(), 120)),
                  { &cutoff, &reso, &fenv, &keytrk, &drive });
        fltEnv.setBounds (content (rFltEnv));
    }

    // Fila C: envolvente de volumen, LFO, voz
    auto rowC = area.removeFromTop (190); area.removeFromTop (gap);
    rAmpEnv = rowC.removeFromLeft (450); rowC.removeFromLeft (gap);
    rLfo = rowC.removeFromLeft (290); rowC.removeFromLeft (gap);
    rVoice = rowC;
    {
        ampEnv.setBounds (content (rAmpEnv));
        auto c = content (rLfo);
        auto line = c.removeFromTop (28);
        lfowave.setBounds (line.removeFromLeft (line.getWidth() / 2 - 3));
        line.removeFromLeft (6);
        lfodest.setBounds (line);
        c.removeFromTop (10);
        layKnobs (c.withSizeKeepingCentre (c.getWidth(), juce::jmin (c.getHeight(), 120)), { &lforate, &lfodepth, &lfofade });
        c = content (rVoice);
        mode.setBounds (c.removeFromTop (28)); c.removeFromTop (10);
        layKnobs (c.withSizeKeepingCentre (c.getWidth(), juce::jmin (c.getHeight(), 120)), { &glide, &velsens });
    }

    // Fila D: efectos
    rFx = area;
    {
        auto c = rFx.reduced (10, 6);
        c.removeFromLeft (90);   // espacio para el titulo
        const int w = c.getWidth() / 6;
        chorus.setBounds (c.removeFromLeft (w).reduced (4, 0));
        dlymix.setBounds (c.removeFromLeft (w).reduced (4, 0));
        auto timeArea = c.removeFromLeft (w + 30);
        dlytime.setBounds (timeArea.withSizeKeepingCentre (timeArea.getWidth() - 8, 28));
        dlyfb.setBounds (c.removeFromLeft (w).reduced (4, 0));
        revmix.setBounds (c.removeFromLeft (w).reduced (4, 0));
        revsize.setBounds (c.reduced (4, 0));
    }
}
