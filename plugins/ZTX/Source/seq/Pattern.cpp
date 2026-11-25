#include "Pattern.h"

namespace ztx
{
Pattern::Pattern()
{
    const int engines[kTracks] = { 4, 0, 1, 3 };
    const int notes[kTracks]   = { 36, 43, 72, 60 };
    const float pans[kTracks]  = { 0.0f, -0.2f, 0.25f, 0.1f };
    for (int t = 0; t < kTracks; ++t)
    {
        tracks[(size_t) t].engine = engines[t];
        tracks[(size_t) t].note = notes[t];
        tracks[(size_t) t].pan = pans[t];
    }
    applyEuclid (tracks[0], 4, 0);
    lane.fill (0.5f);
}

std::array<bool, kMaxSteps> euclid (int hits, int length, int rotation)
{
    std::array<bool, kMaxSteps> out {};
    length = juce::jlimit (1, kMaxSteps, length);
    hits = juce::jlimit (0, length, hits);
    if (hits == 0) return out;
    for (int i = 0; i < length; ++i)
    {
        const int idx = ((i + rotation) % length + length) % length;
        out[(size_t) idx] = ((i * hits) % length) < hits;
    }
    return out;
}

void clearTrack (Track& t)
{
    for (auto& s : t.steps) s = Step();
}

void applyEuclid (Track& t, int hits, int rotation)
{
    t.euclidHits = hits; t.euclidRot = rotation;
    auto e = euclid (hits, t.length, rotation);
    for (int i = 0; i < kMaxSteps; ++i)
    {
        auto& s = t.steps[(size_t) i];
        s.on = i < t.length && e[(size_t) i];
        if (s.on && s.vel < 0.05f) s.vel = 0.8f;
    }
}

namespace
{
// Role per track: 0 pulse/low, 1 counter, 2 micro/hat, 3 texture
struct Role { float base, span, pitchSpread; };
const Role kRoles[kTracks] = { { 0.22f, 0.30f, 0.2f }, { 0.20f, 0.35f, 0.5f }, { 0.45f, 0.50f, 0.6f }, { 0.12f, 0.45f, 1.0f } };

const int kOddLengths16[] = { 12, 13, 14, 15, 10, 11, 9, 7, 5, 6 };
const int kOddLengths32[] = { 24, 28, 27, 20, 21, 30, 31, 18, 13, 12 };
const int kPitchLow[] = { 0, 0, 0, 0, 12, 7, -12, 0, 5, 3 };
const int kPitchHigh[] = { 0, 12, 7, -5, 5, 3, 10, -12, 19, 24, 1, -1 };

float accentFor (int i, float groove, Rng& rng, float chaos)
{
    float v = 0.62f;
    if (i % 4 == 0) v += (1.0f - groove) * 0.3f;
    if (i % 4 == 2) v += groove * 0.25f;
    if (i % 2 == 1) v -= 0.08f + groove * 0.06f;
    if (i % 4 == 3) v += groove * 0.12f;
    v += rng.bi() * (0.08f + chaos * 0.3f);
    return juce::jlimit (0.15f, 1.0f, v);
}
} // namespace

void generatePattern (Pattern& pat, const GenSettings& g, Rng& rng)
{
    for (int ti = 0; ti < kTracks; ++ti)
    {
        Track& t = pat.tracks[(size_t) ti];
        if (t.lock) continue;
        clearTrack (t);
        const Role& role = kRoles[ti];

        // length & division (polymeter from complexity)
        t.length = g.maxSteps;
        if (ti > 0 && rng.uni() < (g.complexity - 0.45f) * 1.4f)
            t.length = g.maxSteps == 32 ? kOddLengths32[rng.range (0, 9)] : kOddLengths16[rng.range (0, 9)];
        t.div = 6;
        if (ti >= 2 && rng.uni() < g.complexity * 0.3f) t.div = rng.chance (0.5f) ? 7 : 10;
        if (ti == 3 && rng.uni() < (1.0f - g.density) * 0.4f) t.div = 5;

        const float dens = juce::jlimit (0.03f, 0.95f, role.base + (g.density - 0.5f) * role.span * 2.0f + rng.bi() * 0.06f);
        int hits = juce::jlimit (1, t.length, (int) std::lround (dens * (float) t.length));
        int rot = 0;
        if (ti == 1) rot = (int) std::lround (g.groove * 3.0f) + (rng.chance (g.chaos) ? rng.range (0, 3) : 0);
        else if (ti >= 2) rot = rng.range (0, std::max (1, (int) (g.groove * 4.0f + g.chaos * 6.0f)));
        auto base = euclid (hits, t.length, rot);
        t.euclidHits = hits; t.euclidRot = rot;

        // secondary euclid layer
        if (rng.uni() < g.complexity * 0.7f && ti > 0)
        {
            auto layer = euclid (std::max (1, hits / 2 + rng.range (0, 2)), t.length, rng.range (0, t.length - 1));
            for (int i = 0; i < t.length; ++i)
                if (layer[(size_t) i] && rng.uni() < 0.5f + g.complexity * 0.4f) base[(size_t) i] = true;
        }
        // chaos flips
        for (int i = 0; i < t.length; ++i)
            if (rng.uni() < g.chaos * 0.28f) base[(size_t) i] = ! base[(size_t) i];

        for (int i = 0; i < t.length; ++i)
        {
            Step& s = t.steps[(size_t) i];
            s.on = base[(size_t) i];
            s.vel = accentFor (i, g.groove, rng, g.chaos);
            s.prob = 1.0f;
            if (i % 4 != 0 && rng.uni() < g.chaos * 0.7f) s.prob = juce::jlimit (0.15f, 1.0f, 1.0f - rng.uni() * (0.25f + g.chaos * 0.55f));
            if (s.on && rng.uni() < g.complexity * (ti == 2 ? 0.28f : 0.12f)) s.ratchet = rng.range (2, ti == 2 ? 4 : 3);
            if (rng.uni() < g.chaos * 0.45f) s.micro = rng.uni() * (0.08f + g.chaos * 0.3f);
            if (ti == 1 && i % 2 == 1) s.micro += g.groove * 0.07f;
            if (rng.uni() < g.complexity * role.pitchSpread * 0.6f)
                s.pitch = ti == 0 ? kPitchLow[rng.range (0, 9)] : kPitchHigh[rng.range (0, 11)];
        }

        // repetition: copy the first cell onto later cells (motif identity)
        const int cell = g.repetition > 0.7f ? 4 : 8;
        if (t.length >= cell * 2)
            for (int c = 1; c * cell < t.length; ++c)
                if (rng.uni() < g.repetition * 0.9f)
                    for (int k = 0; k < cell && c * cell + k < t.length; ++k)
                    {
                        const auto& src = t.steps[(size_t) k];
                        auto& dst = t.steps[(size_t) (c * cell + k)];
                        dst.on = src.on; dst.ratchet = src.ratchet; dst.pitch = src.pitch;
                        if (rng.uni() < g.repetition) dst.prob = src.prob;
                    }

        if (ti == 0) t.steps[0].on = true;     // anchor downbeat
        t.steps[0].prob = 1.0f;
    }

    // step modulation lane: smooth walk, repetition makes it periodic
    float v = rng.uni();
    for (int i = 0; i < kLaneSteps; ++i)
    {
        if (i >= 4 && rng.uni() < g.repetition * 0.7f) v = pat.lane[(size_t) (i - 4)];
        else v = juce::jlimit (0.0f, 1.0f, v + rng.bi() * (0.15f + g.chaos * 0.5f));
        pat.lane[(size_t) i] = v;
    }
}

void mutatePattern (Pattern& pat, const GenSettings& g, Rng& rng, float strength)
{
    for (int ti = 0; ti < kTracks; ++ti)
    {
        Track& t = pat.tracks[(size_t) ti];
        if (t.lock) continue;
        const int changes = juce::jmax (1, (int) std::lround ((1.0f + g.chaos * 3.0f + g.complexity) * strength));
        for (int c = 0; c < changes; ++c)
        {
            const int i = rng.range (0, t.length - 1);
            Step& s = t.steps[(size_t) i];
            const float op = rng.uni();
            if (op < 0.35f)
            {
                // flip - but protect strong beats more (identity)
                const bool strong = (i % 4 == 0);
                if (! (strong && s.on && rng.uni() < 0.8f)) s.on = ! s.on;
                if (ti == 0 && i == 0) s.on = true;
            }
            else if (op < 0.5f) s.vel = juce::jlimit (0.15f, 1.0f, s.vel + rng.bi() * 0.25f);
            else if (op < 0.62f) { if (s.on) s.ratchet = s.ratchet > 1 ? 1 : rng.range (2, 3); }
            else if (op < 0.74f) s.prob = juce::jlimit (0.2f, 1.0f, s.prob + rng.bi() * 0.3f);
            else if (op < 0.84f)
            {
                const int j = rng.range (0, t.length - 1);
                std::swap (t.steps[(size_t) i], t.steps[(size_t) j]);
                if (ti == 0) t.steps[0].on = true;
            }
            else if (op < 0.94f) s.pitch = rng.chance (0.5f) ? 0 : (ti == 0 ? kPitchLow[rng.range (0, 9)] : kPitchHigh[rng.range (0, 11)]);
            else s.micro = juce::jlimit (0.0f, 0.5f, s.micro + rng.bi() * 0.1f);
        }
    }
    for (auto& v : pat.lane)
        if (rng.uni() < 0.25f * strength) v = juce::jlimit (0.0f, 1.0f, v + rng.bi() * 0.2f);
}

// ------------------------------------------------------------------------------------
juce::ValueTree patternToTree (const Pattern& p, const juce::Identifier& type)
{
    juce::ValueTree v (type);
    juce::String lane;
    for (float x : p.lane) lane << juce::String (x, 3) << " ";
    v.setProperty ("lane", lane.trim(), nullptr);
    for (int ti = 0; ti < kTracks; ++ti)
    {
        const Track& t = p.tracks[(size_t) ti];
        juce::ValueTree tr ("Track");
        tr.setProperty ("len", t.length, nullptr);
        tr.setProperty ("div", t.div, nullptr);
        tr.setProperty ("eng", t.engine, nullptr);
        tr.setProperty ("note", t.note, nullptr);
        tr.setProperty ("gate", t.gate, nullptr);
        tr.setProperty ("lvl", t.level, nullptr);
        tr.setProperty ("pan", t.pan, nullptr);
        tr.setProperty ("dcy", t.decayMod, nullptr);
        tr.setProperty ("col", t.colourMod, nullptr);
        tr.setProperty ("mute", t.mute, nullptr);
        tr.setProperty ("lock", t.lock, nullptr);
        tr.setProperty ("eh", t.euclidHits, nullptr);
        tr.setProperty ("er", t.euclidRot, nullptr);
        juce::String s;
        for (const auto& st : t.steps)
            s << (st.on ? 1 : 0) << "," << juce::String (st.vel, 3) << "," << juce::String (st.prob, 3) << ","
              << st.ratchet << "," << juce::String (st.micro, 3) << "," << st.pitch << ";";
        tr.setProperty ("steps", s, nullptr);
        v.appendChild (tr, nullptr);
    }
    return v;
}

void patternFromTree (Pattern& p, const juce::ValueTree& v)
{
    if (! v.isValid()) return;
    auto laneTok = juce::StringArray::fromTokens (v.getProperty ("lane").toString(), " ", "");
    for (int i = 0; i < kLaneSteps && i < laneTok.size(); ++i) p.lane[(size_t) i] = laneTok[i].getFloatValue();
    for (int ti = 0; ti < kTracks && ti < v.getNumChildren(); ++ti)
    {
        auto tr = v.getChild (ti);
        Track& t = p.tracks[(size_t) ti];
        t.length = juce::jlimit (1, kMaxSteps, (int) tr.getProperty ("len", 16));
        t.div = juce::jlimit (0, 12, (int) tr.getProperty ("div", 6));
        t.engine = juce::jlimit (0, 4, (int) tr.getProperty ("eng", 4));
        t.note = juce::jlimit (0, 127, (int) tr.getProperty ("note", 48));
        t.gate = (float) tr.getProperty ("gate", 0.5f);
        t.level = (float) tr.getProperty ("lvl", 0.8f);
        t.pan = (float) tr.getProperty ("pan", 0.0f);
        t.decayMod = (float) tr.getProperty ("dcy", 0.0f);
        t.colourMod = (float) tr.getProperty ("col", 0.0f);
        t.mute = (bool) tr.getProperty ("mute", false);
        t.lock = (bool) tr.getProperty ("lock", false);
        t.euclidHits = (int) tr.getProperty ("eh", 4);
        t.euclidRot = (int) tr.getProperty ("er", 0);
        auto steps = juce::StringArray::fromTokens (tr.getProperty ("steps").toString(), ";", "");
        for (int i = 0; i < kMaxSteps && i < steps.size(); ++i)
        {
            auto f = juce::StringArray::fromTokens (steps[i], ",", "");
            if (f.size() < 6) continue;
            Step& s = t.steps[(size_t) i];
            s.on = f[0].getIntValue() != 0;
            s.vel = f[1].getFloatValue();
            s.prob = f[2].getFloatValue();
            s.ratchet = juce::jlimit (1, 4, f[3].getIntValue());
            s.micro = f[4].getFloatValue();
            s.pitch = f[5].getIntValue();
        }
    }
}
} // namespace ztx
