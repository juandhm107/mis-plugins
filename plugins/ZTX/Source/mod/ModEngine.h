#pragma once
#include "../Params.h"
#include "../dsp/DspUtils.h"
#include "../seq/Pattern.h"

namespace ztx
{
// Everything a voice needs for one control block.
struct ParamFrame
{
    float norm[NUM_PARAMS] {};     // normalised value incl. global modulation (unclamped)
    float actual[NUM_PARAMS] {};   // real-world value incl. global modulation
    struct VSlot { int src = 0; int dst = -1; float amt = 0; };
    VSlot vslots[kNumSlots];
    int numV = 0;
};

inline float denorm (int pid, float n)
{
    const auto& inf = info (pid);
    n = clamp01 (n);
    if (inf.type == PType::Float) return inf.range.convertFrom0to1 (n);
    return std::round (n * inf.max);
}

inline float normOf (int pid, float actual)
{
    const auto& inf = info (pid);
    if (inf.type == PType::Float) return inf.range.convertTo0to1 (juce::jlimit (inf.min, inf.max, actual));
    return inf.max > 0 ? actual / inf.max : 0.0f;
}

class ModEngine
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        lorenz[0] = 0.1f; lorenz[1] = 0.0f; lorenz[2] = 20.0f;
    }

    struct Clock { double ppq = 0; double bpm = 120; bool synced = false; int laneStep = 0; };

    // Computes global sources and writes `frame`. raw = current parameter values (actual).
    void compute (const float* raw, int n, const Clock& clk, const Pattern& pat, float followerIn,
                  float modWheel, ParamFrame& frame, float* destDisplay)
    {
        const float dt = (float) n / sr;
        // ---- LFOs
        for (int l = 0; l < 4; ++l)
        {
            const bool sync = raw[lfoSync (l)] > 0.5f;
            const int shape = (int) raw[lfoShape (l)];
            float ph;
            if (sync)
            {
                const double cyc = divisionInBeats ((int) raw[lfoDiv (l)]);
                if (clk.synced) ph = (float) (clk.ppq / cyc - std::floor (clk.ppq / cyc));
                else { ph = lfoPhase[l] + (float) (clk.bpm / 60.0 / cyc) * dt; ph -= std::floor (ph); }
            }
            else { ph = lfoPhase[l] + raw[lfoRate (l)] * dt; ph -= std::floor (ph); }
            const bool wrapped = ph < lfoPhase[l];
            lfoPhase[l] = ph;
            if (wrapped) { lfoHeldPrev[l] = lfoHeld[l]; lfoHeld[l] = rng.bi(); }
            float v = 0;
            switch (shape)
            {
                case 0: v = std::sin (kTwoPi * ph); break;
                case 1: v = 1.0f - 4.0f * std::abs (ph - 0.5f); break;
                case 2: v = 2.0f * ph - 1.0f; break;
                case 3: v = ph < 0.5f ? 1.0f : -1.0f; break;
                case 4: { const float s = ph * ph * (3.0f - 2.0f * ph); v = lerp (lfoHeldPrev[l], lfoHeld[l], s); break; }
                case 5: v = lfoHeld[l]; break;
                case 6:
                    drunk[l] = juce::jlimit (-1.0f, 1.0f, drunk[l] + rng.bi() * std::sqrt (dt) * (0.6f + raw[lfoRate (l)] * 0.4f));
                    v = drunk[l]; break;
                case 7: default: v = 2.0f * std::exp (-ph * 9.0f) - 1.0f; break;
            }
            src[SrcLfo1 + l] = v;
        }
        // ---- random
        src[SrcRandom] = smoothRnd.next (rng, raw[rndRate] * dt, raw[rndSmooth]);
        // ---- envelope follower
        {
            const float tc = followerIn > follower ? raw[efAtt] : raw[efRel];
            const float c = std::exp (-dt / std::max (0.0005f, tc));
            follower = followerIn + (follower - followerIn) * c;
            src[SrcFollow] = clamp01 (follower * 3.0f);
        }
        // ---- generative (Lorenz attractor blended with a quasi-periodic orbit)
        {
            const float rho = 18.0f + raw[genChaos] * 22.0f;
            const float h = (0.0015f + raw[genRate] * raw[genRate] * 0.03f) * (float) n / 32.0f / 4.0f;
            for (int k = 0; k < 8; ++k)   // small Euler steps keep the attractor stable
            {
                const float x = lorenz[0], y = lorenz[1], z = lorenz[2];
                lorenz[0] += h * 10.0f * (y - x);
                lorenz[1] += h * (x * (rho - z) - y);
                lorenz[2] += h * (x * y - 2.6667f * z);
            }
            if (! std::isfinite (lorenz[0] + lorenz[1] + lorenz[2]) || std::abs (lorenz[0]) > 200.0f)
            { lorenz[0] = 0.1f; lorenz[1] = 0.0f; lorenz[2] = 20.0f; }
            genPhase += (0.05f + raw[genRate] * 2.0f) * dt;
            const float orbit = 0.6f * std::sin (kTwoPi * genPhase) + 0.4f * std::sin (kTwoPi * genPhase * 2.618f);
            src[SrcGen] = juce::jlimit (-1.0f, 1.0f, lerp (orbit, lorenz[0] / 18.0f, raw[genChaos]));
        }
        // ---- step lane
        {
            const float target = pat.lane[(size_t) (clk.laneStep % kLaneSteps)] * 2.0f - 1.0f;
            lane += (target - lane) * std::min (1.0f, dt * 60.0f);
            src[SrcStepLane] = lane;
        }
        src[SrcModWheel] = modWheel;

        // ---- base normalised values
        for (int i = 0; i < NUM_PARAMS; ++i) frame.norm[i] = normOf (i, raw[i]);

        // ---- evolve (exploration mode): bounded random walk per destination
        const auto& dests = modDestinations();
        const bool evolve = raw[evolveOn] > 0.5f;
        const float evAmt = raw[evolveAmt] * 0.35f;
        const float evStep = (0.02f + raw[evolveRate] * raw[evolveRate] * 1.5f) * std::sqrt (dt);
        float offset[NUM_PARAMS] {};
        for (size_t d = 1; d < dests.size(); ++d)
        {
            const int pid = dests[d];
            if (evolve && evolvable (pid))
            {
                auto& w = evolveW[d];
                w = juce::jlimit (-1.0f, 1.0f, w * (1.0f - dt * 0.05f) + rng.bi() * evStep);
                offset[pid] += w * evAmt;
            }
        }

        // ---- matrix (global sources); per-voice sources are forwarded
        frame.numV = 0;
        for (int s = 0; s < kNumSlots; ++s)
        {
            const int sIdx = (int) raw[slotSrc (s)];
            const int dIdx = (int) raw[slotDst (s)];
            const float amt = raw[slotAmt (s)];
            if (sIdx <= 0 || dIdx <= 0 || dIdx >= (int) dests.size() || std::abs (amt) < 1.0e-4f) continue;
            const int pid = dests[(size_t) dIdx];
            if (sIdx == SrcModEnv || sIdx == SrcVelocity || sIdx == SrcKey)
            {
                frame.vslots[frame.numV++] = { sIdx, pid, amt };
                continue;
            }
            offset[pid] += amt * src[sIdx];
        }
        for (int s = 0; s < NUM_SRC; ++s) if (! std::isfinite (src[s])) src[s] = 0.0f;
        for (int i = 0; i < NUM_PARAMS; ++i)
        {
            if (! std::isfinite (offset[i])) offset[i] = 0.0f;
            if (std::abs (offset[i]) > 0.0f)
            {
                frame.norm[i] += offset[i];
                frame.actual[i] = denorm (i, frame.norm[i]);
            }
            else frame.actual[i] = raw[i];
            if (destDisplay) destDisplay[i] = offset[i];
        }
    }

    static bool evolvable (int pid)
    {
        switch (pid)
        {
            case labPitch: case rbPitch: case glPitch: case grPitch: case fmPitch: case pcPitch:
            case fmRatio1: case fmRatio2: case fmRatio3: case fmRatio4: case seqProb:
            case dlTime: case dlMix: case rvMix: case stWidth: case sfMix: case gbMix: case gbProb: case gbPitch:
                return false;
            default: return true;
        }
    }

    float src[NUM_SRC] {};

private:
    float sr = 48000;
    float lfoPhase[4] {}, lfoHeld[4] {}, lfoHeldPrev[4] {}, drunk[4] {};
    float follower = 0, lane = 0, genPhase = 0;
    float lorenz[3] {};
    float evolveW[128] {};
    SmoothedRandom smoothRnd;
    Rng rng { 0x51ED270Bu };
};
} // namespace ztx
