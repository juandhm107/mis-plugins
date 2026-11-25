#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ztx
{
static const juce::Identifier kPatternsId ("ZTXPatterns");
static const juce::Identifier kMetaId ("ZTXMeta");

// A built-in "organic debris" source so the granular engine is alive without loading.
static std::shared_ptr<SampleData> makeDefaultSample (double rate)
{
    auto sd = std::make_shared<SampleData>();
    const int n = (int) (rate * 4.0);
    sd->buffer.setSize (2, n);
    sd->buffer.clear();
    sd->sampleRate = rate;
    sd->name = "Built-in: Organic Debris";
    Rng r (1234u);
    float* L = sd->buffer.getWritePointer (0);
    float* R = sd->buffer.getWritePointer (1);
    const float fr = (float) rate;
    // rubber blips
    for (int e = 0; e < 26; ++e)
    {
        const int start = (int) (r.uni() * (float) (n - fr * 0.5f));
        const float f0 = 60.0f + r.uni() * r.uni() * 900.0f, dec = 0.03f + r.uni() * 0.35f, pan = r.bi();
        const float stretch = 0.5f + r.uni() * 2.0f;
        float ph = 0;
        for (int i = 0; i < (int) (dec * fr * 5) && start + i < n; ++i)
        {
            const float t = (float) i / fr;
            ph += f0 * std::exp2 (stretch * std::exp (-t * 30.0f)) / fr;
            const float s = std::sin (kTwoPi * ph + 1.5f * std::exp (-t * 40.0f) * std::sin (kTwoPi * ph * 2.0f)) * std::exp (-t / dec) * 0.5f;
            L[start + i] += s * (1.0f - pan) * 0.7f;
            R[start + i] += s * (1.0f + pan) * 0.7f;
        }
    }
    // clicks through resonators
    for (int e = 0; e < 40; ++e)
    {
        Mode m; m.set (400.0f + r.uni() * 6000.0f, 0.01f + r.uni() * 0.15f, fr);
        const int start = (int) (r.uni() * (float) (n - fr * 0.2f));
        const float pan = r.bi(), amp = 0.1f + r.uni() * 0.3f;
        for (int i = 0; i < (int) (fr * 0.2f); ++i)
        {
            const float s = m.process (i == 0 ? amp : (i < 30 ? r.bi() * amp * 0.3f : 0.0f));
            L[start + i] += s * (1.0f - pan); R[start + i] += s * (1.0f + pan);
        }
    }
    // swelling resonant noise + metallic drone
    Svf f1, f2;
    float dph = 0, mph = 0;
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) i / fr;
        const float sweep = 300.0f + 2500.0f * (0.5f + 0.5f * std::sin (kTwoPi * t * 0.31f));
        f1.set (sweep, 6.0f, fr); f2.set (sweep * 1.5f, 5.0f, fr);
        const float env = 0.5f + 0.5f * std::sin (kTwoPi * t * 0.5f - 1.0f);
        const float nz = r.bi();
        f1.process (nz); f2.process (nz);
        dph += 55.0f / fr; mph += 55.0f * 3.71f / fr;
        const float drone = std::sin (kTwoPi * dph + 2.0f * std::sin (kTwoPi * mph)) * 0.08f;
        L[i] += f1.bp * env * 0.25f + drone;
        R[i] += f2.bp * env * 0.25f + drone * 0.8f;
    }
    const float peak = std::max (sd->buffer.getMagnitude (0, 0, n), sd->buffer.getMagnitude (1, 0, n));
    if (peak > 0) sd->buffer.applyGain (0.8f / peak);
    return sd;
}

ZtxProcessor::ZtxProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ZTX", createLayout())
{
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        params.push_back (apvts.getParameter (info (i).id));
        raws.push_back (apvts.getRawParameterValue (info (i).id));
        jassert (params.back() != nullptr && raws.back() != nullptr);
    }
    for (auto& m : modDisplay) m.store (0);
    for (auto& s : srcDisplay) s.store (0);
    for (auto& s : playStep) s.store (0);
    for (auto& f : trackFlash) f.store (0);
    formats.registerBasicFormats();
    sample = makeDefaultSample (48000.0);
    startTimerHz (20);
}

ZtxProcessor::~ZtxProcessor() { stopTimer(); }

bool ZtxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    const auto in = layouts.getMainInputChannelSet();
    return in.isDisabled() || in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void ZtxProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& v : voices) v.prepare ((float) sr);
    mod.prepare (sr);
    fx.prepare (sr);
    limRelease = (float) std::exp (-1.0 / (0.15 * sr));
    capBuf.setSize (2, (int) (sr * 10.0));
    capBuf.clear();
    keyboardState.reset();
}

// =====================================================================================
//  AUDIO
// =====================================================================================
void ZtxProcessor::startVoice (const VoiceStart& vs, const ParamFrame& f)
{
    // choke: one voice per sequencer track
    if (vs.track >= 0)
        for (auto& v : voices)
            if (v.active && v.cfg.track == vs.track && ! v.releasing) { v.kill(); }

    Voice* target = nullptr;
    for (auto& v : voices) if (! v.active) { target = &v; break; }
    if (target == nullptr)
    {
        target = &voices[0];
        for (auto& v : voices) if (v.age < target->age) target = &v;
    }
    target->age = ++voiceCounter;
    target->start (vs, f);
}

void ZtxProcessor::handleMidi (const juce::MidiMessage& m, int offset, const ParamFrame& f)
{
    const int mode = (int) raw[seqMidi];
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        if (! heldNotes[note]) { heldNotes[note] = true; ++heldCount; }
        if (heldCount == 1 && mode > 0) internalPpq = 0;
        lastHeldNote = note;
        if (mode == 0)
        {
            VoiceStart vs;
            vs.engine = (int) raw[labEngine];
            vs.note = note;
            vs.velocity = m.getFloatVelocity();
            vs.gated = raw[gateMode] > 0.5f;
            vs.delay = offset;
            vs.seed = rng.next();
            startVoice (vs, f);
        }
    }
    else if (m.isNoteOff())
    {
        const int note = m.getNoteNumber();
        if (heldNotes[note]) { heldNotes[note] = false; heldCount = std::max (0, heldCount - 1); }
        for (auto& v : voices)
            if (v.active && v.cfg.track < 0 && v.cfg.note == note && ! v.releasing) v.noteOff();
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        for (auto& v : voices) if (v.active) v.noteOff();
        std::fill (std::begin (heldNotes), std::end (heldNotes), false);
        heldCount = 0;
    }
    else if (m.isController() && m.getControllerNumber() == 1) modWheel = (float) m.getControllerValue() / 127.0f;
    else if (m.isPitchWheel()) pitchBend = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f;
}

void ZtxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    for (int i = 0; i < NUM_PARAMS; ++i) raw[i] = raws[(size_t) i]->load (std::memory_order_relaxed);

    if (patDirty.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (patLock);
        if (lock.isLocked()) { livePattern = sharedPattern; patDirty = false; }
    }
    std::shared_ptr<const SampleData> smp;
    {
        const juce::SpinLock::ScopedTryLockType lock (sampleLock);
        if (lock.isLocked()) smp = sample;
    }

    // ---- transport
    double bpm = raw[seqBpm], hostPpq = 0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) hostPpq = *q;
            hostPlaying = pos->getIsPlaying();
        }
    if (testBpm > 0) bpm = testBpm;
    bpm = juce::jlimit (20.0, 400.0, bpm);
    const double spb = sr * 60.0 / bpm;
    const int midiMode = (int) raw[seqMidi];
    EngineCtx ctx { (float) sr, bpm, smp.get() };

    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;
    const float* inL = numIn > 0 ? buffer.getReadPointer (0) : nullptr;
    const float* inR = numIn > 1 ? buffer.getReadPointer (1) : inL;
    const float master = juce::Decibels::decibelsToGain (raw[masterVol], -60.0f);

    auto midiIt = midi.begin();
    float destDisp[NUM_PARAMS];
    float peakOut = 0;
    bool running = false;
    double ppqEnd = 0;

    for (int pos = 0; pos < n; pos += kCtrlBlock)
    {
        const int len = std::min (kCtrlBlock, n - pos);

        // ---- clock for this control block
        double ppq;
        if (midiMode == 0)
        {
            if (hostPlaying) { ppq = hostPpq + pos / spb; running = true; }
            else if (raw[seqRun] > 0.5f) { ppq = internalPpq; internalPpq += len / spb; running = true; }
            else { ppq = internalPpq; running = false; }
        }
        else
        {
            running = heldCount > 0;
            ppq = internalPpq;
            if (running) internalPpq += len / spb;
        }
        freePpq += len / spb;
        const double clockPpq = running ? ppq : freePpq;
        ppqEnd = clockPpq;

        ModEngine::Clock clk { clockPpq, bpm, running || hostPlaying, (int) std::floor (clockPpq / 0.25) };
        mod.compute (raw, len, clk, livePattern, follower, modWheel, frame, destDisp);
        if (pitchBend != 0.0f) frame.actual[labPitch] += pitchBend;

        // ---- MIDI in this block
        while (midiIt != midi.end() && (*midiIt).samplePosition < pos + len)
        {
            const auto meta = *midiIt;
            handleMidi (meta.getMessage(), juce::jlimit (0, len - 1, meta.samplePosition - pos), frame);
            ++midiIt;
        }

        // ---- sequencer
        if (running)
        {
            SeqPlayer::Settings st;
            st.swing = raw[seqSwing]; st.human = raw[seqHuman];
            st.probScale = frame.actual[seqProb];
            st.transpose = midiMode == 2 ? lastHeldNote - 60 : 0;
            const int count = seq.collect (livePattern, ppq, len, spb, st, events);
            for (int e = 0; e < count; ++e)
            {
                const auto& ev = events[e];
                const Track& tr = livePattern.tracks[(size_t) ev.track];
                VoiceStart vs;
                vs.engine = tr.engine; vs.note = ev.note; vs.velocity = ev.velocity;
                vs.track = ev.track; vs.gateSamples = ev.gateSamples; vs.delay = ev.sampleOffset;
                vs.pan = tr.pan; vs.level = tr.level; vs.decayMod = tr.decayMod; vs.colourMod = tr.colourMod;
                vs.gated = raw[gateMode] > 0.5f;
                vs.seed = rng.next();
                startVoice (vs, frame);
                trackFlash[ev.track].store (1.0f);
            }
            for (int t = 0; t < kTracks; ++t) playStep[t].store (seq.currentStep[t]);
        }

        // ---- voices
        float tl[kCtrlBlock] {}, tr[kCtrlBlock] {};
        int nActive = 0;
        for (auto& v : voices)
            if (v.active) { v.render (tl, tr, len, frame, ctx); ++nActive; }
        activeVoices.store (nActive);

        float sum = 0;
        for (int i = 0; i < len; ++i) sum += tl[i] * tl[i] + tr[i] * tr[i];
        follower = std::sqrt (sum / (2.0f * (float) len));

        // ---- capture into granular buffer
        if (capturing.load (std::memory_order_relaxed))
        {
            const bool fromIn = captureFromInput.load();
            float* cl = capBuf.getWritePointer (0);
            float* cr = capBuf.getWritePointer (1);
            for (int i = 0; i < len && capPos < capLen; ++i, ++capPos)
            {
                cl[capPos] = fromIn ? (inL ? inL[pos + i] : 0.0f) : tl[i];
                cr[capPos] = fromIn ? (inR ? inR[pos + i] : 0.0f) : tr[i];
            }
            if (capPos >= capLen) { capturing = false; captureReady = true; }
        }

        // ---- destruction
        FxClock fclk { clockPpq, bpm };
        fx.process (tl, tr, len, frame.actual, fclk);

        for (int i = 0; i < len; ++i)
        {
            float l = tl[i] * master, r = tr[i] * master;
            if (! std::isfinite (l) || ! std::isfinite (r)) { l = r = 0; fx.reset(); }
            // safety peak limiter (instant attack, 150 ms release) - this is a destructive instrument
            const float pk = std::max (std::abs (l), std::abs (r));
            limEnv = pk > limEnv ? pk : limEnv * limRelease;
            if (limEnv > 0.89f) { const float g = 0.89f / limEnv; l *= g; r *= g; }
            l = std::clamp (l, -1.0f, 1.0f); r = std::clamp (r, -1.0f, 1.0f);
            outL[pos + i] = l;
            if (outR != outL) outR[pos + i] = r;
            peakOut = std::max (peakOut, std::abs (l) + std::abs (r));
        }
        int sp = scopePos.load (std::memory_order_relaxed);
        for (int i = 0; i < len; ++i) { scope[sp] = (outL[pos + i] + outR[pos + i]) * 0.5f; sp = (sp + 1) & (kScope - 1); }
        scopePos.store (sp, std::memory_order_relaxed);
    }
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);

    // ---- UI feedback
    for (int i = 0; i < NUM_PARAMS; ++i) modDisplay[i].store (destDisp[i], std::memory_order_relaxed);
    for (int s = 0; s < NUM_SRC; ++s) srcDisplay[s].store (mod.src[s], std::memory_order_relaxed);
    seqRunning.store (running);
    laneStepDisplay.store ((int) std::floor (ppqEnd / 0.25) % kLaneSteps);
    barCounter.store ((int) std::floor (ppqEnd / 4.0));
    outLevel.store (std::max (outLevel.load() * 0.9f, peakOut * 0.5f));
}

// =====================================================================================
//  MESSAGE-THREAD SERVICES
// =====================================================================================
void ZtxProcessor::timerCallback()
{
    if (captureReady.exchange (false))
    {
        auto sd = std::make_shared<SampleData>();
        sd->buffer.setSize (2, capLen);
        sd->buffer.copyFrom (0, 0, capBuf, 0, 0, capLen);
        sd->buffer.copyFrom (1, 0, capBuf, 1, 0, capLen);
        sd->sampleRate = sr;
        const float pk = sd->buffer.getMagnitude (0, capLen);
        if (pk > 1.0e-4f) sd->buffer.applyGain (0.8f / pk);
        sd->name = captureFromInput ? "Captured: input" : "Captured: resample";
        samplePath = juce::File();
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        graveyard.push_back (sample);
        sample = sd;
    }
    graveyard.erase (std::remove_if (graveyard.begin(), graveyard.end(),
                                     [] (const auto& s) { return s.use_count() <= 1; }), graveyard.end());

    // exploration mode: gentle pattern mutation every N bars
    if (getParam (evolveOn) > 0.5f && ! frozen && seqRunning.load())
    {
        static const int bars[] = { 0, 1, 2, 4, 8, 16 };
        const int every = bars[juce::jlimit (0, 5, (int) getParam (evolveBars))];
        const int bar = barCounter.load();
        if (every > 0 && bar != lastBar)
        {
            lastBar = bar;
            if (bar - lastEvolveBar >= every || bar < lastEvolveBar)
            {
                lastEvolveBar = bar;
                Rng r (rng.next());
                mutatePattern (pattern(), genSettings(), r, 0.35f + getParam (evolveAmt) * 0.6f);
                commitPattern();
            }
        }
    }
}

void ZtxProcessor::setParam (int pid, float actual)
{
    setParamNorm (pid, normOf (pid, actual));
}

void ZtxProcessor::setParamNorm (int pid, float norm)
{
    auto* p = params[(size_t) pid];
    p->beginChangeGesture();
    p->setValueNotifyingHost (clamp01 (norm));
    p->endChangeGesture();
}

void ZtxProcessor::resetAllParams()
{
    for (int i = 0; i < NUM_PARAMS; ++i)
        params[(size_t) i]->setValueNotifyingHost (params[(size_t) i]->getDefaultValue());
}

// ---- pattern -------------------------------------------------------------------------
void ZtxProcessor::commitPattern()
{
    {
        const juce::SpinLock::ScopedLockType lock (patLock);
        sharedPattern = variations[(size_t) currentVar];
    }
    patDirty = true;
    ++patternEpoch;
}

void ZtxProcessor::selectVariation (int v)
{
    currentVar = juce::jlimit (0, kVariations - 1, v);
    commitPattern();
}

void ZtxProcessor::copyVariationTo (int dest)
{
    dest = juce::jlimit (0, kVariations - 1, dest);
    variations[(size_t) dest] = variations[(size_t) currentVar];
}

void ZtxProcessor::pushUndo()
{
    undoStack.push_back (pattern());
    if (undoStack.size() > 32) undoStack.erase (undoStack.begin());
}

bool ZtxProcessor::undo()
{
    if (undoStack.empty()) return false;
    pattern() = undoStack.back();
    undoStack.pop_back();
    commitPattern();
    return true;
}

GenSettings ZtxProcessor::genSettings() const
{
    GenSettings g;
    g.density = getParam (seqDensity);
    g.complexity = getParam (seqComplexity);
    g.chaos = getParam (seqChaos);
    g.groove = getParam (seqGroove);
    g.repetition = getParam (seqRepeat);
    g.maxSteps = getParam (seqSteps) > 0.5f ? 32 : 16;
    return g;
}

void ZtxProcessor::generate()
{
    if (frozen) return;
    pushUndo();
    Rng r (rng.next() ^ (uint32_t) juce::Time::getMillisecondCounter());
    generatePattern (pattern(), genSettings(), r);
    commitPattern();
}

void ZtxProcessor::mutate (float strength)
{
    if (frozen) return;
    pushUndo();
    Rng r (rng.next() ^ (uint32_t) juce::Time::getMillisecondCounter());
    mutatePattern (pattern(), genSettings(), r, strength);
    commitPattern();
}

// ---- samples -------------------------------------------------------------------------
bool ZtxProcessor::loadSample (const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (f));
    if (reader == nullptr) return false;
    const int len = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 30.0));
    if (len < 64) return false;
    auto sd = std::make_shared<SampleData>();
    sd->buffer.setSize (2, len);
    reader->read (&sd->buffer, 0, len, 0, true, true);
    if (reader->numChannels == 1) sd->buffer.copyFrom (1, 0, sd->buffer, 0, 0, len);
    sd->sampleRate = reader->sampleRate;
    sd->name = f.getFileName();
    samplePath = f;
    const juce::SpinLock::ScopedLockType lock (sampleLock);
    graveyard.push_back (sample);
    sample = sd;
    return true;
}

void ZtxProcessor::useDefaultSample()
{
    auto sd = makeDefaultSample (48000.0);
    samplePath = juce::File();
    const juce::SpinLock::ScopedLockType lock (sampleLock);
    graveyard.push_back (sample);
    sample = sd;
}

void ZtxProcessor::startCapture (bool fromInput, float seconds)
{
    if (capturing) return;
    capLen = juce::jlimit (1024, capBuf.getNumSamples(), (int) (seconds * sr));
    capPos = 0;
    captureFromInput = fromInput;
    capturing = true;
}

juce::String ZtxProcessor::getSampleName()
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);
    return sample ? sample->name : juce::String ("(none)");
}

std::shared_ptr<const SampleData> ZtxProcessor::getSample()
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);
    return sample;
}

// ---- state ---------------------------------------------------------------------------
void ZtxProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    for (auto id : { kPatternsId, kMetaId })
        if (auto c = state.getChildWithName (id); c.isValid()) state.removeChild (c, nullptr);

    juce::ValueTree pats (kPatternsId);
    pats.setProperty ("current", currentVar, nullptr);
    pats.setProperty ("frozen", frozen, nullptr);
    for (int v = 0; v < kVariations; ++v) pats.appendChild (patternToTree (variations[(size_t) v], "Pattern"), nullptr);
    state.appendChild (pats, nullptr);

    juce::ValueTree meta (kMetaId);
    meta.setProperty ("sample", samplePath.getFullPathName(), nullptr);
    meta.setProperty ("preset", currentPresetName, nullptr);
    meta.setProperty ("version", JucePlugin_VersionString, nullptr);
    state.appendChild (meta, nullptr);

    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ZtxProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr) return;
    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.hasType (apvts.state.getType())) return;

    auto pats = state.getChildWithName (kPatternsId);
    auto meta = state.getChildWithName (kMetaId);
    apvts.replaceState (state);

    if (pats.isValid())
    {
        currentVar = juce::jlimit (0, kVariations - 1, (int) pats.getProperty ("current", 0));
        frozen = (bool) pats.getProperty ("frozen", false);
        for (int v = 0; v < kVariations && v < pats.getNumChildren(); ++v)
            patternFromTree (variations[(size_t) v], pats.getChild (v));
    }
    if (meta.isValid())
    {
        currentPresetName = meta.getProperty ("preset", "Init").toString();
        const juce::File f (meta.getProperty ("sample").toString());
        if (meta.getProperty ("sample").toString().isNotEmpty() && f.existsAsFile()) loadSample (f);
    }
    commitPattern();
}

juce::AudioProcessorEditor* ZtxProcessor::createEditor() { return new ZtxEditor (*this); }
} // namespace ztx

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ztx::ZtxProcessor(); }
