#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "DSP.h"

inline juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

class CalorProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kNumStages = 3;
    static constexpr int kMaxChannels = 2;
    static constexpr int kChunk = 32;   // muestras por trozo de control
    static constexpr int kOsOrder = 2;  // 2^2 = sobremuestreo 4x
    static constexpr int kOsFactor = 1 << kOsOrder;

    CalorProcessor();
    ~CalorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Para la interfaz grafica
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> modulationValue { 0.0f };

private:
    void processSlice (juce::dsp::AudioBlock<float> block);
    calor::StageValues computeStageTarget (int stage, float modDb) const;

    struct StageParams
    {
        std::atomic<float>* on = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* drive = nullptr;
        std::atomic<float>* mix = nullptr;
        std::atomic<float>* tone = nullptr;
    };

    std::array<StageParams, kNumStages> stageParams;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pXoverLo = nullptr;
    std::atomic<float>* pXoverHi = nullptr;
    std::atomic<float>* pHeat = nullptr;
    std::atomic<float>* pCharacter = nullptr;
    std::atomic<float>* pLfoRate = nullptr;
    std::atomic<float>* pLfoDepth = nullptr;
    std::atomic<float>* pEnvAmount = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pOutTone = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pAutoGain = nullptr;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::dsp::DryWetMixer<float> dryWet { 512 };
    juce::dsp::Gain<float> outputGain;

    juce::dsp::LinkwitzRileyFilter<float> lp1, hp1, lp2, hp2, ap2;

    std::array<std::array<calor::StageChannel, kMaxChannels>, kNumStages> stageState;
    std::array<calor::StageValues, kNumStages> stageCurrent;
    std::array<calor::Tilt, kMaxChannels> outTilt;
    float outLowCur = 1.0f, outHighCur = 1.0f, widthCur = 1.0f;

    double baseRate = 44100.0;
    int maxBlock = 512;
    int numChannels = 2;

    float lfoPhase = 0.0f;
    float envelope = 0.0f;
    float envAttack = 0.0f, envRelease = 0.0f;

    float dryPower = 0.0f, wetPower = 0.0f, levelCoeff = 0.0f;
    float autoGainCur = 1.0f;
    float meter = 0.0f;

    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CalorProcessor)
};
