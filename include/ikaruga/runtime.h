#pragma once
// =============================================================================
// Ikaruga Static Recompilation - Runtime
//
// Game-specific runtime layer built on top of gcrecomp's generic runtime.
// Re-exports PPCContext, Memory, FuncTable from gcrecomp, and adds
// Ikaruga-specific initialization and OS HLE.
// =============================================================================

#include "gcrecomp/runtime.h"

namespace ikaruga {

// Re-export core types from gcrecomp
using PPCContext    = gcrecomp::PPCContext;
using Memory        = gcrecomp::Memory;
using FuncTable     = gcrecomp::FuncTable;
using RecompiledFunc = gcrecomp::RecompiledFunc;
using GameConfig    = gcrecomp::GameConfig;

// ---- Global runtime state ------------------------------------------
// Alias the shared gcrecomp globals. The recompiled code (recomp_common.h)
// emits references to gcrecomp::g_mem / gcrecomp::g_func_table directly, so
// the launcher and the recompiled code MUST share the same Memory /
// FuncTable instance. Defining a separate ikaruga::g_mem here would create
// a second uninitialized Memory and crash on the first MEM_WRITE32.
using gcrecomp::g_ctx;
using gcrecomp::g_mem;
using gcrecomp::g_func_table;

bool runtime_init();
void runtime_shutdown();

// ---- Ikaruga OS HLE ----------------------------------------------
void init_low_memory(Memory* mem);
void register_os_functions();
RecompiledFunc lookup_os_func(const char* name);
void set_game_root(const std::string& path);

// ---- Disc image support (delegates to gcrecomp) ----------------------------
inline bool mount_disc_image(const char* p, Memory* m) { return gcrecomp::mount_disc_image(p, m); }
inline size_t disc_read(uint32_t off, void* dst, size_t len) { return gcrecomp::disc_read(off, dst, len); }
inline bool is_disc_mounted() { return gcrecomp::is_disc_mounted(); }

} // namespace ikaruga
