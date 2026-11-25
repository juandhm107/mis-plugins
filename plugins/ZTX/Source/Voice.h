#pragma once
#include "engines/Engine.h"
#include "mod/ModEngine.h"

namespace ztx
{
constexpr int kCtrlBlock = 32;

struct VoiceStart
{
    int engine = 0, note = 60, track = -1, gateSamples = -1, delay = 0;
    float velocity = 1, pan = 0, level = 1, decayMod = 0, colourMod = 0;
    bool gated = false;
    uint32_t seed = 1;
};

class Voice
{
public:
    Voice()
    {
        engines[EngRubber] = makeRubber();
        engines[EngGlitch] = makeGlitch();
        engines[EngGran] = makeGranular();
        engines[EngFM] = makeFM();
        engines[EngPerc] = makePerc();
    }

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& e : engines) e->prepare (sr);
        active = false;
    }

    void start (const VoiceStart& vs, const ParamFrame& frame)
    {
        cfg = vs;
        active = true; releasing = false; killFade = 1.0f; killing = false;
        pendingDelay = vs.delay;
        gateLeft = vs.gateSamples;
        const float* a = frame.actual;
        const float velGain = 1.0f - a[velSens] * (1.0f - vs.velocity);
        gain = velGain * vs.level;
        const float pan = juce::jlimit (-1.0f, 1.0f, vs.pan);
        panL = std::cos ((pan + 1.0f) * kPi * 0.25f) * 1.41421f;
        panR = std::sin ((pan + 1.0f) * kPi * 0.25f) * 1.41421f;
        gated = vs.gated || vs.engine == EngGran;
        amp.reset();
        amp.set (a[ampA], a[ampD], gated ? a[ampS] : 1.0f, a[ampR], sr);
        amp.noteOn();
        modEnv.reset();
        modEnv.set (a[meA], a[meD], a[meS], a[meR], sr);
        modEnv.noteOn();
        buildParams (frame, 0.0f);
        engines[(size_t) cfg.engine]->start (freq, vs.velocity, p, vs.seed);
    }

    void noteOff()
    {
        if (! active) return;
        releasing = true;
        if (gated) amp.noteOff();      // one-shot: the engine rings out on its own
        modEnv.noteOff();
        engines[(size_t) cfg.engine]->release();
    }

    void kill() { killing = true; }

    // n <= kCtrlBlock. Adds into L/R.
    void render (float* L, float* R, int n, const ParamFrame& frame, const EngineCtx& ctx)
    {
        if (! active) return;
        int offset = 0;
        if (pendingDelay > 0)
        {
            if (pendingDelay >= n) { pendingDelay -= n; return; }
            offset = pendingDelay; pendingDelay = 0;
        }
        const int len = n - offset;
        if (gateLeft >= 0)
        {
            gateLeft -= len;
            if (gateLeft < 0 && ! releasing) noteOff();
        }
        amp.set (frame.actual[ampA], frame.actual[ampD], gated ? frame.actual[ampS] : 1.0f, frame.actual[ampR], sr);
        modEnv.set (frame.actual[meA], frame.actual[meD], frame.actual[meS], frame.actual[meR], sr);
        const float meVal = modEnv.advance (len);
        buildParams (frame, meVal);

        float tl[kCtrlBlock] {}, tr[kCtrlBlock] {};
        auto& eng = *engines[(size_t) cfg.engine];
        eng.render (tl, tr, len, p, freq, ctx);

        for (int i = 0; i < len; ++i)
        {
            const float e = amp.next();
            if (killing) { killFade -= 1.0f / (0.004f * sr); if (killFade <= 0) { killFade = 0; active = false; } }
            const float g = e * gain * killFade;
            L[offset + i] += tl[i] * g * panL;
            R[offset + i] += tr[i] * g * panR;
            peak = std::max (peak, std::abs (tl[i] * g));
        }
        if (gated && ! amp.active()) active = false;
        if (eng.finished()) active = false;
    }

    bool active = false, releasing = false;
    VoiceStart cfg;
    uint64_t age = 0;
    float peak = 0;

private:
    void buildParams (const ParamFrame& frame, float meVal)
    {
        std::copy (std::begin (frame.actual), std::end (frame.actual), p);
        // per-voice modulation (mod env, velocity, key) + per-track macros
        float off[kNumSlots + 2] {};
        int pids[kNumSlots + 2];
        int count = 0;
        for (int s = 0; s < frame.numV; ++s)
        {
            const auto& v = frame.vslots[s];
            const float srcV = v.src == SrcModEnv ? meVal : v.src == SrcVelocity ? cfg.velocity
                                                          : ((float) cfg.note - 60.0f) / 36.0f;
            pids[count] = v.dst; off[count] = v.amt * srcV; ++count;
        }
        if (cfg.decayMod != 0.0f) { pids[count] = engineDecayParam (cfg.engine); off[count++] = cfg.decayMod * 0.4f; }
        if (cfg.colourMod != 0.0f) { pids[count] = engineColourParam (cfg.engine); off[count++] = cfg.colourMod * 0.4f; }
        for (int k = 0; k < count; ++k)
        {
            float total = off[k];
            for (int j = 0; j < k; ++j) if (pids[j] == pids[k]) total = 0;   // already folded below
            if (total == 0.0f) continue;
            for (int j = k + 1; j < count; ++j) if (pids[j] == pids[k]) total += off[j];
            p[pids[k]] = denorm (pids[k], frame.norm[pids[k]] + total);
        }
        freq = mtof ((float) cfg.note + p[labPitch]);
    }

    std::unique_ptr<EngineVoice> engines[NUM_ENGINES];
    float p[NUM_PARAMS] {};
    float sr = 48000, freq = 261.6f, gain = 1, panL = 1, panR = 1, killFade = 1;
    int pendingDelay = 0, gateLeft = -1;
    bool gated = false, killing = false;
    Adsr amp, modEnv;
};
} // namespace ztx
