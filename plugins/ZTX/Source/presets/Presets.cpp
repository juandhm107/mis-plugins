#include "Presets.h"
#include <map>

namespace ztx
{
// =====================================================================================
//  BUILDER
// =====================================================================================
static int destIndexOfPid (int pid)
{
    const auto& d = modDestinations();
    for (size_t i = 1; i < d.size(); ++i) if (d[i] == pid) return (int) i;
    jassertfalse;
    return 0;
}

PresetBuilder& PresetBuilder::lfo (int l, float rate, int shape, bool sync, int div)
{
    set (lfoRate (l), rate); set (lfoShape (l), (float) shape);
    set (lfoSync (l), sync ? 1.0f : 0.0f); set (lfoDiv (l), (float) div);
    return *this;
}

PresetBuilder& PresetBuilder::mod (int slot, int src, int destPid, float amt)
{
    set (slotSrc (slot), (float) src); set (slotDst (slot), (float) destIndexOfPid (destPid)); set (slotAmt (slot), amt);
    return *this;
}

PresetBuilder& PresetBuilder::track (int t, int eng, int note, float level, float pan, float gate)
{
    auto& tr = pat().tracks[(size_t) t];
    tr.engine = eng; tr.note = note; tr.level = level; tr.pan = pan; tr.gate = gate;
    return *this;
}

PresetBuilder& PresetBuilder::macro (int t, float d, float c)
{
    pat().tracks[(size_t) t].decayMod = d; pat().tracks[(size_t) t].colourMod = c;
    return *this;
}

PresetBuilder& PresetBuilder::steps (int t, const char* s, int div)
{
    auto& tr = pat().tracks[(size_t) t];
    clearTrack (tr);
    const int len = juce::jlimit (1, kMaxSteps, (int) std::strlen (s));
    tr.length = len; tr.div = div;
    for (int i = 0; i < len; ++i)
    {
        auto& st = tr.steps[(size_t) i];
        switch (s[i])
        {
            case 'x': st.on = true; st.vel = 0.95f; break;
            case 'o': st.on = true; st.vel = 0.55f; break;
            case 'r': st.on = true; st.vel = 0.75f; st.ratchet = 2; break;
            case 'R': st.on = true; st.vel = 0.7f; st.ratchet = 3; break;
            case '?': st.on = true; st.vel = 0.7f; st.prob = 0.5f; break;
            case '+': st.on = true; st.vel = 0.8f; st.pitch = 12; break;
            case '-': st.on = true; st.vel = 0.8f; st.pitch = -12; break;
            case '5': st.on = true; st.vel = 0.75f; st.pitch = 7; break;
            default: break;
        }
    }
    return *this;
}

PresetBuilder& PresetBuilder::euclid (int t, int hits, int len, int rot, int div)
{
    auto& tr = pat().tracks[(size_t) t];
    clearTrack (tr);
    tr.length = len; tr.div = div;
    applyEuclid (tr, hits, rot);
    return *this;
}

PresetBuilder& PresetBuilder::generate (uint32_t seed, float d, float c, float ch, float g, float rep)
{
    set (seqDensity, d); set (seqComplexity, c); set (seqChaos, ch); set (seqGroove, g); set (seqRepeat, rep);
    GenSettings gs; gs.density = d; gs.complexity = c; gs.chaos = ch; gs.groove = g; gs.repetition = rep;
    Rng r (seed);
    generatePattern (pat(), gs, r);
    return *this;
}

PresetBuilder& PresetBuilder::emptyPattern (int bassEngine, int bassNote)
{
    for (auto& t : pat().tracks) { clearTrack (t); t.length = 16; t.div = 6; }
    track (0, EngPerc, 36, 0.85f);
    track (1, bassEngine, bassNote, 0.8f, -0.1f);
    track (2, EngGlitch, 72, 0.6f, 0.3f);
    track (3, bassEngine, bassNote + 24, 0.55f, 0.2f);
    return *this;
}

PresetBuilder& PresetBuilder::lockTrack (int t, bool l) { pat().tracks[(size_t) t].lock = l; return *this; }

// =====================================================================================
//  FACTORY LIBRARY
// =====================================================================================
#define P(fn) [] (PresetBuilder& b) fn

static const std::vector<FactoryPreset>& library()
{
    static const std::vector<FactoryPreset> lib = {
    // ------------------------------------------------------------ RUBBER & ELASTIC
    { "Elastic Boing", "Rubber & Elastic", "Classic stretched-rubber drop with a damped bounce. Play low.", P({
        b.engine (EngRubber).set (rbElastic, 0.85f).set (rbTension, 0.35f).set (rbDamping, 0.3f).set (rbResonance, 0.4f)
         .set (rbImpact, 0.5f).set (rbDecay, 0.6f).set (rbMaterial, 0.1f).set (rbDrive, 0.15f)
         .set (fltCutoff, 9000).on (rvOn).set (rvMix, 0.12f).set (rvSize, 0.3f).set (rvDecay, 1.2f)
         .mod (0, SrcVelocity, rbElastic, 0.3f).emptyPattern (EngRubber, 40); }) },
    { "Wet Rubber Pluck", "Rubber & Elastic", "Tight pluck with a wet comb body.", P({
        b.engine (EngRubber).set (rbElastic, 0.4f).set (rbTension, 0.7f).set (rbDamping, 0.55f).set (rbResonance, 0.6f)
         .set (rbImpact, 0.8f).set (rbDecay, 0.25f).set (rbMaterial, 0.2f).set (rbDrive, 0.3f)
         .on (cbOn).set (cbFreq, 330).set (cbFb, 0.55f).set (cbDamp, 0.5f).set (cbMix, 0.25f)
         .mod (0, SrcModEnv, fltCutoff, 0.35f).set (fltCutoff, 2500).set (fltReso, 0.35f).set (meD, 0.15f)
         .emptyPattern (EngRubber, 48); }) },
    { "Stretched Membrane", "Rubber & Elastic", "Slow elastic sheet with scraping friction. LFO bends tension.", P({
        b.engine (EngRubber).set (rbElastic, 1.0f).set (rbTension, 0.15f).set (rbDamping, 0.2f).set (rbResonance, 0.7f)
         .set (rbImpact, 0.45f).set (rbFriction, 0.15f).set (rbDecay, 1.4f).set (rbMaterial, 0.0f).set (rbDrive, 0.1f)
         .lfo (0, 0.3f, 0).mod (0, SrcLfo1, rbTension, 0.2f)
         .on (dlOn).set (dlDiv, 11).set (dlFb, 0.4f).set (dlMix, 0.22f).set (dlTone, 0.3f)
         .emptyPattern (EngRubber, 36); }) },
    { "Pop Bubble Chain", "Rubber & Elastic", "Snappy organic pops, random elasticity, chopped by the glitch buffer.", P({
        b.engine (EngRubber).set (rbElastic, 0.7f).set (rbTension, 0.8f).set (rbDamping, 0.5f).set (rbResonance, 0.3f)
         .set (rbImpact, 0.9f).set (rbDecay, 0.12f).set (rbMaterial, 0.15f).set (rbDrive, 0.2f)
         .set (rndRate, 6.0f).set (rndSmooth, 0.0f).mod (0, SrcRandom, rbElastic, 0.4f).mod (1, SrcRandom, rbTension, -0.2f)
         .on (gbOn).set (gbDiv, 3).set (gbProb, 0.3f).set (gbReverse, 0.4f)
         .emptyPattern (EngRubber, 60).steps (1, "x.o.?..x.r..o.?.").track (1, EngRubber, 60, 0.7f); }) },
    { "Squeaky Friction Toy", "Rubber & Elastic", "Stick-slip squeaks; a drunk LFO rubs the surface.", P({
        b.engine (EngRubber).set (rbElastic, 0.5f).set (rbTension, 0.6f).set (rbDamping, 0.35f).set (rbResonance, 0.55f)
         .set (rbImpact, 0.3f).set (rbFriction, 0.75f).set (rbDecay, 1.2f).set (rbMaterial, 0.35f).set (rbDrive, 0.25f)
         .lfo (1, 1.5f, 6).mod (0, SrcLfo2, rbFriction, 0.3f).mod (1, SrcLfo2, rbPitch, 0.04f)
         .set (fltMode, 1).set (fltCutoff, 1800).set (fltReso, 0.3f).emptyPattern (EngRubber, 55); }) },
    { "Rubber Metal Hybrid", "Rubber & Elastic", "Elastic core in a metal shell, slowly frequency shifted.", P({
        b.engine (EngRubber).set (rbElastic, 0.45f).set (rbTension, 0.5f).set (rbDamping, 0.2f).set (rbResonance, 0.8f)
         .set (rbImpact, 0.7f).set (rbDecay, 1.8f).set (rbMaterial, 0.9f).set (rbDrive, 0.2f)
         .on (fsOn).set (fsShift, 7).set (fsFb, 0.3f).set (fsMix, 0.3f)
         .on (rvOn).set (rvMix, 0.3f).set (rvDecay, 4.0f).emptyPattern (EngRubber, 48); }) },

    // ------------------------------------------------------------ MICRO PERCUSSION
    { "Tick Colony", "Micro Percussion", "Swarm of ticks and pops around a soft kick.", P({
        b.engine (EngPerc).set (pcPitch, 0).set (pcDecay, 0.3f).set (pcSweep, 0.5f).set (pcBody, 0.85f).set (pcTransient, 0.6f)
         .set (pcNoise, 0.1f).set (pcResonance, 0.2f)
         .set (glDensity, 0.8f).set (glGrain, 1.2f).set (glDecay, 0.08f).set (glTone, 0.7f).set (glRandom, 0.5f)
         .set (rbDecay, 0.08f).set (rbElastic, 0.6f).set (rbTension, 0.9f).set (rbImpact, 0.9f)
         .track (0, EngPerc, 36, 0.9f).track (1, EngGlitch, 74, 0.55f, 0.3f, 0.3f).track (2, EngGlitch, 63, 0.45f, -0.4f, 0.2f)
         .track (3, EngRubber, 79, 0.45f, 0.2f)
         .generate (101, 0.6f, 0.55f, 0.25f, 0.5f, 0.6f).steps (0, "x...x...x...x..?")
         .set (seqSwing, 0.12f).on (rvOn).set (rvMix, 0.1f).set (rvSize, 0.2f); }) },
    { "Wooden Insects", "Micro Percussion", "Hollow wood chirps on an odd-length grid.", P({
        b.engine (EngPerc).set (pcMaterial, 0.4f).set (pcDecay, 0.12f).set (pcResonance, 0.75f).set (pcNoise, 0.1f)
         .set (pcBody, 0.3f).set (pcTransient, 0.8f).set (pcSweep, 0.15f)
         .set (fmAlgo, 7).set (fmRatio2, 3.51f).set (fmRatio3, 5.4f).set (fmRatio4, 8.93f).set (fmDecay, 0.07f).set (fmAmount, 0.45f)
         .track (0, EngPerc, 48, 0.8f).track (1, EngPerc, 67, 0.6f, -0.3f).macro (1, -0.4f, 0.5f)
         .track (2, EngFM, 84, 0.4f, 0.4f).track (3, EngPerc, 74, 0.5f, 0.15f).macro (3, -0.6f, 0.8f)
         .generate (202, 0.55f, 0.7f, 0.3f, 0.6f, 0.5f).set (seqHuman, 0.2f); }) },
    { "Seed Pod Shaker", "Micro Percussion", "Granular rattle shakers with a dry wooden knock.", P({
        b.engine (EngPerc).set (pcRattle, 0.8f).set (pcNoise, 0.5f).set (pcBody, 0.25f).set (pcTone, 0.7f).set (pcDecay, 0.22f)
         .set (pcMaterial, 0.35f).set (pcTransient, 0.4f)
         .set (glDensity, 1.0f).set (glGrain, 0.6f).set (glDecay, 0.15f).set (glTone, 0.9f).set (glProb, 0.5f)
         .track (0, EngPerc, 45, 0.7f).track (1, EngGlitch, 61, 0.5f, 0.35f).track (2, EngPerc, 80, 0.45f, -0.3f).macro (2, -0.5f, 0.6f)
         .track (3, EngGlitch, 70, 0.4f, 0.2f)
         .steps (0, "x..o..x...o.x...").steps (1, "..R...o...r...o.").euclid (2, 9, 16, 1).euclid (3, 5, 13, 0, 7)
         .set (seqSwing, 0.2f); }) },
    { "Micro Click Kit", "Micro Percussion", "FM kick, digital clicks, wooden hats, rubber pops.", P({
        b.engine (EngGlitch).set (fmRatio1, 1).set (fmIdx2, 0.25f).set (fmAmount, 0.3f).set (fmPitchEnv, 36).set (fmPitchDec, 0.03f)
         .set (fmDecay, 0.35f).set (fmModDecay, 0.2f)
         .set (glDensity, 0.6f).set (glGrain, 2.0f).set (glDecay, 0.1f).set (glRandom, 0.3f).set (glTone, 0.6f)
         .set (pcMaterial, 0.5f).set (pcDecay, 0.05f).set (pcBody, 0.1f).set (pcTransient, 0.9f).set (pcNoise, 0.4f).set (pcTone, 0.85f)
         .set (rbDecay, 0.1f).set (rbElastic, 0.8f).set (rbTension, 0.6f)
         .track (0, EngFM, 36, 0.9f).track (1, EngGlitch, 72, 0.5f, 0.3f).track (2, EngPerc, 84, 0.45f, -0.25f).track (3, EngRubber, 67, 0.5f)
         .steps (0, "x.....x...x.....").generate (303, 0.55f, 0.5f, 0.2f, 0.5f, 0.7f).lockTrack (0, false)
         .steps (0, "x.....x...x.....").set (seqSwing, 0.1f); }) },
    { "Clockwork Pulse", "Micro Percussion", "Mechanical ticking with swing and metallic accents.", P({
        b.engine (EngPerc).set (pcMaterial, 0.3f).set (pcDecay, 0.18f).set (pcBody, 0.6f).set (pcSweep, 0.2f)
         .set (fmAlgo, 3).set (fmRatio1, 1).set (fmRatio2, 2.76f).set (fmRatio3, 4.2f).set (fmRatio4, 9.3f).set (fmDecay, 0.12f).set (fmInharm, 0.4f)
         .set (glDensity, 0.4f).set (glDecay, 0.06f).set (glGrain, 3.0f)
         .track (0, EngPerc, 41, 0.8f).track (1, EngFM, 79, 0.4f, 0.4f).track (2, EngGlitch, 60, 0.5f, -0.3f).track (3, EngRubber, 72, 0.4f)
         .euclid (0, 4, 16).euclid (1, 3, 8, 2).euclid (2, 11, 16, 3).euclid (3, 2, 7)
         .set (seqSwing, 0.18f).on (dlOn).set (dlDiv, 12).set (dlMix, 0.12f).set (dlFb, 0.3f); }) },

    // ------------------------------------------------------------ DARK TEXTURES
    { "Ashen Cloud", "Dark Textures", "Slow grey grain cloud. Hold chords low.", P({
        b.engine (EngGran).set (grSize, 220).set (grDensity, 40).set (grSpray, 0.4f).set (grPitchRnd, 0.12f).set (grReverse, 0.3f)
         .set (grSpread, 0.9f).set (grEnv, 0.6f).set (grScan, 0.05f).set (gateMode, 1).set (ampA, 0.8f).set (ampR, 2.5f)
         .set (fltCutoff, 3000).set (fltReso, 0.3f).on (rvOn).set (rvSize, 0.8f).set (rvDecay, 7).set (rvMix, 0.45f)
         .on (sfOn).set (sfBlur, 0.6f).set (sfMix, 0.3f).lfo (0, 0.07f, 0).mod (0, SrcLfo1, grPos, 0.2f)
         .emptyPattern (EngGran, 36); }) },
    { "Rust Fog", "Dark Textures", "Dense corroded grains through a negative comb.", P({
        b.engine (EngGran).set (grSize, 60).set (grDensity, 70).set (grSpray, 0.6f).set (grPitch, -12).set (grSpread, 0.7f)
         .set (gateMode, 1).set (ampA, 0.4f).set (ampR, 1.8f)
         .on (cbOn).set (cbFreq, 110).set (cbFb, -0.8f).set (cbMix, 0.35f).on (satOn).set (satType, 1).set (satDrive, 0.35f)
         .on (rvOn).set (rvMix, 0.35f).set (rvDamp, 0.8f).set (rvDecay, 5).mod (0, SrcGen, fltCutoff, 0.25f).set (fltCutoff, 4000)
         .emptyPattern (EngGran, 36); }) },
    { "Tar Pit Drone", "Dark Textures", "Unstable inharmonic FM drone sinking in a resonant filter.", P({
        b.engine (EngFM).set (fmRatio1, 1).set (fmRatio2, 0.5f).set (fmRatio3, 3.51f).set (fmRatio4, 7.13f)
         .set (fmIdx2, 0.5f).set (fmIdx3, 0.4f).set (fmIdx4, 0.3f).set (fmAmount, 0.5f).set (fmFeedback, 0.35f)
         .set (fmDecay, 6).set (fmModDecay, 2).set (fmInstab, 0.3f).set (fmInharm, 0.4f).set (fmDrive, 0.3f)
         .set (gateMode, 1).set (ampA, 1.2f).set (ampR, 3.0f)
         .set (fltCutoff, 1200).set (fltReso, 0.5f).set (fltDrive, 0.3f).on (rvOn).set (rvMix, 0.35f).set (rvDecay, 6)
         .lfo (2, 0.1f, 6).mod (0, SrcLfo3, fmAmount, 0.3f).mod (1, SrcLfo1, fltCutoff, 0.15f).lfo (0, 0.05f, 1)
         .emptyPattern (EngFM, 24); }) },
    { "Underground Breath", "Dark Textures", "Frozen grains breathing through a vowel filter.", P({
        b.engine (EngGran).set (grSize, 140).set (grDensity, 30).set (grSpray, 0.25f).set (grPitchRnd, 0.3f).on (grFreeze)
         .set (grPos, 0.6f).set (grEnv, 0.45f).set (gateMode, 1).set (ampA, 1.5f).set (ampR, 3.0f)
         .set (fltMode, 5).set (fltReso, 0.4f).lfo (0, 0.11f, 4).mod (0, SrcLfo1, fltCutoff, 0.35f).set (fltCutoff, 400)
         .on (rvOn).set (rvMix, 0.4f).set (rvDecay, 8).emptyPattern (EngGran, 36); }) },
    { "Black Static", "Dark Textures", "Long burst of crushed digital debris.", P({
        b.engine (EngGlitch).set (glDensity, 0.9f).set (glProb, 0.4f).set (glGrain, 1.5f).set (glRandom, 0.8f).set (glTone, 0.3f)
         .set (glBits, 6).set (glDecay, 4.0f).set (glBuffer, 0.5f).set (glRepeat, 0.5f)
         .set (fltCutoff, 2500).on (rvOn).set (rvMix, 0.4f).set (rvDecay, 5).set (stWidth, 1.4f)
         .emptyPattern (EngGlitch, 48); }) },

    // ------------------------------------------------------------ ORGANIC GLITCHES
    { "Broken Servo", "Organic Glitches", "Motor-like glitch figures stuttered on the grid.", P({
        b.engine (EngGlitch).set (glDensity, 0.7f).set (glRepeat, 0.6f).set (glBuffer, 0.5f).set (glDecay, 0.25f).set (glTone, 0.55f)
         .set (glRate, 0.3f).set (glBits, 9)
         .set (pcDecay, 0.2f).set (pcBody, 0.8f).set (pcSweep, 0.6f)
         .track (0, EngPerc, 36, 0.85f).track (1, EngGlitch, 60, 0.6f, -0.3f).track (2, EngGlitch, 64, 0.5f, 0.35f).track (3, EngGlitch, 52, 0.5f)
         .generate (404, 0.55f, 0.6f, 0.35f, 0.5f, 0.55f).steps (0, "x...x.....x.x...")
         .on (gbOn).set (gbDiv, 2).set (gbProb, 0.35f).on (crOn).set (crBits, 10).set (crRate, 2).set (crMix, 0.5f); }) },
    { "Digital Moss", "Organic Glitches", "Soft, dark debris. Each key picks a different micro-event type.", P({
        b.engine (EngGlitch).set (glTone, 0.3f).set (glDensity, 0.7f).set (glRepeat, 0.5f).set (glBuffer, 0.6f).set (glDecay, 0.5f)
         .set (glRandom, 0.35f).set (glTiming, 0.4f)
         .track (0, EngGlitch, 60, 0.7f).track (1, EngGlitch, 61, 0.6f, -0.4f).track (2, EngGlitch, 62, 0.55f, 0.4f).track (3, EngGlitch, 63, 0.5f)
         .generate (505, 0.45f, 0.5f, 0.3f, 0.4f, 0.6f)
         .on (rvOn).set (rvMix, 0.25f).set (rvSize, 0.4f).set (fltCutoff, 6000); }) },
    { "Insect Data", "Organic Glitches", "Chirps through a frequency shifter, controlled by the step lane.", P({
        b.engine (EngGlitch).set (glDensity, 0.85f).set (glGrain, 0.8f).set (glTone, 0.85f).set (glPitch, 12).set (glDecay, 0.15f)
         .set (glRandom, 0.6f).on (fsOn).set (fsShift, 120).set (fsFb, 0.4f).set (fsMix, 0.5f)
         .mod (0, SrcStepLane, fsShift, 0.3f).mod (1, SrcStepLane, glGrain, 0.2f)
         .set (pcDecay, 0.15f).set (pcBody, 0.7f)
         .track (0, EngPerc, 38, 0.8f).track (1, EngGlitch, 72, 0.55f, 0.3f).track (2, EngGlitch, 74, 0.5f, -0.3f).track (3, EngFM, 91, 0.3f)
         .generate (606, 0.6f, 0.65f, 0.3f, 0.6f, 0.5f); }) },
    { "Corrupted Water", "Organic Glitches", "Droplets: rubber plips and glitch splashes in ping-pong.", P({
        b.engine (EngRubber).set (rbElastic, 0.9f).set (rbTension, 0.85f).set (rbDamping, 0.6f).set (rbDecay, 0.09f).set (rbResonance, 0.35f)
         .set (rbImpact, 0.6f).set (glTone, 0.6f).set (glDecay, 0.12f).set (glDensity, 0.75f)
         .track (0, EngRubber, 72, 0.7f).track (1, EngRubber, 79, 0.6f, -0.4f).track (2, EngGlitch, 66, 0.4f, 0.4f).track (3, EngRubber, 84, 0.5f, 0.2f)
         .generate (707, 0.5f, 0.6f, 0.4f, 0.6f, 0.5f)
         .on (dlOn).set (dlPing, 1).set (dlDiv, 11).set (dlFb, 0.55f).set (dlMix, 0.3f).set (dlWarble, 0.3f)
         .on (cbOn).set (cbFreq, 880).set (cbFb, 0.4f).set (cbMix, 0.2f)
         .mod (0, SrcRandom, rbPitch, 0.1f).set (rndRate, 9); }) },
    { "Stutter Organism", "Organic Glitches", "Granular stabs chewed by the glitch buffer.", P({
        b.engine (EngGran).set (grSize, 45).set (grDensity, 60).set (grSpray, 0.5f).set (grPitchRnd, 0.2f).set (grReverse, 0.5f)
         .set (gateMode, 1).set (ampR, 0.08f).set (ampA, 0.002f)
         .track (0, EngPerc, 36, 0.85f).track (1, EngGran, 60, 0.75f, 0.0f, 0.9f).track (2, EngGlitch, 70, 0.5f).track (3, EngGran, 72, 0.5f, 0.4f, 0.4f)
         .generate (808, 0.55f, 0.55f, 0.35f, 0.5f, 0.6f).steps (0, "x...x...x...x...")
         .on (gbOn).set (gbDiv, 3).set (gbProb, 0.45f).set (gbReverse, 0.5f).set (gbPitch, 0)
         .mod (0, SrcLfo4, gbPitch, 0.25f).lfo (3, 0.5f, 5, true, 4); }) },

    // ------------------------------------------------------------ METALLIC OBJECTS
    { "Alien Bell", "Metallic Objects", "Inharmonic fan-FM bell with a long shimmer.", P({
        b.engine (EngFM).set (fmAlgo, 7).set (fmRatio1, 1).set (fmRatio2, 3.51f).set (fmRatio3, 5.43f).set (fmRatio4, 8.93f)
         .set (fmIdx2, 0.45f).set (fmIdx3, 0.35f).set (fmIdx4, 0.3f).set (fmAmount, 0.55f).set (fmDecay, 3).set (fmModDecay, 0.6f)
         .set (fmInharm, 0.5f).on (rvOn).set (rvMix, 0.3f).set (rvDecay, 5).mod (0, SrcVelocity, fmAmount, 0.3f)
         .emptyPattern (EngFM, 60); }) },
    { "Struck Pipe", "Metallic Objects", "Hollow pipe resonance with a short metallic click.", P({
        b.engine (EngPerc).set (pcMaterial, 0.66f).set (pcResonance, 0.9f).set (pcDecay, 1.5f).set (pcBody, 0.2f).set (pcTransient, 0.6f)
         .set (pcNoise, 0.1f).set (pcSweep, 0.05f).on (rvOn).set (rvMix, 0.2f).emptyPattern (EngPerc, 55); }) },
    { "Tin Can Gamelan", "Metallic Objects", "Dual-pair FM with drifting ratios and dotted delay.", P({
        b.engine (EngFM).set (fmAlgo, 3).set (fmRatio1, 1).set (fmRatio2, 2.76f).set (fmRatio3, 1.0f).set (fmRatio4, 5.4f)
         .set (fmIdx2, 0.4f).set (fmIdx3, 0.5f).set (fmIdx4, 0.35f).set (fmAmount, 0.5f).set (fmDecay, 1.2f).set (fmModDecay, 0.4f)
         .set (fmInstab, 0.15f).set (fmInharm, 0.2f).on (dlOn).set (dlDiv, 11).set (dlFb, 0.45f).set (dlMix, 0.25f)
         .emptyPattern (EngFM, 60).track (1, EngFM, 60, 0.6f).track (3, EngFM, 67, 0.5f, 0.3f)
         .euclid (1, 5, 16, 0).euclid (3, 3, 11, 2); }) },
    { "Scrap Gong", "Metallic Objects", "Metal-material rubber with friction and a slow ring mod.", P({
        b.engine (EngRubber).set (rbMaterial, 1.0f).set (rbResonance, 1.0f).set (rbDecay, 4).set (rbTension, 0.3f).set (rbElastic, 0.2f)
         .set (rbFriction, 0.25f).set (rbImpact, 0.8f).set (rbDamping, 0.15f)
         .on (rmOn).set (rmFreq, 33).set (rmMix, 0.25f).on (rvOn).set (rvMix, 0.35f).set (rvDecay, 6)
         .emptyPattern (EngRubber, 36); }) },
    { "Chain Rattle", "Metallic Objects", "Metal plate shells with heavy rattle, tempo stutters.", P({
        b.engine (EngPerc).set (pcMaterial, 1.0f).set (pcRattle, 0.7f).set (pcResonance, 0.6f).set (pcDecay, 0.4f).set (pcBody, 0.1f)
         .set (pcNoise, 0.3f).set (pcTone, 0.8f).on (gbOn).set (gbDiv, 4).set (gbProb, 0.25f)
         .emptyPattern (EngPerc, 48).track (0, EngPerc, 50, 0.7f).track (2, EngPerc, 62, 0.5f)
         .euclid (0, 5, 16, 1).euclid (2, 7, 12, 0, 10); }) },

    // ------------------------------------------------------------ EXPERIMENTAL BASS
    { "Rubber Sub", "Experimental Bass", "Round elastic sub with an envelope-opened filter.", P({
        b.engine (EngRubber).set (rbElastic, 0.35f).set (rbTension, 0.2f).set (rbDamping, 0.7f).set (rbResonance, 0.15f)
         .set (rbImpact, 0.3f).set (rbDecay, 1.5f).set (rbMaterial, 0.0f).set (rbDrive, 0.35f)
         .set (gateMode, 1).set (ampS, 0.8f).set (ampR, 0.15f).set (fltCutoff, 900).set (fltReso, 0.2f)
         .set (meD, 0.25f).mod (0, SrcModEnv, fltCutoff, 0.35f).emptyPattern (EngRubber, 36); }) },
    { "Bouncing Bassline", "Experimental Bass", "A rubbery sound turned into a bassline. Press play.", P({
        b.engine (EngRubber).set (rbElastic, 0.75f).set (rbTension, 0.3f).set (rbDamping, 0.5f).set (rbDecay, 0.45f).set (rbResonance, 0.3f)
         .set (rbDrive, 0.3f).set (rbMaterial, 0.1f).set (pcDecay, 0.35f).set (pcSweep, 0.45f)
         .set (fltCutoff, 2200).set (fltReso, 0.35f).mod (0, SrcLfo1, fltCutoff, 0.2f).lfo (0, 0.5f, 1, true, 2)
         .track (0, EngPerc, 36, 0.85f).track (1, EngRubber, 36, 0.8f, 0, 0.7f).track (2, EngGlitch, 72, 0.4f, 0.3f).track (3, EngRubber, 48, 0.4f, 0.3f)
         .steps (0, "x...x...x...x...").steps (1, "..x..o.x..+.x.o.").generate (909, 0.45f, 0.4f, 0.15f, 0.6f, 0.8f)
         .lockTrack (0).lockTrack (1); }) },
    { "FM Growl Pit", "Experimental Bass", "Feedback FM through an acid 4-pole, tempo LFO on cutoff.", P({
        b.engine (EngFM).set (fmAlgo, 0).set (fmRatio1, 1).set (fmRatio2, 1).set (fmRatio3, 2).set (fmRatio4, 0.5f)
         .set (fmAmount, 0.7f).set (fmFeedback, 0.4f).set (fmDecay, 1.2f).set (fmModDecay, 1.0f).set (fmDrive, 0.5f)
         .set (gateMode, 1).set (ampS, 0.9f).set (fltMode, 4).set (fltCutoff, 600).set (fltReso, 0.6f)
         .lfo (1, 2, 1, true, 6).mod (0, SrcLfo2, fltCutoff, 0.3f).emptyPattern (EngFM, 36); }) },
    { "Wet Knock Bass", "Experimental Bass", "Swept woody knock with tube saturation.", P({
        b.engine (EngPerc).set (pcBody, 0.9f).set (pcSweep, 0.6f).set (pcDecay, 0.6f).set (pcMaterial, 0.2f).set (pcResonance, 0.3f)
         .set (pcTransient, 0.3f).set (pcPitch, -12).on (satOn).set (satType, 1).set (satDrive, 0.4f)
         .emptyPattern (EngPerc, 48); }) },
    { "Folded Elastic", "Experimental Bass", "Rubber bass through a wavefolder; LFO sweeps the folds.", P({
        b.engine (EngRubber).set (rbElastic, 0.5f).set (rbTension, 0.35f).set (rbDamping, 0.6f).set (rbDecay, 0.9f).set (rbResonance, 0.2f)
         .set (gateMode, 1).set (ampS, 0.7f).on (foldOn).set (foldAmt, 0.35f).set (foldBias, 0.2f)
         .lfo (0, 0.25f, 0).mod (0, SrcLfo1, foldAmt, 0.4f).set (fltCutoff, 2000).set (fltReso, 0.25f)
         .emptyPattern (EngRubber, 36); }) },

    // ------------------------------------------------------------ PSYCHEDELIC ATMOSPHERES
    { "Melting Prism", "Psychedelic Atmospheres", "Scattered reversed grains shifting through a barber-pole.", P({
        b.engine (EngGran).set (grSize, 400).set (grDensity, 30).set (grPitchRnd, 0.25f).set (grReverse, 0.5f).set (grSpread, 1.0f)
         .set (grSpray, 0.3f).set (gateMode, 1).set (ampA, 1.0f).set (ampR, 4.0f)
         .on (fsOn).set (fsShift, 2).set (fsFb, 0.5f).set (fsMix, 0.4f).on (rvOn).set (rvDecay, 12).set (rvMix, 0.5f).set (rvMod, 0.7f)
         .on (dlOn).set (dlPing, 1).set (dlMix, 0.3f).set (dlFb, 0.5f).emptyPattern (EngGran, 48); }) },
    { "Barber Pole Swamp", "Psychedelic Atmospheres", "Additive FM drifting in an endless shift loop.", P({
        b.engine (EngFM).set (fmAlgo, 6).set (fmRatio2, 2.0f).set (fmRatio3, 3.01f).set (fmRatio4, 4.98f).set (fmIdx2, 0.5f).set (fmIdx3, 0.35f)
         .set (fmIdx4, 0.25f).set (fmDecay, 4).set (fmInstab, 0.25f).set (gateMode, 1).set (ampA, 0.6f).set (ampR, 3.0f)
         .on (fsOn).set (fsShift, -4).set (fsFb, 0.8f).set (fsMix, 0.6f).on (rvOn).set (rvMix, 0.45f).set (rvDecay, 9)
         .emptyPattern (EngFM, 48); }) },
    { "Spectral Cathedral", "Psychedelic Atmospheres", "Grains smeared by spectral blur into a cathedral.", P({
        b.engine (EngGran).set (grSize, 300).set (grDensity, 45).set (grSpray, 0.2f).on (grFreeze).set (grPos, 0.45f).set (grPitchRnd, 0.1f)
         .set (gateMode, 1).set (ampA, 2.0f).set (ampR, 5.0f).on (sfOn).set (sfBlur, 0.92f).set (sfMix, 0.6f)
         .on (rvOn).set (rvDecay, 15).set (rvMix, 0.5f).set (rvSize, 1.0f).emptyPattern (EngGran, 36); }) },
    { "Lysergic Dew", "Psychedelic Atmospheres", "Soft elastic drops in warbling ping-pong. Evolves on its own.", P({
        b.engine (EngRubber).set (rbElastic, 0.6f).set (rbTension, 0.45f).set (rbDamping, 0.45f).set (rbDecay, 0.8f).set (rbResonance, 0.6f)
         .set (rbMaterial, 0.55f).on (dlOn).set (dlPing, 1).set (dlDiv, 11).set (dlFb, 0.7f).set (dlWarble, 0.5f).set (dlMix, 0.35f)
         .on (rvOn).set (rvMod, 0.8f).set (rvMix, 0.35f).lfo (0, 0.2f, 4).mod (0, SrcLfo1, rbElastic, 0.3f)
         .on (evolveOn).set (evolveAmt, 0.35f).set (evolveRate, 0.25f)
         .emptyPattern (EngRubber, 60).track (1, EngRubber, 67, 0.6f).track (3, EngRubber, 79, 0.5f, 0.3f)
         .euclid (1, 3, 16, 2).euclid (3, 2, 13, 0); }) },
    { "Mycelium Choir", "Psychedelic Atmospheres", "Grains singing through moving vowels. Evolves.", P({
        b.engine (EngGran).set (grSize, 180).set (grDensity, 55).set (grSpray, 0.35f).set (grPitchRnd, 0.05f).set (grSpread, 0.8f)
         .set (gateMode, 1).set (ampA, 0.9f).set (ampR, 3.0f).set (fltMode, 5).set (fltReso, 0.5f).set (fltCutoff, 800)
         .lfo (0, 0.08f, 0).mod (0, SrcLfo1, fltCutoff, 0.5f).mod (1, SrcGen, grPos, 0.15f)
         .on (rvOn).set (rvMix, 0.4f).set (rvDecay, 8).on (evolveOn).set (evolveAmt, 0.3f).emptyPattern (EngGran, 48); }) },

    // ------------------------------------------------------------ GENERATIVE RHYTHMS
    { "Polymeter Engine", "Generative Rhythms", "Lengths 16/13/11/7 drifting against each other. Evolving.", P({
        b.engine (EngPerc).set (pcDecay, 0.25f).set (pcSweep, 0.4f).set (rbDecay, 0.2f).set (rbElastic, 0.7f).set (glDecay, 0.1f)
         .set (fmAlgo, 7).set (fmDecay, 0.15f).set (fmRatio2, 3.51f).set (fmInharm, 0.3f)
         .track (0, EngPerc, 36, 0.85f).track (1, EngRubber, 43, 0.7f, -0.2f).track (2, EngGlitch, 72, 0.5f, 0.3f).track (3, EngFM, 79, 0.4f, 0.1f)
         .euclid (0, 4, 16).euclid (1, 5, 13, 1).euclid (2, 7, 11, 2).euclid (3, 3, 7)
         .set (seqComplexity, 0.8f).on (evolveOn).set (evolveAmt, 0.2f).set (evolveBars, 3); }) },
    { "Euclid Swamp", "Generative Rhythms", "Pure euclidean layers with wet rubber and comb.", P({
        b.engine (EngRubber).set (rbElastic, 0.55f).set (rbDecay, 0.3f).set (rbTension, 0.6f).set (pcDecay, 0.2f).set (pcMaterial, 0.4f)
         .track (0, EngPerc, 38, 0.8f).track (1, EngRubber, 48, 0.7f).track (2, EngPerc, 72, 0.5f).macro (2, -0.5f, 0.6f).track (3, EngRubber, 60, 0.5f, 0.3f)
         .euclid (0, 5, 16).euclid (1, 3, 8, 1, 6).euclid (2, 7, 16, 2).euclid (3, 4, 13)
         .on (cbOn).set (cbFreq, 196).set (cbFb, 0.5f).set (cbMix, 0.2f); }) },
    { "Microhouse Engine", "Generative Rhythms", "Kick, rubber off-beat bass, glitch hats, metal ticks. Swung.", P({
        b.engine (EngRubber).set (pcDecay, 0.32f).set (pcSweep, 0.5f).set (pcBody, 0.9f)
         .set (rbElastic, 0.5f).set (rbTension, 0.3f).set (rbDecay, 0.3f).set (rbDrive, 0.3f).set (rbDamping, 0.6f)
         .set (glDensity, 0.75f).set (glDecay, 0.07f).set (glTone, 0.7f).set (glGrain, 1.5f)
         .set (fmAlgo, 7).set (fmRatio2, 5.4f).set (fmRatio3, 8.93f).set (fmDecay, 0.08f).set (fmAmount, 0.4f)
         .track (0, EngPerc, 36, 0.9f).track (1, EngRubber, 36, 0.75f, 0, 0.6f).track (2, EngGlitch, 74, 0.45f, 0.3f).track (3, EngFM, 84, 0.35f, -0.3f)
         .steps (0, "x...x...x...x...").steps (1, "..x...x...x..+x.").generate (1111, 0.5f, 0.45f, 0.2f, 0.6f, 0.75f)
         .lockTrack (0).lockTrack (1).set (seqSwing, 0.22f).set (seqHuman, 0.15f)
         .on (dlOn).set (dlDiv, 12).set (dlMix, 0.1f); }) },
    { "Chaos Garden", "Generative Rhythms", "High chaos and complexity; mutates every 2 bars.", P({
        b.engine (EngGlitch).set (glDensity, 0.6f).set (glDecay, 0.15f).set (rbDecay, 0.15f).set (rbElastic, 0.8f)
         .set (pcMaterial, 0.6f).set (pcDecay, 0.15f).set (pcResonance, 0.7f)
         .track (0, EngPerc, 40, 0.8f).track (1, EngRubber, 55, 0.6f).track (2, EngGlitch, 62, 0.5f).track (3, EngFM, 70, 0.4f)
         .generate (1212, 0.6f, 0.75f, 0.7f, 0.5f, 0.35f).on (evolveOn).set (evolveBars, 2).set (evolveAmt, 0.25f)
         .on (gbOn).set (gbProb, 0.2f).set (gbDiv, 3); }) },
    { "Broken Clock Choir", "Generative Rhythms", "Triplets against sixteenths with ratchets.", P({
        b.engine (EngFM).set (fmAlgo, 4).set (fmRatio2, 2.76f).set (fmRatio3, 4.2f).set (fmRatio4, 1.41f).set (fmDecay, 0.2f).set (fmAmount, 0.4f)
         .set (pcDecay, 0.15f).set (pcMaterial, 0.3f)
         .track (0, EngPerc, 43, 0.8f).track (1, EngFM, 67, 0.5f, -0.3f).track (2, EngFM, 74, 0.45f, 0.3f).track (3, EngGlitch, 60, 0.45f)
         .steps (0, "x..x..x..x..", 10).steps (1, "x.r.o..x.R..o...").steps (2, "o.x..o.x", 9).euclid (3, 5, 16, 3); }) },
    { "Minimal Tick Loop", "Generative Rhythms", "High repetition, low density minimal-techno skeleton.", P({
        b.engine (EngPerc).set (pcDecay, 0.28f).set (pcSweep, 0.35f).set (glDecay, 0.05f).set (glGrain, 0.8f).set (rbDecay, 0.12f)
         .track (0, EngPerc, 36, 0.9f).track (1, EngGlitch, 76, 0.45f, 0.25f).track (2, EngRubber, 67, 0.4f, -0.25f).track (3, EngPerc, 84, 0.4f).macro (3, -0.7f, 0.9f)
         .generate (1313, 0.35f, 0.25f, 0.1f, 0.5f, 0.95f).steps (0, "x...x...x...x...").set (seqSwing, 0.15f); }) },
    };
    return lib;
}
#undef P

const std::vector<FactoryPreset>& factoryPresets() { return library(); }

juce::StringArray presetCategories()
{
    return { "Rubber & Elastic", "Micro Percussion", "Dark Textures", "Organic Glitches",
             "Metallic Objects", "Experimental Bass", "Psychedelic Atmospheres", "Generative Rhythms" };
}

void applyFactoryPreset (ZtxProcessor& p, int index)
{
    const auto& lib = library();
    if (index < 0 || index >= (int) lib.size()) return;
    p.resetAllParams();
    p.variations[0] = Pattern();
    for (auto& t : p.variations[0].tracks) clearTrack (t);
    p.frozen = false;
    PresetBuilder b (p);
    lib[(size_t) index].build (b);
    // variations B-D: progressively mutated copies so every preset ships 4 related patterns
    for (int v = 1; v < kVariations; ++v)
    {
        p.variations[(size_t) v] = p.variations[0];
        for (auto& t : p.variations[(size_t) v].tracks) t.lock = false;
        Rng r ((uint32_t) (index * 977 + v * 131));
        mutatePattern (p.variations[(size_t) v], p.genSettings(), r, 0.6f * (float) v);
    }
    // loudness trims measured with the offline test runner (target ~ -25 dB RMS)
    static const std::map<juce::String, float> trims {
        { "Ashen Cloud", 8.0f }, { "Spectral Cathedral", 12.0f }, { "Melting Prism", 7.0f }, { "Digital Moss", 6.0f },
        { "Black Static", 7.0f }, { "Alien Bell", 3.0f }, { "Wet Rubber Pluck", 4.0f }, { "Squeaky Friction Toy", 3.0f },
        { "Insect Data", 3.0f }, { "FM Growl Pit", -7.0f }, { "Tar Pit Drone", -5.0f }, { "Scrap Gong", -5.0f },
        { "Chain Rattle", -5.0f }, { "Mycelium Choir", -4.0f }, { "Elastic Boing", 3.0f } };
    if (auto it = trims.find (lib[(size_t) index].name); it != trims.end())
        p.setParam (masterVol, info (masterVol).def + it->second);
    p.selectVariation (0);
    p.currentPresetName = lib[(size_t) index].name;
    p.presetIndex = index;
}

// ---- user presets -------------------------------------------------------------------
juce::File userPresetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
#if JUCE_MAC
                 .getChildFile ("Application Support")
#endif
                 .getChildFile ("ZTX Organic Glitch Lab").getChildFile ("Presets");
    f.createDirectory();
    return f;
}

juce::Array<juce::File> userPresets()
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.ztxpreset");
    files.sort();
    return files;
}

bool saveUserPreset (ZtxProcessor& p, const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    p.currentPresetName = clean;
    juce::MemoryBlock mb;
    p.getStateInformation (mb);
    auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
    if (xml == nullptr) return false;
    return xml->writeTo (userPresetFolder().getChildFile (clean + ".ztxpreset"));
}

bool loadUserPreset (ZtxProcessor& p, const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr) return false;
    juce::MemoryBlock mb;
    juce::AudioProcessor::copyXmlToBinary (*xml, mb);
    p.setStateInformation (mb.getData(), (int) mb.getSize());
    p.currentPresetName = f.getFileNameWithoutExtension();
    p.presetIndex = -1;
    return true;
}
} // namespace ztx
