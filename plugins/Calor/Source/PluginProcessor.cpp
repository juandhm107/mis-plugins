#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace calor;

namespace
{
    juce::String sid (int stage, const char* name) { return "s" + juce::String (stage + 1) + "_" + name; }

    struct Preset
    {
        const char* name;
        std::vector<std::pair<juce::String, float>> values;
    };

    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> list = {
            { "Inicial", {} },
            { "Calidez suave", { { "s1_type", Tubo }, { "s1_drive", 8.0f }, { "s1_mix", 70.0f }, { "s1_tone", -0.2f },
                                 { "auto_gain", 1.0f } } },
            { "Cinta caliente", { { "s1_type", Cinta }, { "s1_drive", 14.0f }, { "s1_tone", -0.3f },
                                  { "s2_on", 1.0f }, { "s2_type", Tubo }, { "s2_drive", 6.0f }, { "s2_mix", 50.0f },
                                  { "auto_gain", 1.0f } } },
            { "Bajo con cuerpo", { { "mode", 1.0f }, { "xover_lo", 180.0f }, { "xover_hi", 2500.0f },
                                   { "s1_type", Tubo }, { "s1_drive", 16.0f },
                                   { "s2_on", 1.0f }, { "s2_type", Diodo }, { "s2_drive", 10.0f }, { "s2_mix", 60.0f },
                                   { "s3_on", 0.0f }, { "auto_gain", 1.0f } } },
            { "Brillo en agudos", { { "mode", 1.0f }, { "xover_lo", 300.0f }, { "xover_hi", 4000.0f },
                                    { "s1_on", 0.0f }, { "s2_on", 0.0f },
                                    { "s3_on", 1.0f }, { "s3_type", Cinta }, { "s3_drive", 18.0f }, { "s3_mix", 60.0f },
                                    { "s3_tone", 0.3f } } },
            { "Fuzz sucio", { { "s1_type", Fuzz }, { "s1_drive", 24.0f }, { "s1_tone", 0.2f },
                              { "s2_on", 1.0f }, { "s2_type", Recorte }, { "s2_drive", 6.0f }, { "s2_tone", -0.4f },
                              { "mix", 80.0f }, { "auto_gain", 1.0f } } },
            { "Pliegue metalico", { { "s1_type", Pliegue }, { "s1_drive", 18.0f }, { "s1_mix", 80.0f },
                                    { "lfo_rate", 0.3f }, { "lfo_depth", 0.5f }, { "width", 140.0f },
                                    { "auto_gain", 1.0f } } },
            { "Lo-fi roto", { { "s1_type", Bits }, { "s1_drive", 22.0f }, { "s1_tone", -0.5f },
                              { "s2_on", 1.0f }, { "s2_type", Cinta }, { "s2_drive", 8.0f },
                              { "width", 70.0f }, { "auto_gain", 1.0f } } },
            { "Octava rota", { { "s1_type", Rectificador }, { "s1_drive", 12.0f }, { "s1_mix", 60.0f },
                               { "s2_on", 1.0f }, { "s2_type", Tubo }, { "s2_drive", 10.0f },
                               { "auto_gain", 1.0f } } },
            { "Respira (LFO)", { { "s1_type", Tubo }, { "s1_drive", 18.0f },
                                 { "lfo_rate", 2.0f }, { "lfo_depth", 0.8f }, { "auto_gain", 1.0f } } },
            { "Bombeo (envolvente)", { { "s1_type", Diodo }, { "s1_drive", 20.0f }, { "env_amount", -0.8f },
                                       { "s2_on", 1.0f }, { "s2_type", Cinta }, { "s2_drive", 6.0f },
                                       { "auto_gain", 1.0f } } },
            { "Destruccion total", { { "s1_type", Fuzz }, { "s1_drive", 30.0f },
                                     { "s2_on", 1.0f }, { "s2_type", Pliegue }, { "s2_drive", 20.0f },
                                     { "s3_on", 1.0f }, { "s3_type", Bits }, { "s3_drive", 18.0f },
                                     { "heat", 0.8f }, { "auto_gain", 1.0f } } },
        };
        return list;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout CalorProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray types { "Tubo", "Cinta", "Diodo", "Fuzz", "Pliegue", "Rectificador", "Bits", "Recorte" };
    const ShapeType defaultTypes[kNumStages] = { Tubo, Cinta, Fuzz };

    auto pct = AudioParameterFloatAttributes().withLabel ("%")
                   .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; });
    auto db  = AudioParameterFloatAttributes().withLabel ("dB")
                   .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; });
    auto hz  = AudioParameterFloatAttributes().withLabel ("Hz")
                   .withStringFromValueFunction ([] (float v, int) {
                       return v < 1000.0f ? String (roundToInt (v)) + " Hz" : String (v / 1000.0f, 2) + " kHz"; });
    auto rate = AudioParameterFloatAttributes().withLabel ("Hz")
                   .withStringFromValueFunction ([] (float v, int) { return String (v, 2) + " Hz"; });
    auto plain = AudioParameterFloatAttributes()
                   .withStringFromValueFunction ([] (float v, int) { return String (v, 2); });

    for (int s = 0; s < kNumStages; ++s)
    {
        const auto n = u8 ("Etapa ") + String (s + 1) + " ";
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { sid (s, "on"), 1 }, n + u8 ("Activa"), s == 0));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { sid (s, "type"), 1 }, n + u8 ("Tipo"), types, (int) defaultTypes[s]));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { sid (s, "drive"), 1 }, n + u8 ("Saturación"),
                                                           NormalisableRange<float> (0.0f, 36.0f, 0.01f), s == 0 ? 12.0f : 6.0f, db));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { sid (s, "mix"), 1 }, n + u8 ("Mezcla"),
                                                           NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, pct));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { sid (s, "tone"), 1 }, n + u8 ("Tono"),
                                                           NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, plain));
    }

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, u8 ("Modo"), StringArray { "Serie", "Bandas" }, 0));

    NormalisableRange<float> loRange (40.0f, 1200.0f, 1.0f); loRange.setSkewForCentre (250.0f);
    NormalisableRange<float> hiRange (800.0f, 12000.0f, 1.0f); hiRange.setSkewForCentre (3000.0f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "xover_lo", 1 }, u8 ("Cruce graves"), loRange, 250.0f, hz));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "xover_hi", 1 }, u8 ("Cruce agudos"), hiRange, 3000.0f, hz));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "heat", 1 }, u8 ("Calor (X)"),
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, plain));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "character", 1 }, u8 ("Brillo (Y)"),
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, plain));

    NormalisableRange<float> rateRange (0.05f, 20.0f, 0.001f); rateRange.setSkewForCentre (1.5f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "lfo_rate", 1 }, u8 ("LFO velocidad"), rateRange, 1.0f, rate));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "lfo_depth", 1 }, u8 ("LFO profundidad"),
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, plain));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "env_amount", 1 }, u8 ("Envolvente"),
                                                       NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, plain));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "width", 1 }, u8 ("Anchura"),
                                                       NormalisableRange<float> (0.0f, 200.0f, 0.1f), 100.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "out_tone", 1 }, u8 ("Tono salida"),
                                                       NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, plain));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, u8 ("Mezcla global"),
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, u8 ("Salida"),
                                                       NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f, db));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "auto_gain", 1 }, u8 ("Nivel automático"), false));

    return layout;
}

CalorProcessor::CalorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CalorState", createLayout())
{
    for (int s = 0; s < kNumStages; ++s)
    {
        stageParams[(size_t) s].on    = apvts.getRawParameterValue (sid (s, "on"));
        stageParams[(size_t) s].type  = apvts.getRawParameterValue (sid (s, "type"));
        stageParams[(size_t) s].drive = apvts.getRawParameterValue (sid (s, "drive"));
        stageParams[(size_t) s].mix   = apvts.getRawParameterValue (sid (s, "mix"));
        stageParams[(size_t) s].tone  = apvts.getRawParameterValue (sid (s, "tone"));
    }

    pMode       = apvts.getRawParameterValue ("mode");
    pXoverLo    = apvts.getRawParameterValue ("xover_lo");
    pXoverHi    = apvts.getRawParameterValue ("xover_hi");
    pHeat       = apvts.getRawParameterValue ("heat");
    pCharacter  = apvts.getRawParameterValue ("character");
    pLfoRate    = apvts.getRawParameterValue ("lfo_rate");
    pLfoDepth   = apvts.getRawParameterValue ("lfo_depth");
    pEnvAmount  = apvts.getRawParameterValue ("env_amount");
    pWidth      = apvts.getRawParameterValue ("width");
    pOutTone    = apvts.getRawParameterValue ("out_tone");
    pMix        = apvts.getRawParameterValue ("mix");
    pOutput     = apvts.getRawParameterValue ("output");
    pAutoGain   = apvts.getRawParameterValue ("auto_gain");

    lp1.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    hp1.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    lp2.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    hp2.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    ap2.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
}

bool CalorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void CalorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    baseRate = sampleRate;
    maxBlock = juce::jmax (1, samplesPerBlock);
    numChannels = juce::jlimit (1, kMaxChannels, getTotalNumOutputChannels());

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numChannels, (size_t) kOsOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) maxBlock);
    oversampling->reset();

    const auto latency = oversampling->getLatencyInSamples();
    setLatencySamples ((int) std::round (latency));

    const double osRate = sampleRate * kOsFactor;
    juce::dsp::ProcessSpec osSpec { osRate, (juce::uint32) (maxBlock * kOsFactor), (juce::uint32) numChannels };
    for (auto* f : { &lp1, &hp1, &lp2, &hp2, &ap2 })
    {
        f->prepare (osSpec);
        f->reset();
    }

    for (auto& stage : stageState)
        for (auto& ch : stage)
        {
            ch.tilt.setup (osRate);
            ch.dc.setup (osRate);
            ch.reset();
        }

    for (auto& t : outTilt)
    {
        t.setup (sampleRate);
        t.reset();
    }

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, (juce::uint32) numChannels };
    dryWet.prepare (spec);
    dryWet.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    dryWet.setWetLatency (latency);
    dryWet.reset();

    outputGain.prepare (spec);
    outputGain.setRampDurationSeconds (0.05);
    outputGain.setGainDecibels (pOutput->load());
    outputGain.reset();

    envAttack  = 1.0f - std::exp (-1.0f / (0.005f * (float) sampleRate));
    envRelease = 1.0f - std::exp (-1.0f / (0.150f * (float) sampleRate));
    levelCoeff = 1.0f - std::exp (-1.0f / (0.300f * (float) sampleRate));
    envelope = 0.0f;
    dryPower = wetPower = 1.0e-6f;
    autoGainCur = 1.0f;
    lfoPhase = 0.0f;

    for (int s = 0; s < kNumStages; ++s)
        stageCurrent[(size_t) s] = computeStageTarget (s, 0.0f);

    const float outTone = pOutTone->load();
    outLowCur  = dbToGain (-6.0f * outTone);
    outHighCur = dbToGain (6.0f * outTone);
    widthCur   = pWidth->load() * 0.01f;
}

StageValues CalorProcessor::computeStageTarget (int stage, float modDb) const
{
    const auto& p = stageParams[(size_t) stage];
    const float heatDb = (pHeat->load() - 0.5f) * 24.0f;
    const float driveDb = juce::jlimit (-12.0f, 48.0f, p.drive->load() + heatDb + modDb);
    const float tone = juce::jlimit (-1.0f, 1.0f, p.tone->load() + (pCharacter->load() - 0.5f) * 1.6f);

    StageValues v;
    v.gain  = dbToGain (driveDb);
    v.comp  = dbToGain (-juce::jmax (0.0f, driveDb) * 0.5f);
    v.mix   = p.on->load() > 0.5f ? p.mix->load() * 0.01f : 0.0f;
    v.gLow  = dbToGain (-6.0f * tone);
    v.gHigh = dbToGain (6.0f * tone);
    v.crush = juce::jlimit (0.0f, 1.0f, driveDb / 36.0f);
    return v;
}

void CalorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (oversampling == nullptr)
        return;

    juce::dsp::AudioBlock<float> full (buffer);
    full = full.getSubsetChannelBlock (0, (size_t) juce::jmin ((int) full.getNumChannels(), numChannels));

    for (size_t start = 0; start < full.getNumSamples(); start += (size_t) maxBlock)
    {
        const auto len = juce::jmin ((size_t) maxBlock, full.getNumSamples() - start);
        processSlice (full.getSubBlock (start, len));
    }

    outputLevel.store (meter);
}

void CalorProcessor::processSlice (juce::dsp::AudioBlock<float> block)
{
    const int n = (int) block.getNumSamples();
    const int chans = (int) block.getNumChannels();
    const bool bands = pMode->load() > 0.5f;

    // Tipos de etapa
    int types[kNumStages];
    for (int s = 0; s < kNumStages; ++s)
        types[s] = (int) stageParams[(size_t) s].type->load();

    // Filtros de cruce (modo bandas)
    if (bands)
    {
        const float f1 = pXoverLo->load();
        const float f2 = juce::jmax (pXoverHi->load(), f1 * 1.5f);
        lp1.setCutoffFrequency (f1);
        hp1.setCutoffFrequency (f1);
        lp2.setCutoffFrequency (f2);
        hp2.setCutoffFrequency (f2);
        ap2.setCutoffFrequency (f2);
    }

    dryWet.pushDrySamples (block);

    // ---------- Entrada: envolvente, nivel seco y valores de control por trozo ----------
    const int numChunks = (n + kChunk - 1) / kChunk;
    float lastMod = 0.0f;

    auto osBlock = oversampling->processSamplesUp (block);

    const float lfoRate = pLfoRate->load();
    const float lfoDepth = pLfoDepth->load();
    const float envAmount = pEnvAmount->load();

    for (int c = 0; c < numChunks; ++c)
    {
        const int start = c * kChunk;
        const int len = juce::jmin (kChunk, n - start);

        // Envolvente y potencia de la senal seca
        for (int i = start; i < start + len; ++i)
        {
            float peak = 0.0f, power = 0.0f;
            for (int ch = 0; ch < chans; ++ch)
            {
                const float x = block.getSample (ch, i);
                peak = juce::jmax (peak, std::abs (x));
                power += x * x;
            }
            power /= (float) chans;
            envelope += (peak > envelope ? envAttack : envRelease) * (peak - envelope);
            dryPower += levelCoeff * (power - dryPower);
        }

        // Modulacion: LFO + envolvente
        lfoPhase += lfoRate * (float) len / (float) baseRate;
        lfoPhase -= std::floor (lfoPhase);
        const float lfo = std::sin (2.0f * kPi * lfoPhase);
        const float envDb = juce::Decibels::gainToDecibels (envelope, -60.0f);
        const float envNorm = juce::jlimit (0.0f, 1.0f, (envDb + 60.0f) / 60.0f);
        const float modDb = lfo * lfoDepth * 12.0f + envAmount * envNorm * 18.0f;
        lastMod = modDb;

        StageValues target[kNumStages];
        StageValues from[kNumStages];
        for (int s = 0; s < kNumStages; ++s)
        {
            target[s] = computeStageTarget (s, modDb);
            from[s] = stageCurrent[(size_t) s];
        }

        const int osStart = start * kOsFactor;
        const int osLen = len * kOsFactor;
        const float invLen = 1.0f / (float) osLen;

        for (int ch = 0; ch < chans; ++ch)
        {
            float* data = osBlock.getChannelPointer ((size_t) ch) + osStart;
            auto& st0 = stageState[0][(size_t) ch];
            auto& st1 = stageState[1][(size_t) ch];
            auto& st2 = stageState[2][(size_t) ch];

            for (int i = 0; i < osLen; ++i)
            {
                const float t = (float) (i + 1) * invLen;
                const auto v0 = lerp (from[0], target[0], t);
                const auto v1 = lerp (from[1], target[1], t);
                const auto v2 = lerp (from[2], target[2], t);

                const float x = data[i];
                float y;

                if (bands)
                {
                    float low  = lp1.processSample (ch, x);
                    const float rest = hp1.processSample (ch, x);
                    const float mid  = lp2.processSample (ch, rest);
                    const float high = hp2.processSample (ch, rest);
                    low = ap2.processSample (ch, low);

                    y = processStage (low,  types[0], v0, st0, kOsFactor)
                      + processStage (mid,  types[1], v1, st1, kOsFactor)
                      + processStage (high, types[2], v2, st2, kOsFactor);
                }
                else
                {
                    y = processStage (x, types[0], v0, st0, kOsFactor);
                    y = processStage (y, types[1], v1, st1, kOsFactor);
                    y = processStage (y, types[2], v2, st2, kOsFactor);
                }

                data[i] = y;
            }
        }

        for (int s = 0; s < kNumStages; ++s)
            stageCurrent[(size_t) s] = target[s];
    }

    modulationValue.store (juce::jlimit (-1.0f, 1.0f, lastMod / 24.0f));

    oversampling->processSamplesDown (block);

    // ---------- Salida: nivel automatico, tono, anchura ----------
    const bool autoGain = pAutoGain->load() > 0.5f;
    const float outTone = pOutTone->load();
    const float outLowTarget  = dbToGain (-6.0f * outTone);
    const float outHighTarget = dbToGain (6.0f * outTone);
    const float widthTarget = pWidth->load() * 0.01f;

    float peakOut = 0.0f;

    for (int c = 0; c < numChunks; ++c)
    {
        const int start = c * kChunk;
        const int len = juce::jmin (kChunk, n - start);

        float agTarget = 1.0f;
        if (autoGain)
            agTarget = juce::jlimit (0.0625f, 2.0f, std::sqrt ((dryPower + 1.0e-9f) / (wetPower + 1.0e-9f)));

        const float agStep = (agTarget - autoGainCur) / (float) len;
        const float lowStep = (outLowTarget - outLowCur) / (float) len;
        const float highStep = (outHighTarget - outHighCur) / (float) len;
        const float widthStep = (widthTarget - widthCur) / (float) len;

        for (int i = start; i < start + len; ++i)
        {
            autoGainCur += agStep;
            outLowCur += lowStep;
            outHighCur += highStep;
            widthCur += widthStep;

            float power = 0.0f;
            for (int ch = 0; ch < chans; ++ch)
            {
                const float w = block.getSample (ch, i);
                power += w * w;
            }
            wetPower += levelCoeff * (power / (float) chans - wetPower);

            if (chans == 2)
            {
                float l = block.getSample (0, i) * autoGainCur;
                float r = block.getSample (1, i) * autoGainCur;
                l = outTilt[0].process (l, outLowCur, outHighCur);
                r = outTilt[1].process (r, outLowCur, outHighCur);
                const float m = 0.5f * (l + r);
                const float sd = 0.5f * (l - r) * widthCur;
                block.setSample (0, i, m + sd);
                block.setSample (1, i, m - sd);
            }
            else
            {
                const float x = block.getSample (0, i) * autoGainCur;
                block.setSample (0, i, outTilt[0].process (x, outLowCur, outHighCur));
            }
        }
    }

    dryWet.setWetMixProportion (pMix->load() * 0.01f);
    dryWet.mixWetSamples (block);

    outputGain.setGainDecibels (pOutput->load());
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    outputGain.process (ctx);

    for (int ch = 0; ch < chans; ++ch)
        for (int i = 0; i < n; ++i)
            peakOut = juce::jmax (peakOut, std::abs (block.getSample (ch, i)));

    meter = juce::jmax (peakOut, meter * 0.85f);
}

//==============================================================================
int CalorProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String CalorProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) presets().size()))
        return u8 (presets()[(size_t) index].name);
    return {};
}

void CalorProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;

    currentProgram = index;

    // Primero todo a sus valores por defecto, luego los del preset.
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : presets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

void CalorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void CalorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            apvts.replaceState (tree);

            // Avisa al host de todos los valores, aunque no hayan cambiado
            // (algunos hosts guardan una copia del valor de los interruptores).
            for (auto* p : getParameters())
                p->setValueNotifyingHost (p->getValue());
        }
}

juce::AudioProcessorEditor* CalorProcessor::createEditor() { return new CalorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CalorProcessor(); }
