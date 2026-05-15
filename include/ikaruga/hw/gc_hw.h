#pragma once
// =============================================================================
// Ikaruga Static Recompilation - Hardware Constants
//
// Thin wrapper that re-exports gcrecomp's hardware constants under ikaruga::hw.
// All definitions live in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/hw/gc_hw.h"

namespace ikaruga {
    namespace hw = gcrecomp::hw;
}
