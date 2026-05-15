#pragma once
// =============================================================================
// Ikaruga Static Recompilation - Audio System
//
// Thin wrapper that re-exports gcrecomp's audio API under the ikaruga:: namespace.
// All audio implementation lives in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/audio/audio.h"

namespace ikaruga {
    namespace audio = gcrecomp::audio;
}
