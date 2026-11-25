// Offline test runner: renders every engine and every factory preset headless, checks for
// NaN / silence / clipping, measures CPU (realtime factor), verifies state round-trip and
// the generator, and writes WAV demos.   Usage: ZTXTests [outputDir]
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include "PluginProcessor.h"
#include "presets/Presets.h"
#include "Randomizer.h"
#include "PluginEditor.h"

using namespace ztx;

namespace
{
int failures = 0;
void check (bool ok, const juce::String& what)
{
    if (! ok) { ++failures; std::cout << "  [FAIL] " << what << "\n"; }
}

struct Stats { float peak = 0, rms = 0; bool finite = true; double rtf = 0; };

Stats render (ZtxProcessor& p, double seconds, juce::AudioBuffer<float>* capture,
              std::vector<std::pair<double, juce::MidiMessage>> midiEvents = {})
{
    const double sr = 48000.0;
    const int block = 256;
    const int total = (int) (seconds * sr);
    juce::AudioBuffer<float> buf (2, block);
    if (capture) capture->setSize (2, total);
    Stats s;
    double sum = 0;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        juce::MidiBuffer midi;
        for (auto& [t, m] : midiEvents)
        {
            const int sp = (int) (t * sr);
            if (sp >= pos && sp < pos + n) midi.addEvent (m, sp - pos);
        }
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
        {
            const float* d = buf.getReadPointer (c);
            for (int i = 0; i < n; ++i)
            {
                if (! std::isfinite (d[i])) s.finite = false;
                s.peak = std::max (s.peak, std::abs (d[i]));
                sum += (double) d[i] * d[i];
            }
            if (capture) capture->copyFrom (c, pos, buf, c, 0, n);
        }
    }
    const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
    s.rms = (float) std::sqrt (sum / (2.0 * total));
    s.rtf = (seconds * 1000.0) / std::max (0.001, ms);
    return s;
}

void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    if (os == nullptr) return;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), 48000.0, 2, 24, {}, 0));
    if (w == nullptr) return;
    os.release();
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

juce::String fmt (float v) { return juce::String (juce::Decibels::gainToDecibels (v, -120.0f), 1) + " dB"; }
} // namespace


static void findButtons (juce::Component* c, std::map<juce::String, juce::TextButton*>& out)
{
    for (auto* ch : c->getChildren())
    {
        if (auto* b = dynamic_cast<juce::TextButton*> (ch)) out[b->getButtonText()] = b;
        findButtons (ch, out);
    }
}

static int snapshots (const juce::File& dir)
{
    ZtxProcessor p; p.prepareToPlay (48000, 256); p.testBpm = 120;
    applyFactoryPreset (p, 38);
    p.setParam (seqRun, 1);
    juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer m;
    for (int i = 0; i < 200; ++i) { b.clear(); p.processBlock (b, m); }
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    ed->setSize (ZtxEditor::kBaseW, ZtxEditor::kBaseH);
    std::map<juce::String, juce::TextButton*> btns;
    findButtons (ed.get(), btns);
    for (auto name : { "LAB", "SEQUENCER", "MODULATION", "DESTRUCTION", "PRESETS" })
    {
        if (btns.count (name)) btns[name]->onClick();
        for (int i = 0; i < 12; ++i) { b.clear(); p.processBlock (b, m); dynamic_cast<juce::Timer*> (ed.get())->timerCallback(); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto f = dir.getChildFile (juce::String ("ui_") + name + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        png.writeImageToStream (img, os);
    }
    // also: granular + FM engine views
    for (int e : { (int) EngGran, (int) EngFM })
    {
        p.setParam (labEngine, (float) e);
        if (btns.count ("LAB")) btns["LAB"]->onClick();
        for (int i = 0; i < 12; ++i) { b.clear(); p.processBlock (b, m); dynamic_cast<juce::Timer*> (ed.get())->timerCallback(); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto f = dir.getChildFile ("ui_LAB_engine" + juce::String (e) + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        png.writeImageToStream (img, os);
    }
    std::cout << "snapshots written to " << dir.getFullPathName() << "\n";
    return 0;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc > 2 && juce::String (argv[1]) == "--snapshots") return snapshots (juce::File (argv[2]));
    juce::File outDir = argc > 1 ? juce::File (argv[1]) : juce::File::getCurrentWorkingDirectory().getChildFile ("test-output");
    outDir.createDirectory();

    std::cout << "ZTX offline tests — " << NUM_PARAMS << " parameters, " << modDestinations().size() - 1
              << " mod destinations, " << factoryPresets().size() << " factory presets\n";

    // ---------------------------------------------------------------- engines
    std::cout << "\n== Engines (MIDI note, 2 s) ==\n";
    const char* names[] = { "Rubber", "Glitch", "Granular", "FM", "Perc" };
    for (int e = 0; e < NUM_ENGINES; ++e)
    {
        ZtxProcessor p; p.prepareToPlay (48000, 256); p.testBpm = 120;
        p.setParam (labEngine, (float) e);
        for (auto& t : p.pattern().tracks) clearTrack (t);
        p.commitPattern();
        if (e == EngGran) p.setParam (gateMode, 1);
        juce::AudioBuffer<float> cap;
        auto s = render (p, 2.0, &cap, { { 0.0, juce::MidiMessage::noteOn (1, 48, (juce::uint8) 110) },
                                         { 0.6, juce::MidiMessage::noteOff (1, 48) },
                                         { 0.8, juce::MidiMessage::noteOn (1, 60, (juce::uint8) 90) },
                                         { 1.2, juce::MidiMessage::noteOff (1, 60) } });
        std::cout << "  " << juce::String (names[e]).paddedRight (' ', 10) << " peak " << fmt (s.peak) << "  rms " << fmt (s.rms)
                  << "  x" << juce::String (s.rtf, 0) << " realtime\n";
        check (s.finite, juce::String (names[e]) + " finite");
        check (s.peak > 0.01f, juce::String (names[e]) + " audible");
        check (s.peak < 1.99f, juce::String (names[e]) + " not hard-clipping");
        writeWav (outDir.getChildFile (juce::String ("engine_") + names[e] + ".wav"), cap);
    }

    // ---------------------------------------------------------------- presets
    std::cout << "\n== Factory presets (sequencer running + MIDI note, 4 s @120 BPM) ==\n";
    auto demoDir = outDir.getChildFile ("preset-demos"); demoDir.createDirectory();
    double worstRtf = 1e9;
    const auto& lib = factoryPresets();
    for (int i = 0; i < (int) lib.size(); ++i)
    {
        ZtxProcessor p; p.prepareToPlay (48000, 256); p.testBpm = 120;
        applyFactoryPreset (p, i);
        p.setParam (seqRun, 1.0f);
        juce::AudioBuffer<float> cap;
        auto s = render (p, 4.0, &cap, { { 0.0, juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100) },
                                         { 1.5, juce::MidiMessage::noteOff (1, 48) },
                                         { 2.0, juce::MidiMessage::noteOn (1, 55, (juce::uint8) 100) },
                                         { 3.0, juce::MidiMessage::noteOff (1, 55) } });
        worstRtf = std::min (worstRtf, s.rtf);
        std::cout << "  " << juce::String (i + 1).paddedLeft (' ', 2) << " " << juce::String (lib[(size_t) i].name).paddedRight (' ', 22)
                  << juce::String (lib[(size_t) i].category).paddedRight (' ', 25)
                  << " peak " << fmt (s.peak).paddedRight (' ', 10) << " rms " << fmt (s.rms).paddedRight (' ', 10)
                  << " x" << juce::String (s.rtf, 0) << "\n";
        check (s.finite, juce::String (lib[(size_t) i].name) + " finite");
        check (s.rms > 0.002f, juce::String (lib[(size_t) i].name) + " audible");
        check (s.peak < 0.95f, juce::String (lib[(size_t) i].name) + " under limiter ceiling");
        juce::String fname = juce::String (i + 1).paddedLeft ('0', 2) + "_" + juce::File::createLegalFileName (lib[(size_t) i].name).replaceCharacter (' ', '_') + ".wav";
        writeWav (demoDir.getChildFile (fname), cap);
    }
    std::cout << "  worst-case realtime factor: x" << juce::String (worstRtf, 1) << "\n";
    check (worstRtf > 4.0, "presets render well above realtime");

    // ---------------------------------------------------------------- all FX on (stress)
    {
        std::cout << "\n== Stress: every effect on, 24 voices ==\n";
        ZtxProcessor p; p.prepareToPlay (48000, 256); p.testBpm = 140;
        for (int pid : { fltOn, foldOn, satOn, crOn, fsOn, rmOn, cbOn, dlOn, rvOn, sfOn, gbOn }) p.setParam (pid, 1);
        p.setParam (fltMode, 4); p.setParam (dlFb, 1.1f); p.setParam (cbFb, 0.98f); p.setParam (fsFb, 0.95f);
        p.setParam (seqRun, 1);
        Rng r (5);
        GenSettings g; g.density = 1; g.complexity = 1; g.chaos = 1; generatePattern (p.pattern(), g, r);
        p.commitPattern();
        std::vector<std::pair<double, juce::MidiMessage>> ev;
        for (int k = 0; k < 24; ++k) ev.push_back ({ 0.01 * k, juce::MidiMessage::noteOn (1, 36 + k * 2, (juce::uint8) 127) });
        for (int e = 0; e < NUM_ENGINES; ++e) { p.setParam (labEngine, (float) e); }
        auto s = render (p, 4.0, nullptr, ev);
        std::cout << "  peak " << fmt (s.peak) << "  rms " << fmt (s.rms) << "  x" << juce::String (s.rtf, 1) << " realtime\n";
        check (s.finite, "stress finite");
        check (s.rtf > 1.5, "stress still realtime");
    }

    // ---------------------------------------------------------------- randomize
    {
        std::cout << "\n== Randomize x30 (each render 1 s) ==\n";
        ZtxProcessor p; p.prepareToPlay (48000, 256); p.testBpm = 120; p.setParam (seqRun, 1);
        int bad = 0; float minRms = 1;
        for (int k = 0; k < 30; ++k)
        {
            randomize (p, RandTarget::All);
            p.setParam (labEngine, (float) (k % 5));
            auto s = render (p, 1.0, nullptr, { { 0.0, juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100) }, { 0.5, juce::MidiMessage::noteOff (1, 48) } });
            if (! s.finite || s.peak >= 1.99f) ++bad;
            minRms = std::min (minRms, s.rms);
        }
        std::cout << "  unstable results: " << bad << "/30, quietest rms " << fmt (minRms) << "\n";
        check (bad == 0, "randomize stays stable");
        // locks
        p.setParam (lockSound, 1); const float before = p.getParam (rbTension);
        randomize (p, RandTarget::All);
        check (p.getParam (rbTension) == before, "LOCK sound respected");
    }

    // ---------------------------------------------------------------- generator
    {
        std::cout << "\n== Generator ==\n";
        Rng r (77);
        GenSettings lo; lo.density = 0.1f; lo.chaos = 0; lo.complexity = 0;
        GenSettings hi; hi.density = 0.9f; hi.chaos = 0; hi.complexity = 0;
        auto hits = [] (const Pattern& pt) { int c = 0; for (auto& t : pt.tracks) for (int i = 0; i < t.length; ++i) c += t.steps[(size_t) i].on; return c; };
        int sLo = 0, sHi = 0;
        for (int k = 0; k < 50; ++k) { Pattern a, b; generatePattern (a, lo, r); generatePattern (b, hi, r); sLo += hits (a); sHi += hits (b); }
        std::cout << "  avg hits density 0.1: " << sLo / 50 << "   density 0.9: " << sHi / 50 << "\n";
        check (sHi > sLo * 2, "density control works");

        GenSettings mid; Pattern a; generatePattern (a, mid, r);
        Pattern b = a; mutatePattern (b, mid, r, 1.0f);
        int diff = 0, tot = 0;
        for (int t = 0; t < kTracks; ++t)
            for (int i = 0; i < a.tracks[(size_t) t].length; ++i) { ++tot; diff += a.tracks[(size_t) t].steps[(size_t) i].on != b.tracks[(size_t) t].steps[(size_t) i].on; }
        std::cout << "  mutate changed " << diff << " of " << tot << " gates (identity kept)\n";
        check (diff > 0 && diff < tot / 4, "mutate is gentle");
        a.tracks[1].lock = true; Pattern c = a; generatePattern (c, mid, r);
        bool same = true; for (int i = 0; i < kMaxSteps; ++i) same &= a.tracks[1].steps[(size_t) i].on == c.tracks[1].steps[(size_t) i].on;
        check (same, "track LOCK respected by GENERATE");
        auto e = euclid (5, 16, 0); int eh = 0; for (bool x : e) eh += x;
        check (eh == 5 && e[0], "euclid(5,16)");
    }

    // ---------------------------------------------------------------- state
    {
        std::cout << "\n== State round-trip ==\n";
        ZtxProcessor a; a.prepareToPlay (48000, 256);
        applyFactoryPreset (a, 7);
        a.setParam (fmRatio3, 7.77f);
        a.pattern().tracks[2].steps[5].on = true; a.pattern().tracks[2].steps[5].ratchet = 3;
        juce::MemoryBlock mb; a.getStateInformation (mb);
        ZtxProcessor b; b.prepareToPlay (48000, 256); b.setStateInformation (mb.getData(), (int) mb.getSize());
        int mism = 0;
        for (int i = 0; i < NUM_PARAMS; ++i) if (std::abs (a.getParam (i) - b.getParam (i)) > 1e-3f) ++mism;
        check (mism == 0, "all params restored (" + juce::String (mism) + " mismatches)");
        check (b.pattern().tracks[2].steps[5].ratchet == 3 && b.pattern().tracks[2].steps[5].on, "pattern restored");
        check (b.currentPresetName == a.currentPresetName, "preset name restored");
        std::cout << "  state size " << (int) mb.getSize() << " bytes\n";
    }

    // ---------------------------------------------------------------- spectral unity
    {
        ZtxProcessor p; p.prepareToPlay (48000, 256); p.testBpm = 120;
        for (auto& t : p.pattern().tracks) clearTrack (t);
        p.commitPattern();
        p.setParam (labEngine, EngGran); p.setParam (gateMode, 1);
        auto dry = render (p, 1.5, nullptr, { { 0.0, juce::MidiMessage::noteOn (1, 60, (juce::uint8) 127) } });
        ZtxProcessor q; q.prepareToPlay (48000, 256); q.testBpm = 120;
        for (auto& t : q.pattern().tracks) clearTrack (t);
        q.commitPattern();
        q.setParam (labEngine, EngGran); q.setParam (gateMode, 1);
        q.setParam (sfOn, 1); q.setParam (sfMix, 1); q.setParam (sfBlur, 0);
        auto wet = render (q, 1.5, nullptr, { { 0.0, juce::MidiMessage::noteOn (1, 60, (juce::uint8) 127) } });
        const float ratio = wet.rms / std::max (1e-6f, dry.rms);
        std::cout << "\n== Spectral path gain (should be ~0 dB): " << juce::String (juce::Decibels::gainToDecibels (ratio), 2) << " dB\n";
        check (ratio > 0.6f && ratio < 1.5f, "spectral resynthesis near unity");
    }

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : juce::String (failures) + " FAILURE(S)") << "\n";
    std::cout << "WAV output: " << outDir.getFullPathName() << "\n";
    return failures == 0 ? 0 : 1;
}
