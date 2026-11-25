#pragma once
#include "../dsp/DspUtils.h"
#include "../Params.h"
#include <juce_dsp/juce_dsp.h>

namespace ztx
{
enum FxId { FxFilter, FxFold, FxSat, FxCrush, FxShift, FxRing, FxComb, FxGlitchBuf, FxSpectral, FxDelay, FxReverb, FxStereo, NUM_FX };

struct FxClock { double ppq = 0; double bpm = 120; };

class MultiFilter
{
public:
    void prepare (float sampleRate) { sr = sampleRate; reset(); }
    void reset() { for (auto& c : ch) { c.a.reset(); c.b.reset(); for (auto& f : c.form) f.reset(); } }
    void process (float* L, float* R, int n, const float* p);
private:
    struct Ch { Svf a, b, form[3]; };
    Ch ch[2];
    float sr = 48000;
};

class Hilbert
{
public:
    void process (float x, float& i, float& q);
    void reset() { for (int k = 0; k < 4; ++k) for (int j = 0; j < 2; ++j) xa[k][j] = ya[k][j] = xb[k][j] = yb[k][j] = 0.0f; qd = 0; }
private:
    float xa[4][2] {}, ya[4][2] {}, xb[4][2] {}, yb[4][2] {};
    float qd = 0;
};

class FdnReverb
{
public:
    void prepare (float sampleRate);
    void process (float* L, float* R, int n, const float* p);
private:
    static constexpr int N = 8;
    DelayLine lines[N];
    DelayLine diff[4];
    float diffLen[4] {};
    OnePole damp[N];
    float baseLen[N] {};
    float modPhase[N] {};
    float sr = 48000;
};

class SpectralFreeze
{
public:
    void prepare (float sampleRate);
    void process (float* L, float* R, int n, const float* p);
private:
    static constexpr int order = 11, size = 1 << order, hop = size / 4;
    struct Ch
    {
        std::vector<float> in, out, fft, mag, frozenMag, phase, prevMag;
        int inPos = 0, outPos = 0, hopCount = 0;
    };
    void frame (Ch& c, bool freeze, float blur);
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    Ch ch[2];
    bool wasFrozen = false;
    Rng rng { 0xBEEFu };
};

class GlitchBuffer
{
public:
    void prepare (float sampleRate);
    void process (float* L, float* R, int n, const float* p, const FxClock& clk);
private:
    std::vector<float> bufL, bufR;
    int mask = 0, w = 0;
    bool stutter = false;
    int sliceLen = 1, sliceStart = 0, repeatsLeft = 0;
    long long lastSlice = -1;
    float readPos = 0, speed = 1, fade = 0;
    bool reverse = false;
    float sr = 48000;
};

class FxChain
{
public:
    void prepare (double sampleRate);
    void reset();
    void process (float* L, float* R, int n, const float* p, const FxClock& clk);
private:
    void runModule (int id, float* L, float* R, int n, const float* p, const FxClock& clk);
    float sr = 48000;
    MultiFilter filter;
    DcBlock foldDc[2], satDc[2];
    OnePole satTone[2];
    float crushPhase = 0, crushHold[2] {};
    Hilbert hil[2];
    float fsPhase = 0, shiftFbState[2] {};
    float rmPhase = 0;
    DelayLine comb[2];
    OnePole combDamp[2];
    DelayLine delay[2];
    OnePole dlLp[2], dlHp[2];
    float dlSmooth[2] { 10000, 10000 };
    float dlWarblePhase = 0;
    SmoothedRandom dlWander;
    Rng rng { 0xF00Du };
    FdnReverb reverb;
    SpectralFreeze spectral;
    GlitchBuffer glitch;
    DelayLine haas;
};
} // namespace ztx
