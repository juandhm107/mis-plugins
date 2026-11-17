#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <array>

// Motor de sintesis de Dembow: osciladores, filtro, envolventes y voz.
namespace dembow
{
    constexpr float kTwoPi = 6.283185307179586f;

    enum Wave { Seno = 0, Triangulo, Sierra, Cuadrada, Pulso, SuperSierra, Organo, NumWaves };
    enum FilterType { LP24 = 0, LP12, BP, HP };
    enum LfoWave { LfoSeno = 0, LfoTriangulo, LfoCuadrada, LfoAleatorio };
    enum LfoDest { DestTono = 0, DestFiltro, DestVolumen, DestPulso };
    enum VoiceMode { Poli = 0, Mono, Legato };

    inline float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)        { t /= dt;          return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    inline float wrap01 (float x) noexcept { return x - std::floor (x); }

    inline float softClip (float x) noexcept
    {
        const float a = std::abs (x);
        if (a < 0.8f) return x;
        const float y = 0.8f + 0.2f * std::tanh ((a - 0.8f) * 5.0f);
        return x < 0.0f ? -y : y;
    }

    // Generador de ruido rapido
    struct Rng
    {
        uint32_t s = 0x12345678u;
        float next() noexcept
        {
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            return (float) s * (2.0f / 4294967296.0f) - 1.0f;
        }
    };

    //==========================================================================
    // Envolvente ADSR con curvas exponenciales (estilo analogico)
    class Adsr
    {
    public:
        enum State { Idle, Attack, Decay, Sustain, Release };

        void setParams (float a, float d, float s, float r, float sr) noexcept
        {
            sustain = s;
            attackCoef  = coef (a * sr, 0.3f);
            attackBase  = (1.0f + 0.3f) * (1.0f - attackCoef);
            decayCoef   = coef (d * sr, 0.05f);
            decayBase   = (sustain - 0.05f) * (1.0f - decayCoef);
            releaseCoef = coef (r * sr, 0.05f);
            releaseBase = -0.05f * (1.0f - releaseCoef);
        }

        void noteOn() noexcept  { state = Attack; }
        void noteOff() noexcept { if (state != Idle) state = Release; }
        void reset() noexcept   { state = Idle; value = 0.0f; }

        float process() noexcept
        {
            switch (state)
            {
                case Attack:
                    value = attackBase + value * attackCoef;
                    if (value >= 1.0f) { value = 1.0f; state = Decay; }
                    break;
                case Decay:
                    value = decayBase + value * decayCoef;
                    if (value <= sustain) { value = sustain; state = Sustain; }
                    break;
                case Sustain:
                    value = sustain;
                    break;
                case Release:
                    value = releaseBase + value * releaseCoef;
                    if (value <= 0.0f) { value = 0.0f; state = Idle; }
                    break;
                case Idle:
                default:
                    value = 0.0f;
                    break;
            }
            return value;
        }

        bool isActive() const noexcept
        {
            if (state == Idle) return false;
            if (state == Sustain && sustain <= 0.0f) return false;
            return true;
        }

        State state = Idle;
        float value = 0.0f;

    private:
        static float coef (float rate, float ratio) noexcept
        {
            return rate <= 1.0f ? 0.0f : std::exp (-std::log ((1.0f + ratio) / ratio) / rate);
        }

        float sustain = 1.0f;
        float attackCoef = 0, attackBase = 0, decayCoef = 0, decayBase = 0, releaseCoef = 0, releaseBase = 0;
    };

    //==========================================================================
    // Filtro de estado variable (TPT / Cytomic)
    struct Svf
    {
        float ic1 = 0, ic2 = 0;
        void reset() noexcept { ic1 = ic2 = 0; }

        // devuelve lp, bp, hp
        inline void process (float v0, float g, float k, float& lp, float& bp, float& hp) noexcept
        {
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;
            const float v3 = v0 - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            lp = v2; bp = v1; hp = v0 - k * v1 - v2;
        }
    };

    //==========================================================================
    // Valores de los parametros (se copian una vez por bloque)
    struct OscSettings
    {
        int wave = Sierra;
        float semis = 0.0f;   // octava*12 + semitono + fino
        float level = 0.0f;
    };

    struct Settings
    {
        OscSettings osc[2];
        float pw = 0.5f, detune = 0.3f, fm = 0.0f, fmEnv = 0.0f, sub = 0.0f, noise = 0.0f;

        int filterType = LP24;
        float cutoff = 8000.0f, reso = 0.1f, filterEnv = 0.0f, keyTrack = 0.5f, drive = 0.0f;

        float aA = 0.005f, aD = 0.3f, aS = 0.8f, aR = 0.25f;
        float fA = 0.005f, fD = 0.4f, fS = 0.5f, fR = 0.3f;

        int lfoWave = LfoSeno, lfoDest = DestTono;
        float lfoRate = 5.0f, lfoDepth = 0.0f, lfoFade = 0.0f;

        float velSens = 0.5f;
        float glide = 0.0f;
    };

    struct Globals
    {
        float bendSemis = 0.0f;  // rueda de tono
        float modWheel = 0.0f;   // rueda de modulacion (vibrato extra)
        float sampleRate = 44100.0f;
    };

    //==========================================================================
    class Voice
    {
    public:
        static constexpr int kSaws = 7;
        static constexpr int kChunk = 16;

        void prepare (float sr) noexcept { sampleRate = sr; reset(); }

        void reset() noexcept
        {
            ampEnv.reset(); fltEnv.reset();
            for (auto& f : svf) f.reset();
            active = false;
        }

        // Empieza una nota. retrigger=false -> legato (no reinicia envolventes)
        void start (int midiNote, float vel, float fromPitch, bool doGlide, bool retrigger, uint32_t seed) noexcept
        {
            const bool wasSilent = ! active;
            note = midiNote;
            velocity = vel;
            targetPitch = (float) midiNote;
            currentPitch = doGlide ? fromPitch : targetPitch;
            gate = true;
            sustained = false;

            if (retrigger || wasSilent)
            {
                if (wasSilent)
                {
                    // Fase fija al empezar: ataques consistentes (bajos, plucks)
                    for (int o = 0; o < 2; ++o)
                    {
                        phase[o] = 0.0f;
                        for (int i = 0; i < kSaws; ++i)
                            saw[o][i] = wrap01 ((float) i * 0.1428f + 0.05f * (float) o);
                    }
                    subPhase = 0.0f;
                    for (auto& f : svf) f.reset();
                    lastFc = -1.0f;
                    ampEnv.value = 0.0f; fltEnv.value = 0.0f;
                }
                ampEnv.noteOn();
                fltEnv.noteOn();
                lfoPhase = 0.0f;
                lfoTime = 0.0f;
                rng.s = seed | 1u;
                lfoHold = rng.next();
            }
            active = true;
            age = 0;
        }

        void stop() noexcept
        {
            gate = false;
            ampEnv.noteOff();
            fltEnv.noteOff();
        }

        void glideTo (int midiNote) noexcept { note = midiNote; targetPitch = (float) midiNote; }

        // Suma la voz en L/R
        void render (float* outL, float* outR, int numSamples, const Settings& s, const Globals& g) noexcept
        {
            if (! active) return;

            ampEnv.setParams (s.aA, s.aD, s.aS, s.aR, sampleRate);
            fltEnv.setParams (s.fA, s.fD, s.fS, s.fR, sampleRate);

            const float velScale = 1.0f - s.velSens * (1.0f - velocity);
            const float glideCoef = s.glide <= 0.0005f ? 0.0f
                                  : std::exp (-1.0f / (s.glide * 0.35f * sampleRate / (float) kChunk));
            const float lfoInc = s.lfoRate / sampleRate;
            const float nyq = 0.45f * sampleRate;
            const float k = 2.0f - 1.96f * std::clamp (s.reso, 0.0f, 1.0f);
            const float driveGain = 1.0f + s.drive * 7.0f;
            const float driveNorm = 1.0f / std::tanh (std::min (driveGain, 3.0f));

            int pos = 0;
            while (pos < numSamples)
            {
                const int n = std::min (kChunk, numSamples - pos);

                // ---- control (cada 16 muestras) ----
                currentPitch = targetPitch + (currentPitch - targetPitch) * glideCoef;

                // LFO
                float lfo = 0.0f;
                switch (s.lfoWave)
                {
                    case LfoSeno:      lfo = std::sin (kTwoPi * lfoPhase); break;
                    case LfoTriangulo: lfo = 1.0f - 4.0f * std::abs (lfoPhase - 0.5f); break;
                    case LfoCuadrada:  lfo = lfoPhase < 0.5f ? 1.0f : -1.0f; break;
                    default:           lfo = lfoHold; break;
                }
                lfoPhase += lfoInc * (float) n;
                if (lfoPhase >= 1.0f) { lfoPhase -= std::floor (lfoPhase); lfoHold = rng.next(); }

                const float fade = s.lfoFade <= 0.001f ? 1.0f : std::min (1.0f, lfoTime / s.lfoFade);
                lfoTime += (float) n / sampleRate;
                const float depth = s.lfoDepth * fade;

                float pitchMod = 0.0f, cutMod = 0.0f, ampMod = 1.0f, pwMod = 0.0f;
                switch (s.lfoDest)
                {
                    case DestTono:    pitchMod = lfo * depth * depth * 12.0f; break;
                    case DestFiltro:  cutMod = lfo * depth * 4.0f; break;
                    case DestVolumen: ampMod = 1.0f - depth * (0.5f + 0.5f * lfo); break;
                    case DestPulso:   pwMod = lfo * depth * 0.4f; break;
                    default: break;
                }
                pitchMod += lfo * g.modWheel * 0.6f;

                const float basePitch = currentPitch + g.bendSemis + pitchMod;
                float inc[2], sawInc[2][kSaws];
                for (int o = 0; o < 2; ++o)
                {
                    const float p = basePitch + s.osc[o].semis;
                    const float f = 440.0f * std::exp2 ((p - 69.0f) / 12.0f);
                    inc[o] = std::min (f / sampleRate, 0.49f);
                    if (s.osc[o].wave == SuperSierra)
                        for (int i = 0; i < kSaws; ++i)
                        {
                            const float cents = sawSpread[i] * s.detune * 70.0f;
                            sawInc[o][i] = std::min (inc[o] * std::exp2 (cents / 1200.0f), 0.49f);
                        }
                }
                const float subInc = inc[0] * 0.5f;
                const float pw = std::clamp (s.pw + pwMod, 0.05f, 0.95f);

                const float baseCut = s.cutoff * std::exp2 (s.keyTrack * ((float) note - 60.0f) / 12.0f + cutMod);

                // ---- audio ----
                for (int i = 0; i < n; ++i)
                {
                    const float envA = ampEnv.process();
                    const float envF = fltEnv.process();

                    // Oscilador 2 primero (modula al 1 en FM)
                    float l2 = 0.0f, r2 = 0.0f;
                    renderOsc (1, s.osc[1].wave, inc[1], sawInc[1], pw, 0.0f, l2, r2);

                    const float fmIdx = std::max (0.0f, s.fm + s.fmEnv * envF);
                    const float pm = fmIdx * 0.5f * (l2 + r2);

                    float l1 = 0.0f, r1 = 0.0f;
                    renderOsc (0, s.osc[0].wave, inc[0], sawInc[0], pw, pm, l1, r1);

                    const float subV = std::sin (kTwoPi * subPhase);
                    subPhase += subInc; if (subPhase >= 1.0f) subPhase -= 1.0f;
                    const float nz = rng.next();

                    const float centre = subV * s.sub + nz * s.noise * 0.5f;
                    float inL = l1 * s.osc[0].level + l2 * s.osc[1].level + centre;
                    float inR = r1 * s.osc[0].level + r2 * s.osc[1].level + centre;

                    // Filtro
                    const float envAmt = s.filterEnv * 6.0f * (s.filterEnv > 0.0f ? velScale : 1.0f);
                    float fc = baseCut * std::exp2 (envAmt * envF);
                    fc = std::clamp (fc, 20.0f, nyq);
                    // tan solo cuando cambia de verdad (ahorra CPU)
                    if (std::abs (fc - lastFc) > lastFc * 0.002f)
                    {
                        lastFc = fc;
                        gCoef = std::tan (3.14159265f * fc / sampleRate);
                    }

                    float outs[2] = { inL, inR };
                    for (int c = 0; c < 2; ++c)
                    {
                        float lp, bp, hp;
                        float x = outs[c];
                        switch (s.filterType)
                        {
                            case LP24:
                                svf[c * 2].process (x, gCoef, 1.414f, lp, bp, hp);
                                svf[c * 2 + 1].process (lp, gCoef, k, lp, bp, hp);
                                x = lp; break;
                            case LP12:
                                svf[c * 2].process (x, gCoef, k, lp, bp, hp); x = lp; break;
                            case BP:
                                svf[c * 2].process (x, gCoef, k, lp, bp, hp); x = bp * k; break;
                            case HP:
                            default:
                                svf[c * 2].process (x, gCoef, k, lp, bp, hp); x = hp; break;
                        }
                        if (s.drive > 0.001f)
                            x = std::tanh (x * driveGain) * driveNorm * (1.0f / (1.0f + s.drive));
                        outs[c] = x;
                    }

                    const float gain = envA * velScale * ampMod * 0.35f;
                    outL[pos + i] += outs[0] * gain;
                    outR[pos + i] += outs[1] * gain;
                }

                pos += n;
            }

            age += numSamples;
            if (! ampEnv.isActive())
            {
                active = false;
                ampEnv.reset(); fltEnv.reset();
            }
        }

        bool active = false;
        bool gate = false;
        bool sustained = false;   // nota soltada pero pedal pisado
        int note = -1;
        int64_t age = 0;
        float velocity = 1.0f;
        float currentPitch = 60.0f, targetPitch = 60.0f;
        Adsr ampEnv, fltEnv;

    private:
        inline void renderOsc (int o, int wave, float dt, const float* sInc, float pw, float pm,
                               float& l, float& r) noexcept
        {
            float& ph = phase[o];
            const float t = pm == 0.0f ? ph : wrap01 (ph + pm);
            switch (wave)
            {
                case Seno:
                    l = r = std::sin (kTwoPi * t); break;
                case Triangulo:
                    l = r = 1.0f - 4.0f * std::abs (t - 0.5f); break;
                case Sierra:
                    l = r = 2.0f * t - 1.0f - polyBlep (t, dt); break;
                case Cuadrada:
                case Pulso:
                {
                    const float w = wave == Cuadrada ? 0.5f : pw;
                    float v = t < w ? 1.0f : -1.0f;
                    v += polyBlep (t, dt);
                    v -= polyBlep (wrap01 (t + 1.0f - w), dt);
                    l = r = v * 0.8f;
                    break;
                }
                case SuperSierra:
                {
                    float sl = 0.0f, sr = 0.0f;
                    for (int i = 0; i < kSaws; ++i)
                    {
                        float& sp = saw[o][i];
                        const float tt = pm == 0.0f ? sp : wrap01 (sp + pm);
                        const float v = (2.0f * tt - 1.0f - polyBlep (tt, sInc[i])) * sawGain[i];
                        sl += v * sawPanL[i];
                        sr += v * sawPanR[i];
                        sp += sInc[i]; if (sp >= 1.0f) sp -= 1.0f;
                    }
                    l = sl * 0.45f; r = sr * 0.45f;
                    break;
                }
                case Organo:
                default:
                {
                    const float a = kTwoPi * t;
                    const float v = std::sin (a) + 0.8f * std::sin (2.0f * a) + 0.6f * std::sin (3.0f * a)
                                  + 0.45f * std::sin (4.0f * a) + 0.3f * std::sin (6.0f * a) + 0.22f * std::sin (8.0f * a);
                    l = r = v * 0.36f;
                    break;
                }
            }
            ph += dt; if (ph >= 1.0f) ph -= 1.0f;
        }

        static constexpr float sawSpread[kSaws] = { 0.0f, -0.33f, 0.33f, -0.66f, 0.66f, -1.0f, 1.0f };
        static constexpr float sawGain[kSaws]   = { 1.0f, 0.75f, 0.75f, 0.7f, 0.7f, 0.65f, 0.65f };
        static constexpr float sawPanL[kSaws]   = { 0.7f, 1.0f, 0.4f, 0.9f, 0.5f, 1.0f, 0.3f };
        static constexpr float sawPanR[kSaws]   = { 0.7f, 0.4f, 1.0f, 0.5f, 0.9f, 0.3f, 1.0f };

        float sampleRate = 44100.0f;
        float phase[2] = { 0, 0 };
        float saw[2][kSaws] = {};
        float subPhase = 0.0f;
        float lfoPhase = 0.0f, lfoTime = 0.0f, lfoHold = 0.0f;
        float lastFc = -1.0f, gCoef = 0.1f;
        Svf svf[4];
        Rng rng;
    };
}
