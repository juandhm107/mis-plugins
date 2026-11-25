#include "Randomizer.h"

namespace ztx
{
namespace
{
int destIndexOf (int pid)
{
    const auto& d = modDestinations();
    for (size_t i = 1; i < d.size(); ++i) if (d[i] == pid) return (int) i;
    return 0;
}

void nudge (ZtxProcessor& p, int pid, Rng& r, float amount, float lo = 0.0f, float hi = 1.0f)
{
    const float cur = p.param (pid)->getValue();
    const float target = lo + r.uni() * (hi - lo);
    p.setParamNorm (pid, lerp (cur, target, amount));
}

void randomizeSound (ZtxProcessor& p, Rng& r, float amt)
{
    static const float ratios[] = { 0.5f, 1.0f, 1.0f, 1.41f, 2.0f, 2.76f, 3.0f, 3.51f, 4.2f, 5.4f, 7.0f, 9.3f, 11.7f };
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        const auto& inf = info (i);
        if (inf.sec != Sec::Rubber && inf.sec != Sec::Glitch && inf.sec != Sec::Gran && inf.sec != Sec::FM && inf.sec != Sec::Perc)
            continue;
        switch (i)
        {
            case rbPitch: case glPitch: case grPitch: case fmPitch: case pcPitch:
                if (r.chance (0.35f * amt)) p.setParam (i, (float) (r.range (-2, 2) * (r.chance (0.5f) ? 12 : 7)) * 0.5f);
                break;
            case fmRatio1: case fmRatio2: case fmRatio3: case fmRatio4:
                if (r.chance (0.3f + 0.6f * amt)) p.setParam (i, ratios[r.range (0, 12)] * (r.chance (0.25f) ? 1.0f + r.bi() * 0.04f : 1.0f));
                break;
            case fmAlgo: if (r.chance (amt)) p.setParam (i, (float) r.range (0, 7)); break;
            case grFreeze: p.setParam (i, r.chance (0.2f) ? 1.0f : 0.0f); break;
            case glBits: if (r.chance (0.5f)) nudge (p, i, r, amt, 0.3f, 1.0f); break;
            case rbDecay: case fmDecay: case pcDecay: case glDecay: nudge (p, i, r, amt, 0.15f, 0.75f); break;
            case grDensity: nudge (p, i, r, amt, 0.3f, 0.9f); break;
            case rbFriction: case pcRattle: case fmInstab: nudge (p, i, r, amt, 0.0f, 0.6f); break;
            default: nudge (p, i, r, amt); break;
        }
    }
}

void randomizeRhythm (ZtxProcessor& p, Rng& r, float amt)
{
    nudge (p, seqDensity, r, amt, 0.25f, 0.8f);
    nudge (p, seqComplexity, r, amt, 0.1f, 0.85f);
    nudge (p, seqChaos, r, amt, 0.0f, 0.6f);
    nudge (p, seqGroove, r, amt);
    nudge (p, seqRepeat, r, amt, 0.3f, 1.0f);
    if (r.chance (0.4f)) p.setParam (seqSwing, r.uni() * 0.3f);
    if (p.frozen) return;
    p.pushUndo();
    auto& pat = p.pattern();
    for (auto& t : pat.tracks)
        if (! t.lock && r.chance (0.3f * amt)) t.engine = r.range (0, 4);
    Rng g (r.next());
    generatePattern (pat, p.genSettings(), g);
    p.commitPattern();
}

void randomizeMod (ZtxProcessor& p, Rng& r, float amt)
{
    for (int l = 0; l < 4; ++l)
    {
        nudge (p, lfoRate (l), r, amt, 0.05f, 0.75f);
        if (r.chance (amt)) p.setParam (lfoShape (l), (float) r.range (0, 7));
        if (r.chance (amt * 0.5f)) p.setParam (lfoSync (l), r.chance (0.5f) ? 1.0f : 0.0f);
        if (r.chance (amt)) p.setParam (lfoDiv (l), (float) r.range (2, 12));
    }
    nudge (p, rndRate, r, amt, 0.1f, 0.7f);
    nudge (p, genRate, r, amt);
    nudge (p, genChaos, r, amt);
    // destinations relevant to the current engine + FX
    const int eng = (int) p.getParam (labEngine);
    std::vector<int> pool;
    const Sec es = engineSection (eng);
    for (int pid : modDestinations())
        if (pid >= 0 && (info (pid).sec == es || pid == fltCutoff || pid == fltReso || pid == foldAmt
                         || pid == cbFreq || pid == fsShift || pid == stWidth || pid == dlFb || pid == gbProb))
            pool.push_back (pid);
    for (int s = 0; s < kNumSlots; ++s)
    {
        if (r.chance (0.55f * amt + 0.1f))
        {
            p.setParam (slotSrc (s), (float) r.range (1, NUM_SRC - 2));
            p.setParam (slotDst (s), (float) destIndexOf (pool[(size_t) r.range (0, (int) pool.size() - 1)]));
            p.setParam (slotAmt (s), r.bi() * (0.12f + 0.4f * r.uni()));
        }
        else if (r.chance (amt * 0.5f)) p.setParam (slotSrc (s), 0.0f);
    }
}

void randomizeFx (ZtxProcessor& p, Rng& r, float amt)
{
    struct M { int on; float prob; std::initializer_list<int> knobs; int mix; float mixMax; };
    const M mods[] = {
        { fltOn, 0.8f, { fltCutoff, fltReso, fltDrive }, -1, 0 },
        { foldOn, 0.3f, { foldAmt, foldBias }, foldMix, 0.8f },
        { satOn, 0.45f, { satDrive }, satMix, 1.0f },
        { crOn, 0.25f, { crBits, crRate }, crMix, 0.7f },
        { fsOn, 0.2f, { fsShift, fsFb }, fsMix, 0.6f },
        { rmOn, 0.15f, { rmFreq, rmShape }, rmMix, 0.5f },
        { cbOn, 0.3f, { cbFreq, cbFb, cbDamp }, cbMix, 0.6f },
        { gbOn, 0.25f, { gbProb, gbReverse }, gbMix, 1.0f },
        { sfOn, 0.12f, { sfBlur }, sfMix, 0.6f },
        { dlOn, 0.45f, { dlFb, dlTone, dlWarble }, dlMix, 0.45f },
        { rvOn, 0.5f, { rvSize, rvDecay, rvDamp, rvMod }, rvMix, 0.5f },
    };
    for (const auto& m : mods)
    {
        if (r.chance (amt)) p.setParam (m.on, r.chance (m.prob) ? 1.0f : 0.0f);
        for (int k : m.knobs) nudge (p, k, r, amt);
        if (m.mix >= 0) nudge (p, m.mix, r, amt, 0.1f, m.mixMax);
    }
    if (r.chance (amt)) p.setParam (fltMode, (float) r.range (0, 5));
    if (p.getParam (fltMode) < 0.5f) nudge (p, fltCutoff, r, 1.0f, 0.45f, 1.0f);
    if (r.chance (amt * 0.3f)) p.setParam (fxOrder, (float) r.range (0, 3));
    if (r.chance (amt)) p.setParam (satType, (float) r.range (0, 2));
    if (r.chance (amt)) p.setParam (gbDiv, (float) r.range (1, 6));
    p.setParam (dlFb, std::min (p.getParam (dlFb), 0.85f));
    p.setParam (stWidth, 0.8f + r.uni() * 0.6f);
}
} // namespace

void randomize (ZtxProcessor& p, RandTarget t)
{
    Rng r ((uint32_t) juce::Time::getMillisecondCounterHiRes() ^ 0xA5A5u);
    const float amt = p.getParam (rndAmount);
    const bool all = t == RandTarget::All;
    if ((all || t == RandTarget::Sound) && p.getParam (lockSound) < 0.5f) randomizeSound (p, r, amt);
    if ((all || t == RandTarget::Rhythm) && p.getParam (lockRhythm) < 0.5f) randomizeRhythm (p, r, amt);
    if ((all || t == RandTarget::Mod) && p.getParam (lockMod) < 0.5f) randomizeMod (p, r, amt);
    if ((all || t == RandTarget::FX) && p.getParam (lockFx) < 0.5f) randomizeFx (p, r, amt);
}
} // namespace ztx
