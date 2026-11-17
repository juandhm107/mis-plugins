#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include "SynthEngine.h"
#include "Presets.h"

inline juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

class DembowProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kNumVoices = 16;

    DembowProcessor();
    ~DembowProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return (int) dembow::presets().size(); }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    std::atomic<int> currentProgram { 0 };
    std::atomic<int> programChanges { 0 };   // la interfaz lo vigila para refrescarse

private:
    void readSettings();
    void handleMidi (const juce::MidiMessage& m);
    void noteOn (int note, float vel);
    void noteOff (int note);
    void allNotesOff();
    void renderVoices (float* l, float* r, int start, int num);
    int findVoice (int note);
    float lastPitchFor (int note) const;

    dembow::Settings settings;
    dembow::Globals globals;
    std::array<dembow::Voice, kNumVoices> voices;

    int voiceMode = dembow::Poli;
    bool sustainPedal = false;
    std::vector<int> heldNotes;       // pila de notas (modo mono)
    float lastPitch = -1.0f;          // para el glide
    uint32_t seedCounter = 1;

    // Efectos
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    std::vector<float> delayL, delayR;
    int delayWrite = 0;
    float delaySmoothed = 0.25f;
    float dlyLpL = 0.0f, dlyLpR = 0.0f;
    juce::SmoothedValue<float> volume;
    juce::AudioBuffer<float> work;

    double sampleRateD = 44100.0;

    struct P
    {
        std::atomic<float>* v = nullptr;
        float get() const { return v->load(); }
        int i() const { return (int) std::lround (v->load()); }
    };
    P o1wave, o1oct, o1semi, o1fine, o1level, o2wave, o2oct, o2semi, o2fine, o2level;
    P pw, detune, fm, fmenv, sub, noise;
    P ftype, cutoff, reso, fenv, keytrk, drive;
    P aa, ad, as, ar, fa, fd, fs, fr;
    P lfowave, lfodest, lforate, lfodepth, lfofade;
    P mode, glide, velsens;
    P chorusMix, dlymix, dlytime, dlyfb, revmix, revsize, vol;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DembowProcessor)
};
