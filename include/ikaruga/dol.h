#pragma once
// =============================================================================
// Ikaruga Static Recompilation - DOL File Parser
//
// Thin wrapper that re-exports gcrecomp's DOL parser under ikaruga::.
// All implementation lives in the gcrecomp library (lib/gcrecomp).
// =============================================================================

#include "gcrecomp/dol.h"

namespace ikaruga {

using DOLFile    = gcrecomp::DOLFile;
using DOLSection = gcrecomp::DOLSection;
using DOLHeader  = gcrecomp::DOLHeader;

} // namespace ikaruga
