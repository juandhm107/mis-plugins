// ENGINE A - RUBBER LAB
// Elastic phase-modulated core with pitch "stretch" + damped bounce wobble, struck
// through a 4-mode material body. Friction adds stick-slip impulses while sounding.
#include "Engine.h"

namespace ztx
{
namespace
{
const float kRatios[3][4] = {
    { 1.0f, 1.58f, 2.24f, 2.92f },   // rubber / membrane
    { 1.0f, 2.57f, 4.21f, 6.30f },   // wood
    { 1.0f, 2.76f, 5.40f, 8.93f },   // metal bar
};

class RubberLab final : public EngineVoice
{
public:
    void prepare (float sampleRate) override { sr = sampleRate; }

    void start (float f, float vel, const float*, uint32_t seed) override
    {
        rng.seed (seed);
        t = 0; phase = 0; bouncePhase = 0; velocity = vel;
        coreEnv = 0; clickPending = true; quietBlocks = 0; done = false;
        for (auto& m : modes) m.reset();
        dc.reset(); scrape.reset();
        baseFreq = f;
        detune = 1.0f + rng.bi() * 0.003f;
    }

    void render (float* L, float* R, int n, const float* p, float f, const EngineCtx&) override
    {
        if (done) return;
        const float elastic = p[rbElastic], tension = p[rbTension], damping = p[rbDamping];
        const float reso = p[rbResonance], impact = p[rbImpact], friction = p[rbFriction];
        const float decay = p[rbDecay], material = p[rbMaterial], drive = p[rbDrive];
        const float fBase = f * semis (p[rbPitch]) * detune;
        const float inv = 1.0f / sr;

        const float snap = 3.0f + tension * 55.0f;                // stretch release speed
        const float bounceHz = 3.0f + tension * 28.0f + elastic * 6.0f;
        const float bounceDamp = 1.2f + damping * 14.0f;
        const float decayCoef = std::exp (-6.9078f / (decay * sr));
        const float attackInc = 1.0f / (0.0015f * sr);
        const float fmIdx = tension * 2.8f + material * 0.6f;
        const float fmRatio = 1.0f + material * 1.76f + (1.0f - material) * 0.0f;
        const float fmDecayRate = 5.0f + damping * 35.0f;
        const float excLen = 0.0004f + (1.0f - impact) * 0.008f;
        const float driveGain = 1.0f + drive * 7.0f;

        // modal body follows the stretch slightly -> rubbery modes
        const float stretchNow = std::exp2 (elastic * 2.0f * std::exp (-t * snap));
        const float modeF = fBase * std::pow (stretchNow, 0.35f + elastic * 0.3f);
        const float mt = material * 2.0f;
        const int mi = std::min (1, (int) mt);
        const float mf = mt - (float) mi;
        for (int m = 0; m < 4; ++m)
        {
            const float ratio = lerp (kRatios[mi][m], kRatios[mi + 1][m], mf);
            const float t60 = decay * (0.12f + reso * 1.3f) * (0.5f + material * 1.6f)
                              / (1.0f + (float) m * damping * 3.0f);
            modes[m].set (modeF * ratio, t60, sr);
        }
        scrape.setFreq (600.0f + tension * 3000.0f, sr);

        float peak = 0;
        for (int i = 0; i < n; ++i)
        {
            t += inv;
            // pitch stretch (octaves) + damped bounce
            const float stretch = std::exp2 (elastic * 2.0f * std::exp (-t * snap));
            const float wobble = elastic * 0.22f * std::exp (-t * bounceDamp) * std::sin (bouncePhase);
            bouncePhase += kTwoPi * bounceHz * inv;
            if (bouncePhase > kTwoPi) bouncePhase -= kTwoPi;
            const float freq = fBase * stretch * (1.0f + wobble);
            phase += freq * inv;
            phase -= std::floor (phase);

            // amp envelope
            if (t < 0.0015f) coreEnv = std::min (1.0f, coreEnv + attackInc);
            else coreEnv *= decayCoef;

            const float fmEnv = std::exp (-t * fmDecayRate);
            const float ph = kTwoPi * phase;
            const float core = std::sin (ph + fmIdx * fmEnv * std::sin (ph * fmRatio) + reso * lastModal * 2.0f);

            // exciter: click + short noise burst + friction stick-slip
            float exc = 0;
            if (clickPending) { exc += impact * 1.2f; clickPending = false; }
            if (t < excLen) exc += rng.bi() * impact * 0.6f * (1.0f - t / excLen);
            if (friction > 0.001f)
            {
                if (rng.uni() < friction * friction * 0.025f) exc += rng.bi() * friction * 0.7f * coreEnv;
                exc += scrape.lp (rng.bi()) * friction * 0.12f * coreEnv;
            }

            float m0 = modes[0].process (exc);
            float m1 = modes[1].process (exc) * 0.7f;
            float m2 = modes[2].process (exc) * 0.5f;
            float m3 = modes[3].process (exc) * 0.35f;
            lastModal = (m0 + m1 + m2 + m3) * 0.25f;

            const float coreOut = core * coreEnv * (0.85f - 0.35f * reso);
            const float bodyAmt = 0.08f + 0.35f * reso;
            float l = coreOut + (m0 + m2 * 1.2f + m1 * 0.6f) * bodyAmt;
            float r = coreOut + (m0 + m1 * 1.2f + m3 * 1.1f) * bodyAmt;

            // asymmetric drive
            if (drive > 0.001f)
            {
                l = fastTanh (l * driveGain + drive * 0.35f) / (1.0f + drive * 1.2f);
                r = fastTanh (r * driveGain + drive * 0.35f) / (1.0f + drive * 1.2f);
            }
            l = dc.process (l);
            r = dc2.process (r);

            L[i] += l * velocity;
            R[i] += r * velocity;
            peak = std::max (peak, std::abs (l) + std::abs (r));
        }
        if (t > 0.03f && peak < 2.0e-4f) { if (++quietBlocks > 64) done = true; }
        else quietBlocks = 0;
    }

    bool finished() const override { return done; }

private:
    float sr = 48000, t = 0, phase = 0, bouncePhase = 0, velocity = 1, coreEnv = 0;
    float baseFreq = 100, detune = 1, lastModal = 0;
    bool clickPending = true, done = false;
    int quietBlocks = 0;
    Mode modes[4];
    DcBlock dc, dc2;
    OnePole scrape;
    Rng rng;
};
} // namespace

std::unique_ptr<EngineVoice> makeRubber() { return std::make_unique<RubberLab>(); }
} // namespace ztx
