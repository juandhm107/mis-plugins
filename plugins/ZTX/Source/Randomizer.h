#pragma once
#include "PluginProcessor.h"

namespace ztx
{
enum class RandTarget { Sound, Rhythm, Mod, FX, All };

// Musical randomisation. Honors the LOCK params and the sequencer FREEZE.
void randomize (ZtxProcessor&, RandTarget);
} // namespace ztx
