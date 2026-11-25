// ENGINE B - MICRO GLITCH
// One trigger opens a "burst window". A tempo-locked micro grid fires tiny events
// (clicks, crackles, blips, digital noise, slices of its own past output) with
// probability, jitter and micro-stutter repeats, then bit/rate destruction.
#include "Engine.h"

namespace ztx
{
namespace
{
enum EvType { EvClick, EvCrackle, EvBlip, EvStutter, EvDigital, NUM_EV };

struct Event
{
    bool active = false;
    int type = 0, wait = 0, pos = 0, len = 0, repeatsLeft = 0, gap = 0;
    float freq = 1000, amp = 0, pan = 0, phase = 0, speed = 1, held = 0, readPos = 0;
    bool reverse = false;
    int sliceStart = 0;
};

class MicroGlitch final : public EngineVoice
{
public:
    void prepare (float sampleRate) override
    {
        sr = sampleRate;
        ring.assign (kRing, 0.0f);
    }

    void start (float f, float vel, const float*, uint32_t seed) override
    {
        rng.seed (seed);
        velocity = vel; t = 0; tickCounter = 0; done = false;
        for (auto& e : ev) e.active = false;
        const float note = 12.0f * std::log2 (std::max (f, 8.0f) / 32.70f);
        baseType = ((int) std::lround (note) % NUM_EV + NUM_EV) % NUM_EV;
        holdL = holdR = 0; holdCount = 0;
    }

    void render (float* L, float* R, int n, const float* p, float f, const EngineCtx& ctx) override
    {
        if (done) return;
        const float density = p[glDensity], grainMs = p[glGrain], randomness = p[glRandom];
        const float repeat = p[glRepeat], bits = p[glBits], rateRed = p[glRate];
        const float bufAmt = p[glBuffer], prob = p[glProb], timing = p[glTiming];
        const float decay = p[glDecay], tone = p[glTone];
        const float fPitch = f * semis (p[glPitch]);
        const int ticksPerBeat = 1 << (1 + (int) std::lround (density * 5.0f));     // 2..64
        const int tickPeriod = std::max (8, (int) (60.0 / std::max (20.0, ctx.bpm) / ticksPerBeat * sr));
        const int grainLen = std::max (8, (int) (grainMs * 0.001f * sr));
        lp.setFreq (700.0f + tone * tone * 17000.0f, sr);
        hp.setFreq (15.0f + tone * 400.0f, sr);
        lpR.a = lp.a; hpR.a = hp.a;
        const float holdFactor = 1.0f + rateRed * rateRed * 48.0f;
        const float levels = std::exp2 (bits - 1.0f);
        const float inv = 1.0f / sr;

        bool anyActive = false;
        for (int i = 0; i < n; ++i)
        {
            t += inv;
            const bool open = t < decay;
            if (open && tickCounter <= 0)
            {
                tickCounter += tickPeriod;
                const float windowEnv = 1.0f - t / decay;
                if (t <= inv * 1.5f || rng.uni() < prob * (0.4f + 0.6f * windowEnv))
                    spawn (fPitch, grainLen, tickPeriod, randomness, timing, bufAmt, repeat, windowEnv);
                if (timing > 0.3f && rng.uni() < (timing - 0.3f) * 0.4f)
                    spawn (fPitch, grainLen, tickPeriod, randomness, 1.0f, bufAmt, repeat, windowEnv * 0.6f);
            }
            --tickCounter;

            float l = 0, r = 0;
            for (auto& e : ev)
            {
                if (! e.active) continue;
                anyActive = true;
                if (e.wait > 0) { --e.wait; continue; }
                const float x = (float) e.pos / (float) e.len;
                const float s = renderEvent (e, x) * e.amp;
                l += s * (1.0f - e.pan) * 0.7f;
                r += s * (1.0f + e.pan) * 0.7f;
                if (++e.pos >= e.len)
                {
                    if (e.repeatsLeft > 0)
                    {
                        --e.repeatsLeft; e.pos = 0; e.wait = e.gap; e.amp *= 0.82f; e.phase = 0;
                        e.readPos = 0;
                        if (randomness > 0.5f && rng.chance (0.3f)) e.freq *= rng.chance (0.5f) ? 2.0f : 0.5f;
                    }
                    else e.active = false;
                }
            }

            // sample-rate + bit reduction
            if (--holdCount <= 0) { holdL = l; holdR = r; holdCount = (int) holdFactor; }
            l = holdL; r = holdR;
            if (bits < 15.9f)
            {
                l = std::round (l * levels) / levels;
                r = std::round (r * levels) / levels;
            }
            l = hp.hp (lp.lp (l));
            r = hpR.hp (lpR.lp (r));
            ring[(size_t) wpos] = (l + r) * 0.5f;
            wpos = (wpos + 1) & (kRing - 1);

            L[i] += l * velocity * 2.2f;
            R[i] += r * velocity * 2.2f;
        }
        if (t > decay && ! anyActive) done = true;
    }

    bool finished() const override { return done; }

private:
    void spawn (float f, int grainLen, int tickPeriod, float rnd, float timing, float bufAmt, float repeat, float level)
    {
        for (auto& e : ev)
        {
            if (e.active) continue;
            e = Event();
            e.active = true;
            int type = baseType;
            if (rng.uni() < rnd) type = rng.range (0, NUM_EV - 1);
            if (rng.uni() < bufAmt * 0.6f) type = EvStutter;
            e.type = type;
            e.wait = (int) (timing * rng.uni() * (float) tickPeriod * 0.9f);
            const float lenScale = 1.0f + rnd * rng.bi() * 0.8f;
            e.len = std::max (6, (int) ((float) grainLen * lenScale * (type == EvStutter ? 3.0f : 1.0f)));
            const float pitchJ = semis (rnd * rng.bi() * 19.0f);
            static const float mult[NUM_EV] = { 14.0f, 1.0f, 4.0f, 1.0f, 9.0f };
            e.freq = f * mult[type] * pitchJ;
            e.amp = (0.45f + 0.35f * rng.uni() * rnd) * (0.35f + 0.65f * level);
            e.pan = rng.bi() * (0.25f + 0.6f * rnd);
            e.reverse = rng.uni() < bufAmt * 0.7f;
            e.speed = semis (rnd * rng.bi() * 12.0f) * (rng.chance (0.2f * rnd) ? 0.5f : 1.0f);
            e.sliceStart = wpos - e.len * 2 - rng.range (0, std::min (kRing / 2, tickPeriod * 4));
            if (rng.uni() < repeat)
            {
                e.repeatsLeft = 1 + (int) (repeat * rng.uni() * 7.0f);
                e.gap = (int) ((float) e.len * (0.1f + rng.uni() * 0.9f));
            }
            return;
        }
    }

    float renderEvent (Event& e, float x)
    {
        const float inv = 1.0f / sr;
        switch (e.type)
        {
            case EvClick:
            {
                e.phase += e.freq * inv; e.phase -= std::floor (e.phase);
                const float env = std::exp (-x * 7.0f);
                return (e.pos == 0 ? 1.0f : 0.0f) + std::sin (kTwoPi * e.phase) * env * 0.8f;
            }
            case EvCrackle:
            {
                const float env = 1.0f - x;
                return rng.uni() < 0.18f ? rng.bi() * env * 1.3f : 0.0f;
            }
            case EvBlip:
            {
                const float fr = e.freq * (1.0f + 3.0f * std::exp (-x * 9.0f));
                e.phase += fr * inv; e.phase -= std::floor (e.phase);
                const float env = std::sin (kPi * x);
                return (e.phase < 0.5f ? 0.6f : -0.6f) * env;
            }
            case EvStutter:
            {
                e.readPos += e.speed;
                const int idx = e.reverse ? e.sliceStart + e.len - (int) e.readPos : e.sliceStart + (int) e.readPos;
                const float env = std::min (1.0f, std::min (x * 20.0f, (1.0f - x) * 20.0f));
                return ring[(size_t) (idx & (kRing - 1))] * 1.6f * env;
            }
            case EvDigital: default:
            {
                e.phase += e.freq * inv;
                if (e.phase >= 1.0f) { e.phase -= std::floor (e.phase); e.held = rng.chance (0.5f) ? 0.5f : -0.5f; }
                return e.held * (1.0f - x);
            }
        }
    }

    static constexpr int kRing = 16384;
    float sr = 48000, t = 0, velocity = 1, holdL = 0, holdR = 0;
    int tickCounter = 0, holdCount = 0, baseType = 0, wpos = 0;
    bool done = false;
    Event ev[8];
    std::vector<float> ring;
    OnePole lp, hp, lpR, hpR;
    Rng rng;
};
} // namespace

std::unique_ptr<EngineVoice> makeGlitch() { return std::make_unique<MicroGlitch>(); }
} // namespace ztx
