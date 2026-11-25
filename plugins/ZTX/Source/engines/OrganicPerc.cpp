// ENGINE E - ORGANIC PERCUSSION
// Procedural drum: swept sine body + transient click + modal material shell +
// band-passed noise + granular "rattle" for shakers and seed pods.
#include "Engine.h"

namespace ztx
{
namespace
{
const float kMat[4][4] = {
    { 1.0f, 1.59f, 2.14f, 2.65f },   // skin / membrane
    { 1.0f, 2.57f, 4.40f, 6.90f },   // wood
    { 1.0f, 3.00f, 5.00f, 7.00f },   // hollow tube (odd)
    { 1.0f, 2.32f, 3.71f, 5.43f },   // metal plate
};
const float kMatDecay[4] = { 0.6f, 0.8f, 1.6f, 3.0f };

class OrganicPerc final : public EngineVoice
{
public:
    void prepare (float sampleRate) override { sr = sampleRate; }

    void start (float, float vel, const float*, uint32_t seed) override
    {
        rng.seed (seed);
        velocity = vel; t = 0; phase = 0; done = false; clickPending = true;
        for (auto& m : modes) m.reset();
        noiseF.reset(); hpN.reset(); rattleAmp = 0; quiet = 0;
        dc.reset();
    }

    void render (float* L, float* R, int n, const float* p, float f, const EngineCtx&) override
    {
        if (done) return;
        const float decay = p[pcDecay], trans = p[pcTransient], reso = p[pcResonance];
        const float noise = p[pcNoise], body = p[pcBody], mat = p[pcMaterial];
        const float sweep = p[pcSweep], rattle = p[pcRattle], tone = p[pcTone];
        const float f0 = f * semis (p[pcPitch]);
        const float inv = 1.0f / sr;

        const float mt = mat * 3.0f;
        const int mi = std::min (2, (int) mt);
        const float mf = mt - (float) mi;
        const float matDec = lerp (kMatDecay[mi], kMatDecay[mi + 1], mf);
        for (int m = 0; m < 4; ++m)
        {
            const float ratio = lerp (kMat[mi][m], kMat[mi + 1][m], mf);
            const float t60 = decay * (0.15f + reso * 2.2f) * matDec / (1.0f + (float) m * 0.6f);
            modes[m].set (f0 * ratio * (mat > 0.5f ? 2.0f : 1.0f), t60, sr);
        }
        noiseF.set (300.0f + tone * tone * 12000.0f, 0.6f + reso * 3.0f, sr);
        hpN.setFreq (2500.0f + tone * 5000.0f, sr);
        const float bodyCoef = std::exp (-6.9078f / (decay * sr));
        const float noiseCoef = std::exp (-6.9078f / (decay * 0.6f * sr));
        const float sweepRate = 18.0f + 40.0f * (1.0f - sweep * 0.5f);

        float peak = 0;
        for (int i = 0; i < n; ++i)
        {
            t += inv;
            if (t < 0.001f) { bodyEnv = std::min (1.0f, t / 0.001f); noiseEnv = bodyEnv; }
            else { bodyEnv *= bodyCoef; noiseEnv *= noiseCoef; }

            const float fr = f0 * (1.0f + sweep * 5.0f * std::exp (-t * sweepRate));
            phase += fr * inv; phase -= std::floor (phase);
            const float bodySig = std::sin (kTwoPi * phase) * bodyEnv * body;

            float exc = 0;
            if (clickPending) { exc = 1.0f; clickPending = false; }
            const float nz = rng.bi();
            if (t < 0.002f) exc += nz * 0.5f;
            const float clickSig = (exc * 0.8f + hpN.hp (nz) * (t < 0.003f ? 1.0f : 0.0f)) * trans;

            float m = 0;
            for (int k = 0; k < 4; ++k) m += modes[k].process (exc * (0.4f + trans * 0.6f)) * (1.0f / (1.0f + (float) k * 0.5f));
            const float modal = m * (0.05f + reso * 0.45f);

            noiseF.process (nz);
            const float noiseSig = noiseF.bp * noiseEnv * noise * 1.2f;

            float rat = 0;
            if (rattle > 0.001f)
            {
                const float win = std::exp (-t / (decay * 1.2f + 0.02f));
                if (rng.uni() < rattle * 0.006f) rattleAmp = 0.4f + rng.uni() * 0.6f;
                rattleAmp *= 0.9985f - 0.002f * (1.0f - tone);
                rat = hpN.hp (rng.bi()) * rattleAmp * win * rattle * 0.9f;
            }

            float s = bodySig + clickSig + modal + noiseSig + rat;
            s = fastTanh (s * 1.1f);
            s = dc.process (s);
            const float spread = 0.15f * (modal - rat);
            L[i] += (s - spread) * velocity;
            R[i] += (s + spread) * velocity;
            peak = std::max (peak, std::abs (s));
        }
        if (t > 0.02f && peak < 1.0e-4f) { if (++quiet > 32) done = true; } else quiet = 0;
    }

    bool finished() const override { return done; }

private:
    float sr = 48000, t = 0, phase = 0, velocity = 1, bodyEnv = 0, noiseEnv = 0, rattleAmp = 0;
    bool clickPending = true, done = false;
    int quiet = 0;
    Mode modes[4];
    Svf noiseF;
    OnePole hpN;
    DcBlock dc;
    Rng rng;
};
} // namespace

std::unique_ptr<EngineVoice> makePerc() { return std::make_unique<OrganicPerc>(); }
} // namespace ztx
