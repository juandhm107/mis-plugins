#include "Effects.h"

namespace ztx
{
// =====================================================================================
//  FILTER
// =====================================================================================
void MultiFilter::process (float* L, float* R, int n, const float* p)
{
    const int mode = (int) p[fltMode];
    const float cutoff = p[fltCutoff], reso = p[fltReso], drive = p[fltDrive];
    const float driveG = 1.0f + drive * 8.0f;
    const float driveComp = 1.0f / (1.0f + drive * 2.0f);
    float* chans[2] = { L, R };

    if (mode == 5)
    {
        static const float F[5][3] = { { 300, 870, 2240 }, { 570, 840, 2410 }, { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 } };
        const float pos = clamp01 (std::log (cutoff / 20.0f) / std::log (1000.0f)) * 3.999f;
        const int vi = juce::jlimit (0, 3, (int) pos); const float vf = clamp01 (pos - (float) vi);
        const float q = 3.0f + reso * 20.0f;
        for (int c = 0; c < 2; ++c)
            for (int k = 0; k < 3; ++k)
                ch[c].form[k].set (lerp (F[vi][k], F[vi + 1][k], vf) * (c == 1 ? 1.015f : 1.0f), q, sr);
        for (int c = 0; c < 2; ++c)
        {
            float* x = chans[c];
            for (int i = 0; i < n; ++i)
            {
                float in = x[i];
                if (drive > 0) in = fastTanh (in * driveG) * driveComp;
                float s = 0;
                for (int k = 0; k < 3; ++k) { ch[c].form[k].process (in); s += ch[c].form[k].bp * (k == 0 ? 1.0f : 0.7f); }
                x[i] = s * (1.2f + reso);
            }
        }
        return;
    }

    const float q = mode == 4 ? 0.5f + reso * 7.0f : 0.5f + reso * reso * 18.0f;
    for (int c = 0; c < 2; ++c)
    {
        ch[c].a.set (cutoff, q, sr);
        ch[c].b.set (cutoff, q, sr);
        float* x = chans[c];
        for (int i = 0; i < n; ++i)
        {
            float in = x[i];
            if (drive > 0) in = fastTanh (in * driveG) * driveComp;
            Svf& a = ch[c].a;
            a.process (in);
            float y;
            switch (mode)
            {
                case 1: y = a.bp; break;
                case 2: y = a.hp; break;
                case 3: y = a.lp + a.hp; break;
                case 4:
                {
                    ch[c].b.process (fastTanh (a.lp * 1.5f));
                    y = ch[c].b.lp * (0.75f + reso * 0.25f);
                    break;
                }
                default: y = a.lp; break;
            }
            x[i] = y;
        }
    }
}

// =====================================================================================
//  HILBERT (Niemitalo allpass pair)
// =====================================================================================
static inline float ap2 (float x, float c, float* xs, float* ys)
{
    const float y = c * (x + ys[1]) - xs[1];
    xs[1] = xs[0]; xs[0] = x; ys[1] = ys[0]; ys[0] = y;
    return y;
}

void Hilbert::process (float x, float& i, float& q)
{
    static const float ca[4] = { 0.6923878f * 0.6923878f, 0.9360654322959f * 0.9360654322959f,
                                 0.9882295226860f * 0.9882295226860f, 0.9987488452737f * 0.9987488452737f };
    static const float cb[4] = { 0.4021921162426f * 0.4021921162426f, 0.8561710882420f * 0.8561710882420f,
                                 0.9722909545651f * 0.9722909545651f, 0.9952884791278f * 0.9952884791278f };
    float a = x, b = x;
    for (int k = 0; k < 4; ++k) { a = ap2 (a, ca[k], xa[k], ya[k]); b = ap2 (b, cb[k], xb[k], yb[k]); }
    i = a;
    q = qd; qd = b;
}

// =====================================================================================
//  FDN REVERB
// =====================================================================================
void FdnReverb::prepare (float sampleRate)
{
    sr = sampleRate;
    static const float ms[N] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.0f, 59.9f, 67.7f, 73.1f };
    for (int i = 0; i < N; ++i)
    {
        baseLen[i] = ms[i] * 0.001f * sr;
        lines[i].prepare ((int) (baseLen[i] * 2.2f) + 64);
        modPhase[i] = (float) i * 0.37f;
    }
    static const float dms[4] = { 3.1f, 4.7f, 7.9f, 11.3f };
    for (int i = 0; i < 4; ++i) { diffLen[i] = dms[i] * 0.001f * sr; diff[i].prepare ((int) diffLen[i] + 8); }
}

void FdnReverb::process (float* L, float* R, int n, const float* p)
{
    const float size = 0.35f + p[rvSize] * 1.65f;
    const float t60 = p[rvDecay], mix = p[rvMix], mod = p[rvMod];
    float g[N], len[N];
    for (int i = 0; i < N; ++i)
    {
        len[i] = baseLen[i] * size;
        g[i] = std::pow (10.0f, -3.0f * len[i] / (t60 * sr));
        damp[i].setFreq (900.0f + (1.0f - p[rvDamp]) * (1.0f - p[rvDamp]) * 14000.0f, sr);
    }
    const float modDepth = mod * 0.004f * sr;
    const float inv = 1.0f / sr;
    for (int s = 0; s < n; ++s)
    {
        float in = (L[s] + R[s]) * 0.5f;
        for (int d = 0; d < 4; ++d)
        {
            const float dd = diff[d].read (diffLen[d]);
            const float v = in + dd * 0.6f;
            diff[d].push (v);
            in = dd - v * 0.6f;
        }
        float o[N];
        for (int i = 0; i < N; ++i)
        {
            float dl = len[i];
            if ((i & 1) == 0) { modPhase[i] += (0.13f + (float) i * 0.041f) * inv; dl += std::sin (kTwoPi * modPhase[i]) * modDepth; }
            o[i] = damp[i].lp (lines[i].read (dl)) * g[i];
        }
        // fast Walsh-Hadamard mixing
        for (int h = 1; h < N; h <<= 1)
            for (int i = 0; i < N; i += h << 1)
                for (int j = i; j < i + h; ++j) { const float a = o[j], b = o[j + h]; o[j] = a + b; o[j + h] = a - b; }
        const float norm = 0.35355339f;
        float wl = 0, wr = 0;
        for (int i = 0; i < N; ++i)
        {
            const float v = o[i] * norm;
            lines[i].push (v + ((i & 1) ? R[s] : L[s]) * 0.3f + in * 0.25f);
            if (i & 1) wr += v; else wl += v;
        }
        L[s] = L[s] * (1.0f - mix) + wl * 0.6f * mix;
        R[s] = R[s] * (1.0f - mix) + wr * 0.6f * mix;
    }
}

// =====================================================================================
//  SPECTRAL FREEZE / BLUR
// =====================================================================================
void SpectralFreeze::prepare (float)
{
    fft = std::make_unique<juce::dsp::FFT> (order);
    window.resize (size);
    for (int i = 0; i < size; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (kTwoPi * (float) i / (float) size);
    for (auto& c : ch)
    {
        c.in.assign (size, 0); c.out.assign (size, 0); c.fft.assign (size * 2, 0);
        c.mag.assign (size / 2 + 1, 0); c.frozenMag.assign (size / 2 + 1, 0);
        c.phase.assign (size / 2 + 1, 0); c.prevMag.assign (size / 2 + 1, 0);
        c.inPos = c.outPos = c.hopCount = 0;
    }
}

void SpectralFreeze::frame (Ch& c, bool freeze, float blur)
{
    for (int i = 0; i < size; ++i) c.fft[(size_t) i] = c.in[(size_t) ((c.inPos + i) & (size - 1))] * window[(size_t) i];
    std::fill (c.fft.begin() + size, c.fft.end(), 0.0f);
    fft->performRealOnlyForwardTransform (c.fft.data(), true);
    const int bins = size / 2 + 1;
    for (int k = 0; k < bins; ++k)
    {
        const float re = c.fft[(size_t) (2 * k)], im = c.fft[(size_t) (2 * k + 1)];
        float m = std::sqrt (re * re + im * im);
        float ph = std::atan2 (im, re);
        if (freeze)
        {
            if (! wasFrozen) c.frozenMag[(size_t) k] = m;
            m = c.frozenMag[(size_t) k];
            c.phase[(size_t) k] += kTwoPi * (float) k * (float) hop / (float) size + rng.bi() * 0.5f;
            ph = c.phase[(size_t) k];
        }
        else
        {
            m = lerp (m, c.prevMag[(size_t) k], blur * 0.97f);
            c.prevMag[(size_t) k] = m;
            c.phase[(size_t) k] = ph;
            if (blur > 0.5f) ph += rng.bi() * (blur - 0.5f) * 2.0f;
        }
        c.fft[(size_t) (2 * k)] = m * std::cos (ph);
        c.fft[(size_t) (2 * k + 1)] = m * std::sin (ph);
    }
    for (int k = 1; k < size / 2; ++k)
    {
        c.fft[(size_t) (2 * (size - k))] = c.fft[(size_t) (2 * k)];
        c.fft[(size_t) (2 * (size - k) + 1)] = -c.fft[(size_t) (2 * k + 1)];
    }
    fft->performRealOnlyInverseTransform (c.fft.data());
    const float scale = 1.0f / 1.5f;
    for (int i = 0; i < size; ++i)
        c.out[(size_t) ((c.outPos + i) & (size - 1))] += c.fft[(size_t) i] * window[(size_t) i] * scale;
}

void SpectralFreeze::process (float* L, float* R, int n, const float* p)
{
    const bool freeze = p[sfFreeze] > 0.5f;
    const float blur = p[sfBlur], mix = p[sfMix];
    float* chans[2] = { L, R };
    for (int s = 0; s < n; ++s)
    {
        bool frameNow = false;
        for (int ci = 0; ci < 2; ++ci)
        {
            Ch& c = ch[ci];
            const float x = chans[ci][s];
            c.in[(size_t) c.inPos] = x;
            c.inPos = (c.inPos + 1) & (size - 1);
            const float wet = c.out[(size_t) c.outPos];
            c.out[(size_t) c.outPos] = 0;
            c.outPos = (c.outPos + 1) & (size - 1);
            chans[ci][s] = x * (1.0f - mix) + wet * mix;
            if (++c.hopCount >= hop) { c.hopCount = 0; frame (c, freeze, blur); frameNow = true; }
        }
        if (frameNow) wasFrozen = freeze;
    }
}

// =====================================================================================
//  GLITCH BUFFER (tempo-synced stutter)
// =====================================================================================
void GlitchBuffer::prepare (float sampleRate)
{
    sr = sampleRate;
    int len = 1; while (len < (int) (sr * 8)) len <<= 1;
    bufL.assign ((size_t) len, 0); bufR.assign ((size_t) len, 0);
    mask = len - 1; w = 0; stutter = false; lastSlice = -1;
}

void GlitchBuffer::process (float* L, float* R, int n, const float* p, const FxClock& clk)
{
    static const double sliceBeats[7] = { 1.0, 0.5, 0.25, 0.125, 0.0625, 1.0 / 3.0, 1.0 / 6.0 };
    const double sb = sliceBeats[juce::jlimit (0, 6, (int) p[gbDiv])];
    const long long slice = (long long) std::floor (clk.ppq / sb);
    const float mix = p[gbMix], gate = p[gbGate];
    if (slice != lastSlice)
    {
        lastSlice = slice;
        sliceLen = juce::jlimit (64, mask / 2, (int) (sb * 60.0 / std::max (20.0, clk.bpm) * sr));
        if (stutter && repeatsLeft > 0) { --repeatsLeft; readPos = 0; }
        else if (hash01 ((uint32_t) slice, 99u) < p[gbProb])
        {
            stutter = true;
            sliceStart = (w - sliceLen) & mask;
            repeatsLeft = (int) (hash01 ((uint32_t) slice, 3u) * 3.0f);
            reverse = hash01 ((uint32_t) slice, 5u) < p[gbReverse];
            speed = semis (p[gbPitch]);
            readPos = 0;
        }
        else stutter = false;
    }
    const float xf = 1.0f / 64.0f;
    for (int s = 0; s < n; ++s)
    {
        bufL[(size_t) w] = L[s]; bufR[(size_t) w] = R[s];
        w = (w + 1) & mask;
        fade = stutter ? std::min (1.0f, fade + xf) : std::max (0.0f, fade - xf);
        if (fade <= 0.0f) continue;
        const float pos = reverse ? (float) sliceLen - 1.0f - readPos : readPos;
        const int idx = (sliceStart + (int) pos) & mask;
        float env = 1.0f;
        const float x = readPos / (float) sliceLen;
        if (x > gate) env = 0.0f;
        const float edge = std::min (readPos, (float) sliceLen * gate - readPos) * xf;
        env *= clamp01 (edge);
        const float wl = bufL[(size_t) idx] * env, wr = bufR[(size_t) idx] * env;
        readPos += speed;
        if (readPos >= (float) sliceLen) readPos -= (float) sliceLen;
        const float m = fade * mix;
        L[s] = L[s] * (1.0f - m) + wl * m;
        R[s] = R[s] * (1.0f - m) + wr * m;
    }
}

// =====================================================================================
//  CHAIN
// =====================================================================================
void FxChain::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    filter.prepare (sr);
    for (int c = 0; c < 2; ++c)
    {
        comb[c].prepare ((int) (sr / 15.0f) + 8);
        delay[c].prepare ((int) (sr * 4.2f));
    }
    haas.prepare ((int) (sr * 0.04f));
    reverb.prepare (sr);
    spectral.prepare (sr);
    glitch.prepare (sr);
    reset();
}

void FxChain::reset()
{
    filter.reset();
    for (int c = 0; c < 2; ++c) { comb[c].clear(); delay[c].clear(); foldDc[c].reset(); satDc[c].reset(); hil[c].reset(); shiftFbState[c] = 0; }
    haas.clear();
}

void FxChain::process (float* L, float* R, int n, const float* p, const FxClock& clk)
{
    static const int orders[4][NUM_FX] = {
        { FxFilter, FxFold, FxSat, FxCrush, FxShift, FxRing, FxComb, FxGlitchBuf, FxSpectral, FxDelay, FxReverb, FxStereo },
        { FxShift, FxRing, FxComb, FxFilter, FxDelay, FxFold, FxSat, FxCrush, FxGlitchBuf, FxSpectral, FxReverb, FxStereo },
        { FxComb, FxReverb, FxDelay, FxSpectral, FxFilter, FxFold, FxSat, FxCrush, FxShift, FxRing, FxGlitchBuf, FxStereo },
        { FxGlitchBuf, FxCrush, FxSpectral, FxRing, FxShift, FxFold, FxSat, FxFilter, FxComb, FxDelay, FxReverb, FxStereo },
    };
    const int o = juce::jlimit (0, 3, (int) p[fxOrder]);
    for (int k = 0; k < NUM_FX; ++k) runModule (orders[o][k], L, R, n, p, clk);
}

void FxChain::runModule (int id, float* L, float* R, int n, const float* p, const FxClock& clk)
{
    float* ch[2] = { L, R };
    const float inv = 1.0f / sr;
    switch (id)
    {
        case FxFilter:
            if (p[fltOn] > 0.5f) filter.process (L, R, n, p);
            break;

        case FxFold:
            if (p[foldOn] > 0.5f)
            {
                const float g = 1.0f + p[foldAmt] * 9.0f, bias = p[foldBias] * 0.6f, mix = p[foldMix];
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < n; ++i)
                    {
                        const float x = ch[c][i];
                        const float u = 0.25f * (x * g + bias) + 0.25f;
                        const float y = 4.0f * std::abs (u - std::floor (u + 0.5f)) - 1.0f;
                        ch[c][i] = x + (foldDc[c].process (-y) * 0.8f - x) * mix;
                    }
            }
            break;

        case FxSat:
            if (p[satOn] > 0.5f)
            {
                const int type = (int) p[satType];
                const float g = 1.0f + p[satDrive] * 14.0f, mix = p[satMix];
                const float comp = 1.0f / std::sqrt (g) * 1.2f;
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < n; ++i)
                    {
                        const float x = ch[c][i];
                        float y;
                        if (type == 0) y = fastTanh (x * g);
                        else if (type == 1) y = x >= 0 ? fastTanh (x * g) : fastTanh (x * g * 0.55f) * 1.3f;
                        else y = fastTanh (std::abs (x) * g * 1.2f) * 1.1f;
                        y = satDc[c].process (y) * (type == 0 ? 0.9f : comp * 1.4f);
                        ch[c][i] = x + (y - x) * mix;
                    }
            }
            break;

        case FxCrush:
            if (p[crOn] > 0.5f)
            {
                const float levels = std::exp2 (p[crBits] - 1.0f), mix = p[crMix];
                const float step = 1.0f / p[crRate];
                for (int i = 0; i < n; ++i)
                {
                    crushPhase += step;
                    if (crushPhase >= 1.0f)
                    {
                        crushPhase -= 1.0f;
                        crushHold[0] = std::round (L[i] * levels) / levels;
                        crushHold[1] = std::round (R[i] * levels) / levels;
                    }
                    L[i] += (crushHold[0] - L[i]) * mix;
                    R[i] += (crushHold[1] - R[i]) * mix;
                }
            }
            break;

        case FxShift:
            if (p[fsOn] > 0.5f)
            {
                const float shift = p[fsShift], fb = p[fsFb], mix = p[fsMix];
                for (int i = 0; i < n; ++i)
                {
                    fsPhase += shift * inv; fsPhase -= std::floor (fsPhase);
                    const float cs = std::cos (kTwoPi * fsPhase), sn = std::sin (kTwoPi * fsPhase);
                    for (int c = 0; c < 2; ++c)
                    {
                        float re, im;
                        hil[c].process (ch[c][i] + shiftFbState[c] * fb, re, im);
                        // right channel gets the quadrature twin -> wide barber-pole
                        const float y = c == 0 ? re * cs - im * sn : re * sn + im * cs;
                        shiftFbState[c] = fastTanh (y);
                        ch[c][i] += (y - ch[c][i]) * mix;
                    }
                }
            }
            break;

        case FxRing:
            if (p[rmOn] > 0.5f)
            {
                const float f = p[rmFreq], shape = p[rmShape], mix = p[rmMix];
                for (int i = 0; i < n; ++i)
                {
                    rmPhase += f * inv; rmPhase -= std::floor (rmPhase);
                    const float s = std::sin (kTwoPi * rmPhase);
                    const float car = lerp (s, fastTanh (s * 12.0f), shape);
                    L[i] += (L[i] * car - L[i]) * mix;
                    R[i] += (R[i] * car - R[i]) * mix;
                }
            }
            break;

        case FxComb:
            if (p[cbOn] > 0.5f)
            {
                const float fb = p[cbFb], mix = p[cbMix];
                const float norm = 1.0f - std::abs (fb) * 0.8f;
                for (int c = 0; c < 2; ++c)
                {
                    const float d = sr / (p[cbFreq] * (c == 1 ? 1.007f : 1.0f));
                    combDamp[c].setFreq (500.0f + (1.0f - p[cbDamp]) * 16000.0f, sr);
                    for (int i = 0; i < n; ++i)
                    {
                        const float x = ch[c][i];
                        const float y = comb[c].read (d);
                        comb[c].push (fastTanh (x + fb * combDamp[c].lp (y)));
                        ch[c][i] = x + (y * norm * 1.4f - x) * mix;
                    }
                }
            }
            break;

        case FxGlitchBuf:
            if (p[gbOn] > 0.5f) glitch.process (L, R, n, p, clk);
            break;

        case FxSpectral:
            if (p[sfOn] > 0.5f) spectral.process (L, R, n, p);
            break;

        case FxDelay:
            if (p[dlOn] > 0.5f)
            {
                double secs = p[dlSync] > 0.5f ? divisionInBeats ((int) p[dlDiv]) * 60.0 / std::max (20.0, clk.bpm)
                                               : p[dlTime] * 0.001;
                const float target = (float) juce::jlimit (0.001, 4.0, secs) * sr;
                const float fb = p[dlFb], mix = p[dlMix], warble = p[dlWarble], tone = p[dlTone];
                for (int c = 0; c < 2; ++c)
                {
                    dlLp[c].setFreq (tone < 0.5f ? 700.0f + tone * 2.0f * 11000.0f : 16000.0f, sr);
                    dlHp[c].setFreq (tone > 0.5f ? 20.0f + (tone - 0.5f) * 2.0f * 1800.0f : 20.0f, sr);
                }
                const bool ping = p[dlPing] > 0.5f;
                for (int i = 0; i < n; ++i)
                {
                    dlWarblePhase += 0.6f * inv; dlWarblePhase -= std::floor (dlWarblePhase);
                    const float wob = (std::sin (kTwoPi * dlWarblePhase) * 0.6f + dlWander.next (rng, 1.5f * inv, 1.0f) * 0.4f)
                                      * warble * 0.006f * sr;
                    float out[2];
                    for (int c = 0; c < 2; ++c)
                    {
                        dlSmooth[c] += (target * (c == 1 && ! ping ? 1.02f : 1.0f) - dlSmooth[c]) * 0.0007f;
                        out[c] = delay[c].read (dlSmooth[c] + wob * (c == 0 ? 1.0f : -1.0f));
                    }
                    for (int c = 0; c < 2; ++c)
                    {
                        const float fbSig = ping ? out[1 - c] : out[c];
                        const float in = ping && c == 1 ? 0.0f : ch[c][i] * (ping ? 1.4f : 1.0f);
                        delay[c].push (fastTanh (dlHp[c].hp (dlLp[c].lp (fbSig * fb)) + in));
                        ch[c][i] += out[c] * mix * (1.0f - 0.5f * mix);
                        ch[c][i] *= 1.0f - mix * 0.3f;
                    }
                }
            }
            break;

        case FxReverb:
            if (p[rvOn] > 0.5f) reverb.process (L, R, n, p);
            break;

        case FxStereo:
        {
            const float width = p[stWidth];
            const float haasS = p[stHaas] * 0.001f * sr;
            for (int i = 0; i < n; ++i)
            {
                const float m = (L[i] + R[i]) * 0.5f, s = (L[i] - R[i]) * 0.5f * width;
                L[i] = m + s; R[i] = m - s;
                if (haasS > 1.0f) { haas.push (R[i]); R[i] = haas.read (haasS); }
                else haas.push (R[i]);
            }
            break;
        }
        default: break;
    }
}
} // namespace ztx
