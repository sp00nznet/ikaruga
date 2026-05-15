#pragma once
// =============================================================================
// Ikaruga Static Recompilation - GX Graphics API
//
// Thin wrapper that re-exports gcrecomp's GX API under the ikaruga:: namespace.
// All GX implementation lives in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/gx/gx.h"

namespace ikaruga {
    namespace gx = gcrecomp::gx;
}
