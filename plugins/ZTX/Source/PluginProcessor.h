#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Voice.h"
#include "seq/SeqPlayer.h"
#include "fx/Effects.h"

namespace ztx
{
class ZtxProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    ZtxProcessor();
    ~ZtxProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ZTX Organic Glitch Lab"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return currentPresetName; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- parameters (message thread helpers)
    juce::AudioProcessorValueTreeState apvts;
    float getParam (int pid) const { return raws[(size_t) pid]->load(); }
    void setParam (int pid, float actual);
    void setParamNorm (int pid, float norm);
    void resetAllParams();
    juce::RangedAudioParameter* param (int pid) { return params[(size_t) pid]; }

    // ---- pattern (message thread)
    Pattern& pattern() { return variations[(size_t) currentVar]; }
    int currentVariation() const { return currentVar; }
    void selectVariation (int v);
    void copyVariationTo (int dest);
    void commitPattern();
    void pushUndo();
    bool undo();
    void generate();
    void mutate (float strength = 1.0f);
    GenSettings genSettings() const;
    bool frozen = false;
    std::array<Pattern, kVariations> variations;

    // ---- granular source
    bool loadSample (const juce::File&);
    void useDefaultSample();
    void startCapture (bool fromInput, float seconds = 4.0f);
    bool isCapturing() const { return capturing.load(); }
    juce::String getSampleName();
    std::shared_ptr<const SampleData> getSample();
    juce::File samplePath;

    // ---- UI feedback (lock-free)
    std::atomic<float> modDisplay[NUM_PARAMS];
    std::atomic<float> srcDisplay[NUM_SRC];
    std::atomic<int> playStep[kTracks];
    std::atomic<float> trackFlash[kTracks];
    std::atomic<int> laneStepDisplay { 0 };
    std::atomic<juce::uint32> patternEpoch { 1 };
    std::atomic<bool> seqRunning { false };
    std::atomic<int> barCounter { 0 };
    std::atomic<int> activeVoices { 0 };
    std::atomic<float> outLevel { 0 };
    static constexpr int kScope = 2048;
    float scope[kScope] {};
    std::atomic<int> scopePos { 0 };

    juce::MidiKeyboardState keyboardState;
    juce::String currentPresetName { "Init" };
    int presetIndex = -1;   // factory index, -1 = user / init
    juce::UndoManager* undoManager = nullptr;

    // Offline testing hook: force a tempo/transport when no host playhead exists.
    double testBpm = 0.0;

private:
    void timerCallback() override;
    void startVoice (const VoiceStart&, const ParamFrame&);
    void handleMidi (const juce::MidiMessage&, int offset, const ParamFrame&);

    std::vector<juce::RangedAudioParameter*> params;
    std::vector<std::atomic<float>*> raws;
    float raw[NUM_PARAMS] {};

    Voice voices[24];
    uint64_t voiceCounter = 0;
    ModEngine mod;
    FxChain fx;
    SeqPlayer seq;
    ParamFrame frame;
    SeqEvent events[SeqPlayer::kMaxEvents];

    Pattern livePattern, sharedPattern;
    juce::SpinLock patLock;
    std::atomic<bool> patDirty { true };
    std::vector<Pattern> undoStack;
    int currentVar = 0;

    std::shared_ptr<const SampleData> sample;
    juce::SpinLock sampleLock;
    std::vector<std::shared_ptr<const SampleData>> graveyard;

    juce::AudioBuffer<float> capBuf;
    std::atomic<bool> capturing { false }, captureReady { false }, captureFromInput { false };
    int capPos = 0, capLen = 0;

    double sr = 48000.0;
    double internalPpq = 0, freePpq = 0;
    int heldCount = 0, lastHeldNote = 60;
    bool heldNotes[128] {};
    float modWheel = 0, pitchBend = 0, follower = 0, limEnv = 0, limRelease = 0.9999f;
    int lastBar = -1, lastEvolveBar = 0;
    Rng rng { 0xC0FFEEu };
    juce::AudioFormatManager formats;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZtxProcessor)
};
} // namespace ztx
