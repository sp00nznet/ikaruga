#pragma once
// =============================================================================
// Ikaruga Static Recompilation - OS Definitions
//
// Thin wrapper that re-exports gcrecomp's OS definitions under ikaruga::os.
// All definitions live in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/hw/os_defs.h"

namespace ikaruga {
    namespace os = gcrecomp::os;
}
