#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace dembow;

namespace
{
    using Range = juce::NormalisableRange<float>;

    Range timeRange (float maxSec, float centre)
    {
        Range r (0.001f, maxSec);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::String fmtTime (float s, int)
    {
        return s < 1.0f ? juce::String (juce::roundToInt (s * 1000.0f)) + " ms"
                        : juce::String (s, 2) + " s";
    }

    juce::String fmtPct (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }

    juce::String fmtHz (float v, int)
    {
        return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz" : juce::String (v / 1000.0f, 1) + " kHz";
    }

    const juce::StringArray& waveNames()
    {
        static juce::StringArray a { u8 ("Seno"), u8 ("Triángulo"), u8 ("Sierra"), u8 ("Cuadrada"),
                                     u8 ("Pulso"), u8 ("Súper sierra"), u8 ("Órgano") };
        return a;
    }

    double delayBeats (int choice)
    {
        switch (choice)
        {
            case 0: return 1.0;           // 1/4
            case 1: return 0.75;          // 1/8 punteado
            case 2: return 0.5;           // 1/8
            case 3: return 0.25;          // 1/16
            case 4: return 2.0 / 3.0;     // 1/4 tresillo
            case 5: return 1.0 / 3.0;     // 1/8 tresillo
            default: return 0.5;
        }
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout DembowProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using AP = juce::AudioParameterFloat;
    using AC = juce::AudioParameterChoice;
    using AI = juce::AudioParameterInt;
    using Attr = juce::AudioParameterFloatAttributes;

    auto pct = Attr().withStringFromValueFunction (fmtPct);
    auto tim = Attr().withStringFromValueFunction (fmtTime);

    auto addOsc = [&] (const juce::String& n, int defWave, float defLevel)
    {
        const auto pre = "o" + n;
        layout.add (std::make_unique<AC> (juce::ParameterID (pre + "wave", 1), "Osc " + n + " onda", waveNames(), defWave));
        layout.add (std::make_unique<AI> (juce::ParameterID (pre + "oct", 1), "Osc " + n + " octava", -2, 2, 0));
        layout.add (std::make_unique<AI> (juce::ParameterID (pre + "semi", 1), "Osc " + n + " semitono", -12, 12, 0));
        layout.add (std::make_unique<AP> (juce::ParameterID (pre + "fine", 1), "Osc " + n + " fino", Range (-50.0f, 50.0f), 0.0f,
                                          Attr().withLabel ("ct")
                                                .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " ct"; })));
        layout.add (std::make_unique<AP> (juce::ParameterID (pre + "level", 1), "Osc " + n + " nivel", Range (0.0f, 1.0f), defLevel, pct));
    };
    addOsc ("1", Sierra, 0.8f);
    addOsc ("2", Sierra, 0.0f);

    layout.add (std::make_unique<AP> (juce::ParameterID ("pw", 1), "Ancho de pulso", Range (0.05f, 0.95f), 0.5f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("detune", 1), u8 ("Desafinación súper sierra"), Range (0.0f, 1.0f), 0.3f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("fm", 1), "FM", Range (0.0f, 1.0f, 0.0f, 0.5f), 0.0f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("fmenv", 1), "FM por envolvente", Range (0.0f, 1.0f, 0.0f, 0.5f), 0.0f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("sub", 1), "Sub", Range (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("noise", 1), "Ruido", Range (0.0f, 1.0f, 0.0f, 0.5f), 0.0f, pct));

    layout.add (std::make_unique<AC> (juce::ParameterID ("ftype", 1), "Tipo de filtro",
                                      juce::StringArray { "Graves 24", "Graves 12", "Banda", "Agudos" }, LP24));
    {
        Range r (20.0f, 20000.0f); r.setSkewForCentre (1000.0f);
        layout.add (std::make_unique<AP> (juce::ParameterID ("cutoff", 1), "Corte", r, 8000.0f,
                                          Attr().withStringFromValueFunction (fmtHz)));
    }
    layout.add (std::make_unique<AP> (juce::ParameterID ("reso", 1), "Resonancia", Range (0.0f, 1.0f), 0.1f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("fenv", 1), "Envolvente al filtro", Range (-1.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("keytrk", 1), "Seguimiento de teclado", Range (0.0f, 1.0f), 0.5f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("drive", 1), u8 ("Saturación"), Range (0.0f, 1.0f), 0.0f, pct));

    auto addEnv = [&] (const juce::String& p, const juce::String& n, float a, float d, float s, float r)
    {
        layout.add (std::make_unique<AP> (juce::ParameterID (p + "a", 1), n + " ataque", timeRange (10.0f, 0.4f), a, tim));
        layout.add (std::make_unique<AP> (juce::ParameterID (p + "d", 1), n + " decaimiento", timeRange (10.0f, 0.6f), d, tim));
        layout.add (std::make_unique<AP> (juce::ParameterID (p + "s", 1), n + u8 (" sostén"), Range (0.0f, 1.0f), s, pct));
        layout.add (std::make_unique<AP> (juce::ParameterID (p + "r", 1), n + u8 (" liberación"), timeRange (10.0f, 0.6f), r, tim));
    };
    addEnv ("a", "Volumen", 0.005f, 0.3f, 0.8f, 0.25f);
    addEnv ("f", "Filtro", 0.005f, 0.4f, 0.5f, 0.3f);

    layout.add (std::make_unique<AC> (juce::ParameterID ("lfowave", 1), "LFO onda",
                                      juce::StringArray { u8 ("Seno"), u8 ("Triángulo"), u8 ("Cuadrada"), u8 ("Aleatorio") }, 0));
    layout.add (std::make_unique<AC> (juce::ParameterID ("lfodest", 1), "LFO destino",
                                      juce::StringArray { u8 ("Tono (vibrato)"), u8 ("Filtro"), u8 ("Volumen (trémolo)"), u8 ("Ancho de pulso") }, 0));
    {
        Range r (0.05f, 20.0f); r.setSkewForCentre (3.0f);
        layout.add (std::make_unique<AP> (juce::ParameterID ("lforate", 1), "LFO velocidad", r, 5.0f,
                                          Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " Hz"; })));
    }
    layout.add (std::make_unique<AP> (juce::ParameterID ("lfodepth", 1), "LFO cantidad", Range (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("lfofade", 1), "LFO entrada", Range (0.0f, 3.0f, 0.0f, 0.5f), 0.0f,
                                      Attr().withStringFromValueFunction ([] (float v, int) { return v < 0.002f ? juce::String ("0 s") : fmtTime (v, 0); })));

    layout.add (std::make_unique<AC> (juce::ParameterID ("mode", 1), "Modo de voz",
                                      juce::StringArray { u8 ("Poli (acordes)"), u8 ("Mono"), u8 ("Legato") }, Poli));
    layout.add (std::make_unique<AP> (juce::ParameterID ("glide", 1), "Glide", Range (0.0f, 1.0f, 0.0f, 0.5f), 0.0f,
                                      Attr().withStringFromValueFunction ([] (float v, int) { return v < 0.001f ? juce::String ("Apagado") : fmtTime (v, 0); })));
    layout.add (std::make_unique<AP> (juce::ParameterID ("velsens", 1), "Sensibilidad a la fuerza", Range (0.0f, 1.0f), 0.5f, pct));

    layout.add (std::make_unique<AP> (juce::ParameterID ("chorus", 1), "Chorus", Range (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("dlymix", 1), "Eco", Range (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AC> (juce::ParameterID ("dlytime", 1), "Eco tiempo",
                                      juce::StringArray { "1/4", "1/8 punteado", "1/8", "1/16", "1/4 tresillo", "1/8 tresillo" }, 1));
    layout.add (std::make_unique<AP> (juce::ParameterID ("dlyfb", 1), u8 ("Eco repeticiones"), Range (0.0f, 0.9f), 0.3f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("revmix", 1), "Reverb", Range (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("revsize", 1), u8 ("Reverb tamaño"), Range (0.0f, 1.0f), 0.5f, pct));
    layout.add (std::make_unique<AP> (juce::ParameterID ("volume", 1), "Volumen", Range (-30.0f, 6.0f), -4.0f,
                                      Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })));

    return layout;
}

//==============================================================================
DembowProcessor::DembowProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Salida", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DEMBOW", createLayout())
{
    auto bind = [this] (P& p, const char* id) { p.v = apvts.getRawParameterValue (id); jassert (p.v != nullptr); };
    bind (o1wave, "o1wave"); bind (o1oct, "o1oct"); bind (o1semi, "o1semi"); bind (o1fine, "o1fine"); bind (o1level, "o1level");
    bind (o2wave, "o2wave"); bind (o2oct, "o2oct"); bind (o2semi, "o2semi"); bind (o2fine, "o2fine"); bind (o2level, "o2level");
    bind (pw, "pw"); bind (detune, "detune"); bind (fm, "fm"); bind (fmenv, "fmenv"); bind (sub, "sub"); bind (noise, "noise");
    bind (ftype, "ftype"); bind (cutoff, "cutoff"); bind (reso, "reso"); bind (fenv, "fenv"); bind (keytrk, "keytrk"); bind (drive, "drive");
    bind (aa, "aa"); bind (ad, "ad"); bind (as, "as"); bind (ar, "ar");
    bind (fa, "fa"); bind (fd, "fd"); bind (fs, "fs"); bind (fr, "fr");
    bind (lfowave, "lfowave"); bind (lfodest, "lfodest"); bind (lforate, "lforate"); bind (lfodepth, "lfodepth"); bind (lfofade, "lfofade");
    bind (mode, "mode"); bind (glide, "glide"); bind (velsens, "velsens");
    bind (chorusMix, "chorus"); bind (dlymix, "dlymix"); bind (dlytime, "dlytime"); bind (dlyfb, "dlyfb");
    bind (revmix, "revmix"); bind (revsize, "revsize"); bind (vol, "volume");

    setCurrentProgram (0);
}

bool DembowProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void DembowProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateD = sampleRate;
    globals.sampleRate = (float) sampleRate;
    for (auto& v : voices) v.prepare ((float) sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };
    chorus.prepare (spec);
    chorus.setRate (0.7f);
    chorus.setDepth (0.25f);
    chorus.setCentreDelay (8.0f);
    chorus.setFeedback (0.0f);
    reverb.prepare (spec);

    const int maxDelay = (int) (sampleRate * 4.0) + 8;
    delayL.assign ((size_t) maxDelay, 0.0f);
    delayR.assign ((size_t) maxDelay, 0.0f);
    delayWrite = 0;
    dlyLpL = dlyLpR = 0.0f;

    volume.reset (sampleRate, 0.05);
    volume.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (vol.get()));

    work.setSize (2, juce::jmax (1, samplesPerBlock));
    heldNotes.clear();
    heldNotes.reserve (128);
    lastPitch = -1.0f;
}

void DembowProcessor::readSettings()
{
    auto& s = settings;
    s.osc[0].wave = o1wave.i();
    s.osc[0].semis = (float) (o1oct.i() * 12 + o1semi.i()) + o1fine.get() / 100.0f;
    s.osc[0].level = o1level.get();
    s.osc[1].wave = o2wave.i();
    s.osc[1].semis = (float) (o2oct.i() * 12 + o2semi.i()) + o2fine.get() / 100.0f;
    s.osc[1].level = o2level.get();
    s.pw = pw.get(); s.detune = detune.get(); s.fm = fm.get(); s.fmEnv = fmenv.get();
    s.sub = sub.get(); s.noise = noise.get();
    s.filterType = ftype.i(); s.cutoff = cutoff.get(); s.reso = reso.get();
    s.filterEnv = fenv.get(); s.keyTrack = keytrk.get(); s.drive = drive.get();
    s.aA = aa.get(); s.aD = ad.get(); s.aS = as.get(); s.aR = ar.get();
    s.fA = fa.get(); s.fD = fd.get(); s.fS = fs.get(); s.fR = fr.get();
    s.lfoWave = lfowave.i(); s.lfoDest = lfodest.i(); s.lfoRate = lforate.get();
    s.lfoDepth = lfodepth.get(); s.lfoFade = lfofade.get();
    s.velSens = velsens.get();
    s.glide = glide.get();

    const int newMode = mode.i();
    if (newMode != voiceMode)
    {
        allNotesOff();
        voiceMode = newMode;
    }
}

//==============================================================================
float DembowProcessor::lastPitchFor (int note) const
{
    return lastPitch < 0.0f ? (float) note : lastPitch;
}

int DembowProcessor::findVoice (int note)
{
    // 1) la misma nota (re-ataque)  2) una libre  3) la mas vieja ya soltada  4) la mas vieja
    for (int i = 0; i < kNumVoices; ++i)
        if (voices[(size_t) i].active && voices[(size_t) i].note == note)
            return i;
    for (int i = 0; i < kNumVoices; ++i)
        if (! voices[(size_t) i].active)
            return i;

    int best = -1; int64_t bestAge = -1;
    for (int i = 0; i < kNumVoices; ++i)
        if (! voices[(size_t) i].gate && voices[(size_t) i].age > bestAge)
        { best = i; bestAge = voices[(size_t) i].age; }
    if (best >= 0) return best;

    for (int i = 0; i < kNumVoices; ++i)
        if (voices[(size_t) i].age > bestAge)
        { best = i; bestAge = voices[(size_t) i].age; }
    return juce::jmax (0, best);
}

void DembowProcessor::noteOn (int note, float vel)
{
    const bool doGlide = settings.glide > 0.0005f;

    if (voiceMode == Poli)
    {
        auto& v = voices[(size_t) findVoice (note)];
        v.start (note, vel, lastPitchFor (note), doGlide && lastPitch >= 0.0f, true, seedCounter++ * 2654435761u);
        lastPitch = (float) note;
        return;
    }

    // Mono / Legato: una sola voz
    auto& v = voices[0];
    const bool overlapping = ! heldNotes.empty() && v.active && v.gate;
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
    heldNotes.push_back (note);

    if (voiceMode == Legato && overlapping)
    {
        if (doGlide) v.glideTo (note);
        else { v.glideTo (note); v.currentPitch = (float) note; }
    }
    else
    {
        const bool glideNow = doGlide && v.active;
        v.start (note, vel, v.active ? v.currentPitch : lastPitchFor (note), glideNow, true, seedCounter++ * 2654435761u);
    }
    lastPitch = (float) note;
}

void DembowProcessor::noteOff (int note)
{
    if (voiceMode == Poli)
    {
        for (auto& v : voices)
            if (v.active && v.gate && v.note == note)
            {
                if (sustainPedal) { v.sustained = true; v.gate = false; }
                else v.stop();
            }
        return;
    }

    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
    auto& v = voices[0];
    if (! v.active || v.note != note) return;

    if (! heldNotes.empty())
    {
        // vuelve a la nota anterior que sigue pisada
        const int back = heldNotes.back();
        if (settings.glide > 0.0005f) v.glideTo (back);
        else { v.glideTo (back); v.currentPitch = (float) back; }
        if (voiceMode == Mono) v.start (back, v.velocity, v.currentPitch, settings.glide > 0.0005f, true, seedCounter++);
        lastPitch = (float) back;
    }
    else if (sustainPedal) { v.sustained = true; v.gate = false; }
    else v.stop();
}

void DembowProcessor::allNotesOff()
{
    for (auto& v : voices) if (v.active) v.stop();
    heldNotes.clear();
}

void DembowProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
        globals.bendSemis = (float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        if (cc == 1)
            globals.modWheel = (float) m.getControllerValue() / 127.0f;
        else if (cc == 64)
        {
            sustainPedal = m.getControllerValue() >= 64;
            if (! sustainPedal)
                for (auto& v : voices)
                    if (v.sustained) { v.sustained = false; v.stop(); }
        }
        else if (cc == 120 || cc == 123)
            allNotesOff();
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        allNotesOff();
}

void DembowProcessor::renderVoices (float* l, float* r, int start, int num)
{
    if (num <= 0) return;
    for (auto& v : voices)
        if (v.active)
            v.render (l + start, r + start, num, settings, globals);
}

//==============================================================================
void DembowProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // Teclado de la pantalla
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    readSettings();

    // Trabajamos siempre en estereo
    work.setSize (2, numSamples, false, false, true);
    work.clear();
    auto* L = work.getWritePointer (0);
    auto* R = work.getWritePointer (1);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, numSamples, meta.samplePosition);
        renderVoices (L, R, pos, t - pos);
        pos = t;
        handleMidi (meta.getMessage());
    }
    renderVoices (L, R, pos, numSamples - pos);

    // ---- Efectos ----
    juce::dsp::AudioBlock<float> block (work);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    const float chorusAmt = chorusMix.get();
    if (chorusAmt > 0.001f)
    {
        chorus.setMix (chorusAmt * 0.6f);
        chorus.process (ctx);
    }

    // Eco sincronizado al tempo (ping-pong)
    const float dMix = dlymix.get();
    {
        double bpm = 100.0;   // reggaeton tipico si el host no da tempo
        if (auto* ph = getPlayHead())
            if (auto posInfo = ph->getPosition())
                if (auto b = posInfo->getBpm()) bpm = juce::jlimit (40.0, 240.0, *b);

        const float targetDelay = (float) juce::jlimit (0.01, 3.9, delayBeats (dlytime.i()) * 60.0 / bpm);
        const float fb = dlyfb.get();
        const int size = (int) delayL.size();
        const float smooth = 1.0f - std::exp (-1.0f / (0.05f * (float) sampleRateD));
        const float lpc = 0.35f;

        if (dMix > 0.001f && size > 0)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                delaySmoothed += (targetDelay - delaySmoothed) * smooth;
                const float dSamp = delaySmoothed * (float) sampleRateD;
                float rp = (float) delayWrite - dSamp;
                while (rp < 0.0f) rp += (float) size;
                const int i0 = (int) rp;
                const int i1 = (i0 + 1) % size;
                const float frac = rp - (float) i0;
                const float dl = delayL[(size_t) i0] + (delayL[(size_t) i1] - delayL[(size_t) i0]) * frac;
                const float dr = delayR[(size_t) i0] + (delayR[(size_t) i1] - delayR[(size_t) i0]) * frac;

                dlyLpL += (dl - dlyLpL) * lpc;
                dlyLpR += (dr - dlyLpR) * lpc;

                const float inMono = 0.5f * (L[i] + R[i]);
                // ping-pong: la entrada va a la izquierda, las repeticiones cruzan
                delayL[(size_t) delayWrite] = inMono + dlyLpR * fb;
                delayR[(size_t) delayWrite] = dlyLpL * fb;
                delayWrite = (delayWrite + 1) % size;

                L[i] += dl * dMix;
                R[i] += dr * dMix;
            }
        }
        else
        {
            delaySmoothed = targetDelay;
            if (dlyLpL != 0.0f || dlyLpR != 0.0f)
            {
                std::fill (delayL.begin(), delayL.end(), 0.0f);
                std::fill (delayR.begin(), delayR.end(), 0.0f);
                dlyLpL = dlyLpR = 0.0f;
            }
        }
    }

    const float rMix = revmix.get();
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + revsize.get() * 0.65f;
        rp.damping = 0.45f;
        rp.wetLevel = rMix * 0.5f;
        rp.dryLevel = 1.0f;
        rp.width = 1.0f;
        reverb.setParameters (rp);
        if (rMix > 0.001f)
            reverb.process (ctx);
        else
            reverb.reset();
    }

    // Volumen y limitador suave de seguridad
    volume.setTargetValue (juce::Decibels::decibelsToGain (vol.get()));
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = volume.getNextValue();
        L[i] = softClip (L[i] * g);
        R[i] = softClip (R[i] * g);
    }

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, work, 0, 0, numSamples);
        buffer.copyFrom (1, 0, work, 1, 0, numSamples);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.copyFrom (0, 0, work.getReadPointer (0), numSamples, 0.5f);
        buffer.addFrom (0, 0, work, 1, 0, numSamples, 0.5f);
    }
}

//==============================================================================
const juce::String DembowProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) presets().size()))
    {
        const auto& p = presets()[(size_t) index];
        return u8 (categoryName (p.category)).toLowerCase() + " - " + u8 (p.name);
    }
    return {};
}

void DembowProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;

    currentProgram = index;

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : presets()[(size_t) index].values)
    {
        auto* rp = apvts.getParameter (id);
        jassert (rp != nullptr);
        if (rp != nullptr)
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
    }
    ++programChanges;
}

void DembowProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DembowProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
            for (auto* p : getParameters())
                p->setValueNotifyingHost (p->getValue());
            ++programChanges;
        }
}

juce::AudioProcessorEditor* DembowProcessor::createEditor() { return new DembowEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DembowProcessor(); }
