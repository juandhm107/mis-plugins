#pragma once

#include <cmath>
#include <algorithm>

// Bloques de procesamiento sencillos y sin asignaciones de memoria.
namespace calor
{
    constexpr float kPi = 3.14159265358979f;

    enum ShapeType
    {
        Tubo = 0,
        Cinta,
        Diodo,
        Fuzz,
        Pliegue,
        Rectificador,
        Bits,
        Recorte,
        NumShapes
    };

    inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

    // Curvas de saturacion. La entrada ya viene amplificada por el drive.
    inline float shape (int type, float x) noexcept
    {
        switch (type)
        {
            case Tubo:
            {
                // Asimetrica: anade armonicos pares, como una valvula.
                constexpr float bias = 0.25f;
                return std::tanh (x + bias) - std::tanh (bias);
            }
            case Cinta:
            {
                // Saturacion muy redonda y gradual.
                return std::copysign (1.0f - std::exp (-std::abs (x) * 1.5f), x);
            }
            case Diodo:
            {
                // Recorte asimetrico: rodilla dura en positivo, blanda en negativo.
                return x > 0.0f ? std::tanh (1.6f * x) * 0.85f
                                : std::tanh (0.8f * x);
            }
            case Fuzz:
            {
                // Mucha ganancia, casi una onda cuadrada.
                return std::atan (x * 4.0f) * (2.0f / kPi);
            }
            case Pliegue:
            {
                // Plegado de onda: la senal "se dobla" sobre si misma.
                return std::sin (x * 1.3f) * 0.9f;
            }
            case Rectificador:
            {
                // Mezcla de onda completa: da un caracter de octava hacia arriba.
                const float t = std::tanh (x);
                return 0.35f * t + 0.9f * std::abs (t);
            }
            case Recorte:
            {
                return std::clamp (x, -1.0f, 1.0f);
            }
            default:
                return x;
        }
    }

    // Filtro de inclinacion (tilt): realza graves o agudos alrededor de ~800 Hz.
    struct Tilt
    {
        float coeff = 0.0f;
        float lp = 0.0f;

        void setup (double sampleRate, float freq = 800.0f) noexcept
        {
            coeff = 1.0f - std::exp (-2.0f * kPi * freq / (float) sampleRate);
        }

        void reset() noexcept { lp = 0.0f; }

        float process (float x, float gLow, float gHigh) noexcept
        {
            lp += coeff * (x - lp);
            return lp * gLow + (x - lp) * gHigh;
        }
    };

    // Bloqueador de corriente continua (quita el desplazamiento que crean las curvas asimetricas).
    struct DcBlocker
    {
        float r = 0.999f;
        float x1 = 0.0f, y1 = 0.0f;

        void setup (double sampleRate, float freq = 10.0f) noexcept
        {
            r = std::exp (-2.0f * kPi * freq / (float) sampleRate);
        }

        void reset() noexcept { x1 = y1 = 0.0f; }

        float process (float x) noexcept
        {
            const float y = x - x1 + r * y1;
            x1 = x;
            y1 = y;
            return y;
        }
    };

    // Estado de una etapa para un canal.
    struct StageChannel
    {
        Tilt tilt;
        DcBlocker dc;
        float held = 0.0f;
        int holdCounter = 0;

        void reset() noexcept
        {
            tilt.reset();
            dc.reset();
            held = 0.0f;
            holdCounter = 0;
        }
    };

    // Valores de una etapa que se interpolan dentro de cada trozo de audio.
    struct StageValues
    {
        float gain = 1.0f;   // ganancia de entrada (drive)
        float comp = 1.0f;   // compensacion de nivel
        float mix = 1.0f;    // mezcla seco/procesado de la etapa
        float gLow = 1.0f;   // tilt: ganancia de graves
        float gHigh = 1.0f;  // tilt: ganancia de agudos
        float crush = 0.0f;  // cantidad para el modo Bits (0..1)
    };

    inline StageValues lerp (const StageValues& a, const StageValues& b, float t) noexcept
    {
        auto l = [t] (float x, float y) { return x + (y - x) * t; };
        return { l (a.gain, b.gain), l (a.comp, b.comp), l (a.mix, b.mix),
                 l (a.gLow, b.gLow), l (a.gHigh, b.gHigh), l (a.crush, b.crush) };
    }

    // Procesa una muestra a traves de una etapa.
    inline float processStage (float in, int type, const StageValues& v,
                               StageChannel& st, int osFactor) noexcept
    {
        float wet;

        if (type == Bits)
        {
            // Reduccion de bits y de frecuencia de muestreo.
            const float bits = 16.0f - v.crush * 13.0f;
            const float steps = std::pow (2.0f, bits) * 0.5f;
            const int hold = std::max (1, (int) (1.0f + v.crush * v.crush * 40.0f) * osFactor);

            if (--st.holdCounter <= 0)
            {
                st.held = std::round (in * steps) / steps;
                st.holdCounter = hold;
            }
            wet = std::tanh (st.held * 1.2f) / std::tanh (1.2f);
        }
        else
        {
            wet = shape (type, in * v.gain) * v.comp;
        }

        wet = st.dc.process (wet);
        wet = st.tilt.process (wet, v.gLow, v.gHigh);

        return in + (wet - in) * v.mix;
    }
}
