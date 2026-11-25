#include "LookAndFeel.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace ztx
{
using namespace theme;

ZtxLookAndFeel::ZtxLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::Label::textColourId, text);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, panel2);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, silverDim);
    setColour (juce::ComboBox::arrowColourId, acid);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, acidDim);
    setColour (juce::PopupMenu::highlightedTextColourId, text);
    setColour (juce::PopupMenu::headerTextColourId, orange);
    setColour (juce::TextButton::buttonColourId, panel2);
    setColour (juce::TextButton::buttonOnColourId, acidDim);
    setColour (juce::TextButton::textColourOffId, silver);
    setColour (juce::TextButton::textColourOnId, text);
    setColour (juce::TextEditor::backgroundColourId, panel2);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, silverDim);
    setColour (juce::TextEditor::focusedOutlineColourId, acid);
    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, panel2);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::BubbleComponent::backgroundColourId, panel2);
    setColour (juce::BubbleComponent::outlineColourId, acid);
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xff2a2d26));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff0d0e0c));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, bg);
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, acid.withAlpha (0.7f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, silver.withAlpha (0.2f));
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, panel2);
    setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, acid);
}

void ZtxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = std::min (area.getWidth(), area.getHeight()) * 0.5f;
    const auto c = area.getCentre();
    const float ang = start + pos * (end - start);
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroAng = bipolar ? start + (float) s.valueToProportionOfLength (0.0) * (end - start) : start;
    const float trackW = std::max (2.0f, r * 0.13f);

    // track
    juce::Path track;
    track.addCentredArc (c.x, c.y, r - trackW, r - trackW, 0, start, end, true);
    g.setColour (silverDim.withAlpha (0.35f));
    g.strokePath (track, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc
    juce::Path val;
    val.addCentredArc (c.x, c.y, r - trackW, r - trackW, 0, std::min (zeroAng, ang), std::max (zeroAng, ang), true);
    g.setColour (s.isEnabled() ? acid : silverDim);
    g.strokePath (val, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // modulation arc (orange), offset stored by the Knob as normalised value
    const float mod = (float) s.getProperties().getWithDefault ("mod", 0.0f);
    if (std::abs (mod) > 0.002f)
    {
        const float mpos = juce::jlimit (0.0f, 1.0f, pos + mod);
        const float mang = start + mpos * (end - start);
        juce::Path m;
        m.addCentredArc (c.x, c.y, r - trackW * 2.4f, r - trackW * 2.4f, 0, std::min (ang, mang), std::max (ang, mang), true);
        g.setColour (orange.withAlpha (0.9f));
        g.strokePath (m, juce::PathStrokeType (trackW * 0.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.fillEllipse (juce::Rectangle<float> (4, 4).withCentre (c.getPointOnCircumference (r - trackW * 2.4f, mang)));
    }

    // body: slightly irregular organic blob
    const float br = r * 0.62f;
    juce::Path body;
    const int seed = (int) (reinterpret_cast<juce::pointer_sized_int> (&s) >> 4);
    for (int i = 0; i <= 36; ++i)
    {
        const float a = (float) i / 36.0f * juce::MathConstants<float>::twoPi;
        const float rr = br * (1.0f + 0.035f * std::sin (a * 3.0f + (float) (seed % 17)) + 0.02f * std::sin (a * 5.0f + (float) (seed % 7)));
        const auto p = c.getPointOnCircumference (rr, a);
        if (i == 0) body.startNewSubPath (p); else body.lineTo (p);
    }
    body.closeSubPath();
    juce::ColourGradient gr (juce::Colour (0xff3a3d36), c.x - br * 0.5f, c.y - br * 0.6f, juce::Colour (0xff121310), c.x + br * 0.6f, c.y + br * 0.8f, false);
    g.setGradientFill (gr);
    g.fillPath (body);
    g.setColour (silver.withAlpha (s.isMouseOverOrDragging() ? 0.55f : 0.25f));
    g.strokePath (body, juce::PathStrokeType (1.0f));

    // pointer
    const auto p1 = c.getPointOnCircumference (br * 0.25f, ang);
    const auto p2 = c.getPointOnCircumference (br * 0.92f, ang);
    g.setColour (text);
    g.drawLine ({ p1, p2 }, std::max (1.5f, r * 0.07f));
}

void ZtxLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                       juce::Slider::SliderStyle style, juce::Slider& s)
{
    const auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    if (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical)
    {
        g.setColour (panel2);
        g.fillRoundedRectangle (r, 3.0f);
        const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
        if (bip)
        {
            const float zx = r.getX() + (float) s.valueToProportionOfLength (0.0) * r.getWidth();
            g.setColour (acidDim.withAlpha (0.8f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (std::min (zx, pos), r.getY(), std::max (zx, pos), r.getBottom()));
        }
        else
        {
            g.setColour (acidDim.withAlpha (0.55f));
            g.fillRoundedRectangle (r.withRight (pos), 3.0f);
        }
        g.setColour (silverDim.withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        return;
    }
    // horizontal fader (used for mod amounts)
    const auto track = r.withSizeKeepingCentre (r.getWidth(), 4.0f);
    g.setColour (silverDim.withAlpha (0.4f));
    g.fillRoundedRectangle (track, 2.0f);
    const float zx = (s.getMinimum() < 0) ? r.getX() + (float) s.valueToProportionOfLength (0.0) * r.getWidth() : r.getX();
    g.setColour (pos >= zx ? acid : orange);
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (std::min (zx, pos), track.getY(), std::max (zx, pos), track.getBottom()));
    g.setColour (text);
    g.fillEllipse (juce::Rectangle<float> (9, 9).withCentre ({ pos, r.getCentreY() }));
}

void ZtxLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const auto accent = b.getProperties().contains ("accent") ? orange : acid;
    juce::Colour fill = on ? accent.withAlpha (0.28f) : panel2;
    if (down) fill = fill.brighter (0.2f);
    else if (over) fill = fill.brighter (0.08f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, r.getHeight() * 0.32f);
    g.setColour (on ? accent : silverDim.withAlpha (over ? 0.9f : 0.6f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.32f, on ? 1.4f : 1.0f);
    if (b.getProperties().contains ("led"))
    {
        const auto led = juce::Rectangle<float> (6, 6).withCentre ({ r.getX() + 9.0f, r.getCentreY() });
        g.setColour (on ? accent : silverDim.withAlpha (0.5f));
        g.fillEllipse (led);
        if (on) { g.setColour (accent.withAlpha (0.25f)); g.fillEllipse (led.expanded (3)); }
    }
}

void ZtxLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    auto r = b.getLocalBounds();
    if (b.getProperties().contains ("led")) r.removeFromLeft (14);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? text : (b.isEnabled() ? silver : silverDim));
    g.drawFittedText (b.getButtonText(), r.reduced (3, 0), juce::Justification::centred, 1, 0.7f);
}

juce::Font ZtxLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return label (std::min (13.0f, (float) h * 0.48f)); }

void ZtxLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (panel2);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (false) || box.isMouseOver() ? acid.withAlpha (0.8f) : silverDim.withAlpha (0.7f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 11.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (acid);
    g.fillPath (arrow);
}

juce::Font ZtxLookAndFeel::getComboBoxFont (juce::ComboBox& b) { return mono (std::min (12.5f, (float) b.getHeight() * 0.55f)); }
juce::Font ZtxLookAndFeel::getPopupMenuFont() { return mono (13.0f); }
juce::Font ZtxLookAndFeel::getLabelFont (juce::Label& l) { return l.getFont(); }

void ZtxLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& l)
{
    l.setBounds (4, 0, box.getWidth() - 20, box.getHeight());
    l.setFont (getComboBoxFont (box));
}

juce::Label* ZtxLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (mono (11.5f));
    l->setColour (juce::Label::textColourId, text);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

void ZtxLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& t, int w, int h)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (panel2.withAlpha (0.97f));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (acid.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (text);
    g.setFont (mono (12.0f));
    g.drawFittedText (t, r.reduced (8, 4).toNearestInt(), juce::Justification::centredLeft, 4);
}

juce::Rectangle<int> ZtxLookAndFeel::getTooltipBounds (const juce::String& t, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    const int w = std::min (360, juce::GlyphArrangement::getStringWidthInt (mono (12.0f), t) + 20);
    const int lines = 1 + juce::GlyphArrangement::getStringWidthInt (mono (12.0f), t) / 340;
    const int h = 10 + lines * 16;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 18,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 6) : pos.y + 6, w, h).constrainedWithin (parent);
}

void ZtxLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (panel);
    g.setColour (acid.withAlpha (0.4f));
    g.drawRect (0, 0, w, h);
}

void ZtxLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                    int thumbStart, int thumbSize, bool over, bool)
{
    auto thumb = vertical ? juce::Rectangle<int> (x, thumbStart, w, thumbSize) : juce::Rectangle<int> (thumbStart, y, thumbSize, h);
    g.setColour ((over ? acid : silverDim).withAlpha (0.6f));
    g.fillRoundedRectangle (thumb.reduced (2).toFloat(), 3.0f);
}
} // namespace ztx
