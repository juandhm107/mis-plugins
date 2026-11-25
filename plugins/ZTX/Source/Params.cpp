#include "Params.h"

namespace ztx
{
static std::vector<ParamInfo> buildTable()
{
    std::vector<ParamInfo> t;
    auto addF = [&] (const char* id, const char* name, float mn, float mx, float df, float sk, Sec s, const char* tip)
    {
        ParamInfo p { id, name, PType::Float, mn, mx, df, sk, s, tip, {}, {} };
        p.range = juce::NormalisableRange<float> (mn, mx, 0.0f, sk);
        t.push_back (p);
    };
    auto addC = [&] (const char* id, const char* name, const char* choices, int df, Sec s, const char* tip)
    {
        ParamInfo p { id, name, PType::Choice, 0, 0, (float) df, 1.0f, s, tip, {}, {} };
        juce::String c (choices);
        if (c != "@DEST")
            p.choices = juce::StringArray::fromTokens (c, "|", "");
        p.max = (float) juce::jmax (1, p.choices.size() - 1);
        t.push_back (p);
    };
    auto addB = [&] (const char* id, const char* name, bool df, Sec s, const char* tip)
    {
        ParamInfo p { id, name, PType::Bool, 0, 1, df ? 1.0f : 0.0f, 1.0f, s, tip, {}, {} };
        p.range = juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f);
        t.push_back (p);
    };
#define ZTX_T_F(id, name, mn, mx, df, sk, sec, tip) addF (#id, name, mn, mx, df, sk, sec, tip);
#define ZTX_T_C(id, name, ch, df, sec, tip)         addC (#id, name, ch, df, sec, tip);
#define ZTX_T_B(id, name, df, sec, tip)             addB (#id, name, df, sec, tip);
    ZTX_PARAMS (ZTX_T_F, ZTX_T_C, ZTX_T_B)

    // Fill the @DEST choice lists now that every parameter exists.
    juce::StringArray dests;
    dests.add ("None");
    for (int pid : modDestinations())
        if (pid >= 0)
            dests.add (juce::String (t[(size_t) pid].name));
    for (auto& p : t)
        if (p.type == PType::Choice)
        {
            if (p.choices.isEmpty()) p.choices = dests;
            p.max = (float) (p.choices.size() - 1);
            p.range = juce::NormalisableRange<float> (0.0f, p.max, 1.0f);
        }
    jassert ((int) t.size() == NUM_PARAMS);
    return t;
}

const ParamInfo& info (int index)
{
    static const std::vector<ParamInfo> table = buildTable();
    return table[(size_t) index];
}

const std::vector<int>& modDestinations()
{
    static const std::vector<int> d {
        -1,
        labPitch,
        rbElastic, rbTension, rbDamping, rbResonance, rbImpact, rbFriction, rbPitch, rbDecay, rbMaterial, rbDrive,
        glDensity, glGrain, glRandom, glRepeat, glBits, glRate, glBuffer, glProb, glTiming, glPitch, glDecay, glTone,
        grSize, grDensity, grPos, grSpray, grScan, grPitch, grPitchRnd, grReverse, grSpread, grEnv,
        fmRatio1, fmRatio2, fmRatio3, fmRatio4, fmIdx2, fmIdx3, fmIdx4, fmAmount, fmFeedback, fmDecay,
        fmPitchEnv, fmInharm, fmInstab, fmDrive, fmPitch,
        pcPitch, pcDecay, pcTransient, pcResonance, pcNoise, pcBody, pcMaterial, pcSweep, pcRattle, pcTone,
        seqProb,
        fltCutoff, fltReso, fltDrive, foldAmt, foldBias, satDrive, crBits, crRate, fsShift, fsFb, fsMix,
        rmFreq, rmMix, cbFreq, cbFb, cbMix, dlTime, dlFb, dlMix, rvSize, rvMix, stWidth, sfBlur, sfMix,
        gbProb, gbPitch, gbMix
    };
    return d;
}

juce::String modDestName (int destIndex)
{
    auto& d = modDestinations();
    if (destIndex <= 0 || destIndex >= (int) d.size()) return "None";
    return info (d[(size_t) destIndex]).name;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        const auto& p = info (i);
        juce::ParameterID pid { p.id, 1 };
        if (p.type == PType::Float)
        {
            const float span = p.max - p.min;
            auto fmt = [span] (float v, int) -> juce::String
            {
                const float a = std::abs (v);
                if (span >= 200.0f || a >= 1000.0f) return juce::String (juce::roundToInt (v));
                if (span >= 20.0f || a >= 100.0f) return juce::String (v, 1);
                return juce::String (v, 2);
            };
            layout.add (std::make_unique<juce::AudioParameterFloat> (pid, p.name, p.range, p.def,
                                                                     juce::AudioParameterFloatAttributes().withStringFromValueFunction (fmt)));
        }
        else if (p.type == PType::Choice)
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, p.name, p.choices, (int) p.def));
        else
            layout.add (std::make_unique<juce::AudioParameterBool> (pid, p.name, p.def > 0.5f));
    }
    return layout;
}

double divisionInBeats (int i)
{
    static const double v[] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0, 0.75, 0.375 };
    return v[juce::jlimit (0, 12, i)];
}

juce::StringArray engineNames()      { return juce::StringArray::fromTokens (ZTX_ENGINES, "|", ""); }
juce::StringArray engineShortNames() { return { "RUBBER", "GLITCH", "GRAIN", "FM", "PERC" }; }

Sec engineSection (int e)
{
    static const Sec s[] = { Sec::Rubber, Sec::Glitch, Sec::Gran, Sec::FM, Sec::Perc };
    return s[juce::jlimit (0, 4, e)];
}

int engineDecayParam (int e)
{
    static const int p[] = { rbDecay, glDecay, grSize, fmDecay, pcDecay };
    return p[juce::jlimit (0, 4, e)];
}
int engineColourParam (int e)
{
    static const int p[] = { rbTension, glTone, grPos, fmAmount, pcTone };
    return p[juce::jlimit (0, 4, e)];
}
} // namespace ztx
