#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <juce_core/juce_core.h>

namespace ztx
{
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float clamp01 (float x) { return x > 0.f ? (x < 1.f ? x : 1.f) : 0.f; }   // NaN -> 0
inline float lerp (float a, float b, float t) { return a + (b - a) * t; }
inline float semis (float s) { return std::exp2 (s * (1.0f / 12.0f)); }
inline float mtof (float note) { return 440.0f * std::exp2 ((note - 69.0f) * (1.0f / 12.0f)); }

inline float fastTanh (float x)
{
    x = std::clamp (x, -4.5f, 4.5f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float softClip (float x) { return fastTanh (x); }

// Fast, allocation-free PRNG (xorshift32).
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    explicit Rng (uint32_t seed = 0x9E3779B9u) : s (seed ? seed : 1u) {}
    void seed (uint32_t v) { s = v ? v : 1u; }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }         // [0,1)
    float bi() { return uni() * 2.0f - 1.0f; }                                   // [-1,1)
    bool chance (float p) { return uni() < p; }
    int range (int lo, int hiInclusive) { return lo + (int) (uni() * (float) (hiInclusive - lo + 1)); }
};

// Stateless hash -> [0,1) (deterministic per-step decisions in the sequencer).
inline float hash01 (uint32_t a, uint32_t b = 0, uint32_t c = 0)
{
    uint32_t h = a * 0x8da6b343u ^ b * 0xd8163841u ^ c * 0xcb1ab31fu;
    h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15;
    return (float) (h >> 8) * (1.0f / 16777216.0f);
}

// Topology-preserving state variable filter (Cytomic / Simper).
struct Svf
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;
    float lp = 0, bp = 0, hp = 0;
    void set (float freq, float q, float sr)
    {
        freq = std::clamp (freq, 8.0f, sr * 0.48f);
        const float g = std::tan (kPi * freq / sr);
        k = 1.0f / std::max (q, 0.05f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    float process (float v0)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
        return lp;
    }
    void reset() { ic1 = ic2 = 0; }
};

// Two-pole modal resonator, unity-ish peak gain.
struct Mode
{
    float b1 = 0, b2 = 0, y1 = 0, y2 = 0, g = 0;
    void set (float freq, float t60, float sr)
    {
        freq = std::clamp (freq, 10.0f, sr * 0.45f);
        const float r = std::exp (-6.9078f / (std::max (t60, 0.002f) * sr));
        const float w = kTwoPi * freq / sr;
        b1 = 2.0f * r * std::cos (w);
        b2 = -r * r;
        g = std::sin (w);          // impulse-normalised: unit impulse -> ~unit ring
    }
    float process (float x)
    {
        const float y = g * x + b1 * y1 + b2 * y2;
        y2 = y1; y1 = y;
        return y;
    }
    void reset() { y1 = y2 = 0; }
};

struct OnePole
{
    float z = 0, a = 0;
    void setFreq (float f, float sr) { a = std::exp (-kTwoPi * std::clamp (f, 1.0f, sr * 0.49f) / sr); }
    float lp (float x) { z = x + a * (z - x); return z; }
    float hp (float x) { return x - lp (x); }
    void reset() { z = 0; }
};

struct DcBlock
{
    float x1 = 0, y1 = 0;
    float process (float x) { const float y = x - x1 + 0.995f * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0; }
};

// Exponential-segment ADSR.
struct Adsr
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float value = 0, a = 0.01f, d = 0.2f, s = 1, r = 0.2f, sr = 48000;
    void set (float A, float D, float S, float R, float sampleRate) { a = A; d = D; s = S; r = R; sr = sampleRate; }
    void noteOn() { stage = Attack; }
    void noteOff() { if (stage != Idle) stage = Release; }
    void reset() { stage = Idle; value = 0; }
    bool active() const { return stage != Idle; }
    float next()
    {
        switch (stage)
        {
            case Attack:
                value += 1.0f / std::max (1.0f, a * sr);
                if (value >= 1.0f) { value = 1.0f; stage = Decay; }
                break;
            case Decay:
                value = s + (value - s) * coef (d);
                if (value - s < 1.0e-4f) { value = s; stage = Sustain; }
                break;
            case Sustain: value = s; break;
            case Release:
                value *= coef (r);
                if (value < 1.0e-5f) { value = 0; stage = Idle; }
                break;
            case Idle: default: value = 0; break;
        }
        return value;
    }
    // Advance n samples at once (control-rate use).
    float advance (int n) { float v = value; for (int i = 0; i < n; ++i) v = next(); return v; }
private:
    float coef (float t) const { return std::exp (-4.6f / std::max (1.0f, t * sr)); }
};

// Simple delay line with fractional read.
struct DelayLine
{
    std::vector<float> buf;
    int w = 0, mask = 0;
    void prepare (int maxSamples)
    {
        int n = 1; while (n < maxSamples + 4) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0;
    }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read (float delaySamples) const
    {
        delaySamples = std::clamp (delaySamples, 1.0f, (float) (mask - 2));
        const float rp = (float) w - delaySamples;
        const int i0 = (int) std::floor (rp);
        const float f = rp - (float) i0;
        const float a = buf[(size_t) (i0 & mask)];
        const float b = buf[(size_t) ((i0 + 1) & mask)];
        return a + (b - a) * f;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
};

struct SmoothedRandom
{
    float cur = 0, target = 0, phase = 1;
    float next (Rng& rng, float inc, float smooth)
    {
        phase += inc;
        if (phase >= 1.0f) { phase -= std::floor (phase); cur = target; target = rng.bi(); }
        const float t = phase;
        const float sm = t * t * (3.0f - 2.0f * t);
        const float stepped = target;
        const float glided = cur + (target - cur) * sm;
        return lerp (stepped, glided, smooth);
    }
};
} // namespace ztx
