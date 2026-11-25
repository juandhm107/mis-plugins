// ENGINE D - FM / METALLIC LAB
// 4-operator phase modulation with 8 routings, op4 feedback, inharmonic ratio stretch,
// per-operator random pitch drift, pitch-drop envelope and a nonlinear output stage.
#include "Engine.h"

namespace ztx
{
namespace
{
// mod[algo][src][dst] - src modulates dst; ops indexed 0..3 (= op1..op4)
struct Algo { bool mod[4][4]; bool carrier[4]; };

Algo makeAlgo (std::initializer_list<std::pair<int, int>> mods, std::initializer_list<int> carriers)
{
    Algo a {};
    for (auto& m : mods) a.mod[m.first - 1][m.second - 1] = true;
    for (int c : carriers) a.carrier[c - 1] = true;
    return a;
}

const Algo& algo (int i)
{
    static const Algo algos[8] = {
        makeAlgo ({ { 4, 3 }, { 3, 2 }, { 2, 1 } }, { 1 }),          // Stack
        makeAlgo ({ { 4, 2 }, { 3, 2 }, { 2, 1 } }, { 1 }),          // Twin Mod
        makeAlgo ({ { 4, 3 }, { 3, 1 }, { 2, 1 } }, { 1 }),          // Branch
        makeAlgo ({ { 4, 3 }, { 2, 1 } }, { 1, 3 }),                 // Dual Pair
        makeAlgo ({ { 4, 1 }, { 4, 2 }, { 4, 3 } }, { 1, 2, 3 }),    // Fan Out
        makeAlgo ({ { 4, 3 }, { 3, 2 } }, { 1, 2 }),                 // Sub+Stack
        makeAlgo ({}, { 1, 2, 3, 4 }),                               // Additive
        makeAlgo ({ { 2, 1 }, { 3, 1 }, { 4, 1 } }, { 1 }),          // Metal Fan
    };
    return algos[std::clamp (i, 0, 7)];
}

class FMLab final : public EngineVoice
{
public:
    void prepare (float sampleRate) override { sr = sampleRate; }

    void start (float, float vel, const float*, uint32_t seed) override
    {
        rng.seed (seed);
        velocity = vel; t = 0; done = false;
        for (int o = 0; o < 4; ++o) { phase[o] = rng.uni() * 0.0f; env[o] = 0; drift[o] = SmoothedRandom(); }
        fb1 = fb2 = 0; dc.reset(); dc2.reset();
    }

    void render (float* L, float* R, int n, const float* p, float f, const EngineCtx&) override
    {
        if (done) return;
        const Algo& a = algo ((int) p[fmAlgo]);
        const float amount = p[fmAmount];
        const float idx[4] = { 1.0f, p[fmIdx2], p[fmIdx3], p[fmIdx4] };
        const float baseR[4] = { p[fmRatio1], p[fmRatio2], p[fmRatio3], p[fmRatio4] };
        const float inharm = p[fmInharm], instab = p[fmInstab], drive = p[fmDrive];
        const float fbAmt = p[fmFeedback] * p[fmFeedback] * 1.6f;
        const float carDecay = p[fmDecay], modDecay = carDecay * p[fmModDecay];
        const float pEnv = p[fmPitchEnv], pDec = p[fmPitchDec];
        const float f0 = f * semis (p[fmPitch]);
        const float inv = 1.0f / sr;

        float ratio[4], coef[4];
        int numCarriers = 0;
        for (int o = 0; o < 4; ++o)
        {
            const float r = std::pow (baseR[o], 1.0f + inharm * 0.12f * (float) o) * (1.0f + inharm * 0.013f * (float) (o + 1));
            const float d = drift[o].next (rng, (3.0f + (float) o * 1.7f) * (float) n * inv, 1.0f);
            ratio[o] = r * semis (instab * d * (0.4f + 0.3f * (float) o));
            coef[o] = std::exp (-6.9078f / ((a.carrier[o] ? carDecay : modDecay) * sr));
            numCarriers += a.carrier[o] ? 1 : 0;
        }
        const float outNorm = 0.8f / std::sqrt ((float) std::max (1, numCarriers));
        static const float panL[4] = { 0.7f, 0.45f, 0.9f, 0.6f };
        static const float panR[4] = { 0.7f, 0.9f, 0.45f, 0.6f };

        float peakEnv = 0;
        for (int i = 0; i < n; ++i)
        {
            t += inv;
            const float pitchMul = pEnv > 0.01f ? semis (pEnv * std::exp (-t / pDec)) : 1.0f;
            const float fr = f0 * pitchMul;
            float out[4] = {};
            float l = 0, r = 0;
            for (int o = 3; o >= 0; --o)
            {
                if (t < 0.0006f) env[o] = std::min (1.0f, env[o] + inv / 0.0006f);
                else env[o] *= coef[o];
                float pm = 0;
                for (int s = o + 1; s < 4; ++s)
                    if (a.mod[s][o]) pm += out[s] * idx[s] * amount * 6.0f;
                if (o == 3) pm += (fb1 + fb2) * 0.5f * fbAmt * kPi;
                phase[o] += fr * ratio[o] * inv;
                phase[o] -= std::floor (phase[o]);
                out[o] = std::sin (kTwoPi * phase[o] + pm) * env[o];
                if (o == 3) { fb2 = fb1; fb1 = out[3]; }
                if (a.carrier[o])
                {
                    const float lvl = (o == 0 ? 1.0f : idx[o]) * out[o];
                    l += lvl * panL[o];
                    r += lvl * panR[o];
                    peakEnv = std::max (peakEnv, env[o]);
                }
            }
            l *= outNorm; r *= outNorm;
            if (drive > 0.001f)
            {
                const float g = 1.0f + drive * 5.0f;
                l = std::sin (fastTanh (l * g) * (1.0f + drive * 2.0f)) / (1.0f + drive);
                r = std::sin (fastTanh (r * g) * (1.0f + drive * 2.0f)) / (1.0f + drive);
            }
            L[i] += dc.process (l) * velocity;
            R[i] += dc2.process (r) * velocity;
        }
        if (t > 0.01f && peakEnv < 1.0e-4f) done = true;
    }

    bool finished() const override { return done; }

private:
    float sr = 48000, t = 0, velocity = 1, fb1 = 0, fb2 = 0;
    float phase[4] {}, env[4] {};
    SmoothedRandom drift[4];
    bool done = false;
    DcBlock dc, dc2;
    Rng rng;
};
} // namespace

std::unique_ptr<EngineVoice> makeFM() { return std::make_unique<FMLab>(); }
} // namespace ztx
