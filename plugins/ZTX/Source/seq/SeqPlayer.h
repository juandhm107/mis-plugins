#pragma once
#include "Pattern.h"
#include "../Params.h"

namespace ztx
{
struct SeqEvent
{
    int sampleOffset = 0;
    int track = 0;
    int note = 60;
    float velocity = 1.0f;
    int gateSamples = 1000;
};

// Stateless-per-block step scheduler: everything is derived from the ppq position and a
// hash of the absolute step index, so seeking / looping never double-fires or drifts.
class SeqPlayer
{
public:
    static constexpr int kMaxEvents = 256;

    struct Settings { float swing = 0, human = 0, probScale = 1; int transpose = 0; };

    // Collect events whose onset lies in [ppqStart, ppqStart + n / samplesPerBeat).
    int collect (const Pattern& pat, double ppqStart, int n, double samplesPerBeat, const Settings& s, SeqEvent* out)
    {
        int count = 0;
        const double ppqEnd = ppqStart + (double) n / samplesPerBeat;
        for (int ti = 0; ti < kTracks; ++ti)
        {
            const Track& t = pat.tracks[(size_t) ti];
            const double stepBeats = divisionInBeats (t.div);
            const int len = juce::jlimit (1, kMaxSteps, t.length);
            const long long kFirst = (long long) std::floor (ppqStart / stepBeats) - 1;
            const long long kLast = (long long) std::floor (ppqEnd / stepBeats);
            currentStep[ti] = (int) (((kLast % len) + len) % len);
            if (t.mute) continue;
            for (long long k = std::max (0LL, kFirst); k <= kLast; ++k)
            {
                const int si = (int) (k % len);
                const Step& st = t.steps[(size_t) si];
                if (! st.on) continue;
                const uint32_t ku = (uint32_t) k;
                double offset = st.micro * stepBeats;
                if (k & 1) offset += s.swing * 0.5 * stepBeats;
                if (s.human > 0) offset += s.human * hash01 (ku, (uint32_t) ti, 11u) * 0.12 * stepBeats;
                const double onset = (double) k * stepBeats + offset;
                if (hash01 (ku, (uint32_t) ti, 7u) >= st.prob * s.probScale) continue;
                const int rat = juce::jlimit (1, 4, st.ratchet);
                for (int j = 0; j < rat; ++j)
                {
                    const double tj = onset + (double) j * stepBeats / rat;
                    if (tj < ppqStart || tj >= ppqEnd || count >= kMaxEvents) continue;
                    SeqEvent& e = out[count++];
                    e.sampleOffset = juce::jlimit (0, n - 1, (int) ((tj - ppqStart) * samplesPerBeat));
                    e.track = ti;
                    e.note = juce::jlimit (0, 127, t.note + st.pitch + s.transpose);
                    float v = st.vel * (1.0f - (float) j * 0.18f);
                    if (s.human > 0) v *= 1.0f - s.human * 0.35f * hash01 (ku, (uint32_t) ti, 13u);
                    e.velocity = juce::jlimit (0.02f, 1.0f, v);
                    e.gateSamples = std::max (32, (int) (t.gate * stepBeats / rat * samplesPerBeat));
                    hitFlash[ti] = 1.0f;
                }
            }
        }
        return count;
    }

    int currentStep[kTracks] {};
    float hitFlash[kTracks] {};
};
} // namespace ztx
