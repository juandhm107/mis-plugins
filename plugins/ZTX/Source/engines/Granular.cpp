// ENGINE C - GRANULAR ORGANISM
// Up to 40 overlapping grains read from the loaded / captured buffer. Position, spray,
// scan, freeze, per-grain pitch scatter, reverse probability, pan spread and a window
// that morphs percussive -> Hann -> flat. Always gated by note length.
#include "Engine.h"

namespace ztx
{
namespace
{
struct Grain
{
    bool active = false;
    double pos = 0;
    float inc = 1, gl = 0.7f, gr = 0.7f;
    int len = 1, age = 0;
};

class Granular final : public EngineVoice
{
public:
    void prepare (float sampleRate) override { sr = sampleRate; }

    void start (float, float vel, const float* p, uint32_t seed) override
    {
        rng.seed (seed);
        velocity = vel; counter = 0; scan = 0;
        for (auto& g : grains) g.active = false;
        startPos = p[grPos];
    }

    void render (float* L, float* R, int n, const float* p, float f, const EngineCtx& ctx) override
    {
        const SampleData* sd = ctx.sample;
        if (sd == nullptr || sd->buffer.getNumSamples() < 64) return;
        const int len = sd->buffer.getNumSamples();
        const float* dl = sd->buffer.getReadPointer (0);
        const float* dr = sd->buffer.getReadPointer (sd->buffer.getNumChannels() > 1 ? 1 : 0);

        const float sizeS = p[grSize] * 0.001f;
        const float density = p[grDensity];
        const float spray = p[grSpray], pitchRnd = p[grPitchRnd], rev = p[grReverse];
        const float spread = p[grSpread], shape = p[grEnv];
        const bool freeze = p[grFreeze] > 0.5f;
        const float baseRate = (f / 261.6256f) * semis (p[grPitch]) * (float) (sd->sampleRate / sr);
        const float overlap = std::max (1.0f, density * sizeS);
        const float norm = 2.2f / std::sqrt (overlap);
        const float period = sr / density;

        if (! freeze) scan += p[grScan] * 0.25f * (float) n / sr;
        const float centre = freeze ? p[grPos] : p[grPos] + scan;

        for (int i = 0; i < n; ++i)
        {
            if (--counter <= 0)
            {
                counter = period * (1.0f + rng.bi() * 0.25f * (0.3f + spray));
                spawn (centre, spray, pitchRnd, rev, spread, baseRate, sizeS, len);
            }
            float l = 0, r = 0;
            for (auto& g : grains)
            {
                if (! g.active) continue;
                const float x = (float) g.age / (float) g.len;
                const float w = window (x, shape);
                double ip = std::floor (g.pos);
                const float fr = (float) (g.pos - ip);
                int i0 = (int) ip % len; if (i0 < 0) i0 += len;
                const int i1 = (i0 + 1) % len;
                const float sl = dl[i0] + (dl[i1] - dl[i0]) * fr;
                const float sr_ = dr[i0] + (dr[i1] - dr[i0]) * fr;
                l += sl * w * g.gl;
                r += sr_ * w * g.gr;
                g.pos += g.inc;
                if (++g.age >= g.len) g.active = false;
            }
            L[i] += l * norm * velocity;
            R[i] += r * norm * velocity;
        }
    }

    bool finished() const override { return false; }   // voice ADSR decides

private:
    static float window (float x, float s)
    {
        const float hann = 0.5f - 0.5f * std::cos (kTwoPi * x);
        if (s < 0.5f)
        {
            const float perc = x < 0.03f ? x / 0.03f : std::pow (1.0f - (x - 0.03f) / 0.97f, 2.5f);
            return lerp (perc, hann, s * 2.0f);
        }
        const float edge = 0.08f;
        float tuk = 1.0f;
        if (x < edge) tuk = 0.5f - 0.5f * std::cos (kPi * x / edge);
        else if (x > 1.0f - edge) tuk = 0.5f - 0.5f * std::cos (kPi * (1.0f - x) / edge);
        return lerp (hann, tuk, (s - 0.5f) * 2.0f);
    }

    void spawn (float centre, float spray, float pitchRnd, float rev, float spread, float baseRate, float sizeS, int len)
    {
        for (auto& g : grains)
        {
            if (g.active) continue;
            float pos01 = centre + spray * rng.bi() * 0.5f;
            pos01 -= std::floor (pos01);
            float rate = baseRate * semis (pitchRnd * rng.bi() * 12.0f);
            g.len = std::max (16, (int) (sizeS * sr * (1.0f + spray * rng.bi() * 0.3f)));
            g.pos = (double) pos01 * len;
            if (rng.uni() < rev) { rate = -rate; g.pos += (double) g.len * -rate; }
            g.inc = rate;
            const float pan = spread * rng.bi();
            g.gl = std::sqrt (0.5f * (1.0f - pan));
            g.gr = std::sqrt (0.5f * (1.0f + pan));
            g.age = 0;
            g.active = true;
            return;
        }
    }

    float sr = 48000, velocity = 1, counter = 0, scan = 0, startPos = 0;
    Grain grains[40];
    Rng rng;
};
} // namespace

std::unique_ptr<EngineVoice> makeGranular() { return std::make_unique<Granular>(); }
} // namespace ztx
