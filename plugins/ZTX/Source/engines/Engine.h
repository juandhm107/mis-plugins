#pragma once
#include "../dsp/DspUtils.h"
#include "../Params.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

namespace ztx
{
// Audio that the granular engine reads. Immutable once published.
struct SampleData
{
    juce::AudioBuffer<float> buffer;   // stereo
    double sampleRate = 48000.0;
    juce::String name;
};

struct EngineCtx
{
    float sr = 48000.0f;
    double bpm = 120.0;
    const SampleData* sample = nullptr;
};

// One engine instance inside one voice. render() ADDS into L/R.
class EngineVoice
{
public:
    virtual ~EngineVoice() = default;
    virtual void prepare (float sampleRate) = 0;
    virtual void start (float freq, float velocity, const float* p, uint32_t seed) = 0;
    virtual void render (float* L, float* R, int n, const float* p, float freq, const EngineCtx& ctx) = 0;
    virtual bool finished() const = 0;
    virtual void release() {}
};

std::unique_ptr<EngineVoice> makeRubber();
std::unique_ptr<EngineVoice> makeGlitch();
std::unique_ptr<EngineVoice> makeGranular();
std::unique_ptr<EngineVoice> makeFM();
std::unique_ptr<EngineVoice> makePerc();
} // namespace ztx
