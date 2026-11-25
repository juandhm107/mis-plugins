#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ztx::theme
{
const juce::Colour bg        { 0xff0a0b09 };
const juce::Colour panel     { 0xff121410 };
const juce::Colour panel2    { 0xff1a1d17 };
const juce::Colour acid      { 0xff9dbf3a };
const juce::Colour acidDim   { 0xff56691f };
const juce::Colour silver    { 0xffa7a9a2 };
const juce::Colour silverDim { 0xff5b5d57 };
const juce::Colour text      { 0xffe8e6dc };
const juce::Colour textDim   { 0xff8d8c84 };
const juce::Colour orange    { 0xffc8742c };
const juce::Colour rust      { 0xff7a3f1c };

juce::Font mono (float size, bool bold = false);
juce::Font label (float size);

// Organic "pod" panel: wobbly rounded outline, grain, faint contour lines.
void drawPod (juce::Graphics&, juce::Rectangle<float>, int seed, juce::Colour fill = panel, float alpha = 0.92f);
// Glitched / chromatically smeared title text.
void drawGlitchText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, int seed,
                     juce::Justification j = juce::Justification::centredLeft, float amount = 1.0f);
// Full-window background with grain, blobs and contour lines (cached image).
juce::Image makeBackground (int w, int h);
} // namespace ztx::theme
