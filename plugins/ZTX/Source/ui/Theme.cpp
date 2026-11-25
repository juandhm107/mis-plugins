#include "Theme.h"
#include "../dsp/DspUtils.h"

namespace ztx::theme
{
juce::Font mono (float size, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font label (float size)
{
    return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (0.08f);
}

void drawPod (juce::Graphics& g, juce::Rectangle<float> r, int seed, juce::Colour fill, float alpha)
{
    Rng rng ((uint32_t) seed * 2654435761u + 7u);
    const float rad = std::min (14.0f, std::min (r.getWidth(), r.getHeight()) * 0.25f);
    juce::Path base;
    base.addRoundedRectangle (r, rad);
    const float L = base.getLength();
    const int pts = std::max (24, (int) (L / 18.0f));
    const auto centre = r.getCentre();
    juce::Path p;
    for (int i = 0; i < pts; ++i)
    {
        auto pt = base.getPointAlongPath (L * (float) i / (float) pts);
        auto dir = centre - pt;
        const float len = std::max (0.001f, dir.getDistanceFromOrigin());
        const float wob = std::sin ((float) i * 0.45f + (float) seed) * 1.3f + rng.bi() * 0.5f;
        pt += dir / len * wob;
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    p.closeSubPath();
    p = p.createPathWithRoundedCorners (6.0f);

    g.setColour (fill.withAlpha (alpha));
    g.fillPath (p);
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (p);
        juce::ColourGradient gr (acid.withAlpha (0.035f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getRight(), r.getBottom(), true);
        g.setGradientFill (gr);
        g.fillRect (r);
        g.setColour (silver.withAlpha (0.025f));
        for (int k = 0; k < 4; ++k)
        {
            juce::Path c;
            const float y0 = r.getY() + r.getHeight() * (0.2f + 0.2f * (float) k) + rng.bi() * 6.0f;
            c.startNewSubPath (r.getX(), y0);
            for (float x = r.getX(); x <= r.getRight(); x += 12.0f)
                c.lineTo (x, y0 + std::sin (x * 0.021f + (float) (seed + k)) * 5.0f);
            g.strokePath (c, juce::PathStrokeType (1.0f));
        }
    }
    g.setColour (silver.withAlpha (0.16f));
    g.strokePath (p, juce::PathStrokeType (1.0f));
}

void drawGlitchText (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, juce::Font f, int seed,
                     juce::Justification j, float amount)
{
    Rng rng ((uint32_t) seed * 7919u + 1u);
    g.setFont (f);
    // chromatic ghosts
    g.setColour (acid.withAlpha (0.35f * amount));
    g.drawText (s, r.translated (-1.5f * amount, 0.5f), j, false);
    g.setColour (orange.withAlpha (0.22f * amount));
    g.drawText (s, r.translated (1.5f * amount, -0.5f), j, false);
    g.setColour (text);
    g.drawText (s, r, j, false);
    // displaced horizontal slices
    const int slices = (int) (3 * amount);
    for (int k = 0; k < slices; ++k)
    {
        const float y = r.getY() + rng.uni() * r.getHeight();
        const float h = 1.0f + rng.uni() * r.getHeight() * 0.18f;
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (juce::Rectangle<float> (r.getX() - 10, y, r.getWidth() + 20, h).toNearestInt());
        g.setColour (k % 2 ? acid : text.withAlpha (0.9f));
        g.drawText (s, r.translated (rng.bi() * 6.0f * amount, 0), j, false);
    }
}

juce::Image makeBackground (int w, int h)
{
    juce::Image img (juce::Image::RGB, std::max (1, w), std::max (1, h), true);
    juce::Graphics g (img);
    g.fillAll (bg);
    Rng rng (99);
    // organic blobs
    for (int i = 0; i < 9; ++i)
    {
        const float cx = rng.uni() * (float) w, cy = rng.uni() * (float) h, rr = 120.0f + rng.uni() * 360.0f;
        const auto c = (i % 3 == 0 ? orange : acid).withAlpha (i % 3 == 0 ? 0.025f : 0.04f);
        juce::ColourGradient gr (c, cx, cy, c.withAlpha (0.0f), cx + rr, cy, true);
        g.setGradientFill (gr);
        g.fillEllipse (cx - rr, cy - rr, rr * 2, rr * 2);
    }
    // contour lines
    g.setColour (acid.withAlpha (0.035f));
    for (int k = 0; k < 26; ++k)
    {
        juce::Path c;
        const float base = (float) k / 26.0f * (float) h;
        for (int x = 0; x <= w; x += 8)
        {
            const float fx = (float) x;
            const float y = base + 22.0f * std::sin (fx * 0.006f + (float) k * 0.7f) + 11.0f * std::sin (fx * 0.017f - (float) k);
            if (x == 0) c.startNewSubPath (fx, y); else c.lineTo (fx, y);
        }
        g.strokePath (c, juce::PathStrokeType (0.8f));
    }
    // grain
    juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const float nz = rng.bi() * 6.0f;
            auto px = bd.getPixelColour (x, y);
            const auto clampC = [] (float v) { return (juce::uint8) juce::jlimit (0.0f, 255.0f, v); };
            bd.setPixelColour (x, y, juce::Colour (clampC (px.getRed() + nz), clampC (px.getGreen() + nz), clampC (px.getBlue() + nz * 0.8f)));
        }
    return img;
}
} // namespace ztx::theme
