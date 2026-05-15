#pragma once
// =============================================================================
// Ikaruga Static Recompilation - Input System
//
// Thin wrapper that re-exports gcrecomp's input API under the ikaruga:: namespace.
// All input implementation lives in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/input/input.h"

namespace ikaruga {
    namespace input = gcrecomp::input;
}
