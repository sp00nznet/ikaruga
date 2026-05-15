// =============================================================================
// Ikaruga Static Recompilation - Main Launcher
//
// "I... I'M GOING IN!"
//
// Entry point for the recompiled game. Initializes the runtime, mounts the
// disc image, brings up GX/audio/input via gcrecomp, then jumps into the
// recompiled DOL.
// =============================================================================

#include "ikaruga/runtime.h"
#include "ikaruga/dol.h"
#include "ikaruga/gx/gx.h"
#include "ikaruga/audio/audio.h"
#include "ikaruga/input/input.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef IK_HAS_RECOMPILED
extern void register_recompiled_functions(ikaruga::FuncTable& table);
#endif

using namespace ikaruga;

static const uint32_t WINDOW_WIDTH  = 1280;
static const uint32_t WINDOW_HEIGHT = 720;

// =============================================================================
// DOL Information (extracted from GIKE70 main.dol)
//
// Game ID:        GIKE70 (NTSC-U, Atari)
// Entry point:    0x80003140
// Text sections:  2 (T0: 0x80003100  T1: 0x80005600)
// Data sections:  6 (D0..D5)
// BSS:            0x8016B380 (size 0xBAA44, 746.6 KB)
// Total DOL size: 1.42 MB (1,489,152 bytes)
//
// SDA bases (extracted from __init_registers @ 0x80003278):
//   r1  = 0x80235DC8  (initial stack)
//   r2  = 0x8022ADA0  (SDA2)
//   r13 = 0x80229D20  (SDA)
// =============================================================================

static const uint32_t DOL_ENTRY_POINT = 0x80003140;
static const uint32_t INITIAL_STACK   = 0x80235DC8;
static const uint32_t SDA_BASE        = 0x80229D20;  // r13
static const uint32_t SDA2_BASE       = 0x8022ADA0;  // r2

static bool load_dol_into_memory(const char* dol_path) {
    FILE* fp = fopen(dol_path, "rb");
    if (!fp) {
        fprintf(stderr, "[WARN] Could not open %s for memory initialization\n", dol_path);
        return false;
    }

    ikaruga::DOLFile dol;
    if (!dol.load(fp)) {
        fclose(fp);
        return false;
    }
    fclose(fp);

    for (const auto& sec : dol.sections) {
        printf("[Init] Loading %s%d -> 0x%08X (%u bytes)\n",
               sec.is_text ? "text" : "data", sec.index,
               sec.address, sec.size);
        for (uint32_t i = 0; i < sec.size; i++) {
            g_mem.write8(sec.address + i, sec.data[i]);
        }
    }

    printf("[Init] Clearing BSS: 0x%08X (%u bytes)\n",
           dol.bss_address, dol.bss_size);
    for (uint32_t i = 0; i < dol.bss_size; i++) {
        g_mem.write8(dol.bss_address + i, 0);
    }

    return true;
}

int main(int argc, char* argv[]) {
    printf("=============================================================\n");
    printf(" Ikaruga - Static Recompilation\n");
    printf(" Game ID: GIKE70 (NTSC-U)\n");
    printf(" Entry Point: 0x%08X\n", DOL_ENTRY_POINT);
    printf(" SDA base:  0x%08X   SDA2 base: 0x%08X\n", SDA_BASE, SDA2_BASE);
    printf("=============================================================\n\n");

    // ---- Initialize runtime ----
    if (!runtime_init()) {
        fprintf(stderr, "[FATAL] Runtime initialization failed!\n");
        return 1;
    }

    // ---- Set initial stack + SDA bases (Metrowerks __init_registers convention) ----
    g_ctx.r[1]  = INITIAL_STACK;
    g_ctx.r[2]  = SDA2_BASE;
    g_ctx.r[13] = SDA_BASE;
    printf("[Init] r1  (stack)  = 0x%08X\n", INITIAL_STACK);
    printf("[Init] r2  (SDA2)   = 0x%08X\n", SDA2_BASE);
    printf("[Init] r13 (SDA)    = 0x%08X\n", SDA_BASE);

    // ---- Mount disc image ----
    const char* iso_path = "Ikaruga.iso";
    if (argc > 1) {
        iso_path = argv[1];
    }

    printf("[Init] Mounting disc image: %s\n", iso_path);
    if (!gcrecomp::mount_disc_image(iso_path, &g_mem)) {
        fprintf(stderr, "[WARN] Could not mount disc image: %s\n", iso_path);
        fprintf(stderr, "[WARN] DVD file access will not work.\n");
    }

    // ---- Load DOL sections into emulated memory ----
    load_dol_into_memory("main.dol");

#ifdef IK_HAS_RECOMPILED
    // ---- Register recompiled functions ----
    printf("[Init] Registering recompiled functions...\n");
    register_recompiled_functions(g_func_table);
    printf("[Init] %zu functions in table\n", g_func_table.table.size());
#else
    printf("[Init] No recompiled code linked yet — run gcrecomp_recompiler first.\n");
#endif

    // ---- Initialize graphics ----
    printf("[Init] Initializing GX graphics (D3D11)...\n");
    gx::GXInit();
    if (!gx::GXInitBackend(nullptr, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "[WARN] GX backend initialization failed — running headless\n");
    }

    // ---- Initialize audio ----
    printf("[Init] Initializing audio (XAudio2)...\n");
    if (!audio::audio_init(32000, 2)) {
        fprintf(stderr, "[WARN] Audio initialization failed — running silent\n");
    }

    // ---- Initialize input ----
    printf("[Init] Initializing input...\n");
    input::input_init();

#ifdef IK_HAS_RECOMPILED
    // ---- Catch silent SEH crashes; dump the trace ring so we can see
    //      which recompiled function we were inside when it AV'd. ----
#ifdef _WIN32
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ep) -> LONG {
        auto* r = ep->ExceptionRecord;
        fprintf(stderr,
                "\n[CRASH] code=0x%08lX  at host RIP=0x%p  param0=0x%llX  param1=0x%llX\n",
                r->ExceptionCode, r->ExceptionAddress,
                (unsigned long long)(r->NumberParameters > 0 ? r->ExceptionInformation[0] : 0),
                (unsigned long long)(r->NumberParameters > 1 ? r->ExceptionInformation[1] : 0));
        fflush(stderr);
        gcrecomp::trace_dump_recent(40);
        return EXCEPTION_CONTINUE_SEARCH;
    });
#endif

    // ---- Watchdog: every 3s, dump a histogram of the most-called
    //      recompiled functions in the last ~4k entries. This is how
    //      we identify the loop we're stuck in. ----
    std::atomic<bool> watchdog_stop{false};
    std::thread watchdog([&watchdog_stop]() {
        using namespace std::chrono;
        auto last_pos = gcrecomp::g_trace_ring.pos.load();
        while (!watchdog_stop.load()) {
            std::this_thread::sleep_for(seconds(3));
            if (watchdog_stop.load()) break;
            auto pos = gcrecomp::g_trace_ring.pos.load();
            uint64_t delta = pos - last_pos;
            last_pos = pos;
            printf("\n[Watchdog] +%llu function entries since last tick (total %llu)\n",
                   (unsigned long long)delta, (unsigned long long)pos);
            if (pos > 0) {
                uint32_t last_func = gcrecomp::g_trace_ring.entries[(pos - 1) & (gcrecomp::TraceRing::SIZE - 1)];
                printf("[Watchdog] last function entered: func_%08X\n", last_func);
            }
            if (delta == 0) {
                gcrecomp::trace_dump_recent(8);
                // Live CPU state — note these reads race with the main thread,
                // but for diagnosing a stuck loop we just need an approximate
                // snapshot.
                auto& c = gcrecomp::g_ctx;
                printf("[Watchdog] cpu state (racy): r0=0x%08X r3=0x%08X r4=0x%08X "
                       "r5=0x%08X r6=0x%08X ctr=0x%08X lr=0x%08X\n",
                       c.r[0], c.r[3], c.r[4], c.r[5], c.r[6], c.ctr, c.lr);
            } else {
                gcrecomp::trace_dump_summary();
            }
        }
    });

    gcrecomp::trace_set_enabled(true);

    // ---- Jump into the recompiled DOL ----
    printf("\n[Game] Launching Ikaruga at 0x%08X...\n\n", DOL_ENTRY_POINT);
    fflush(stdout);

    auto entry = g_func_table.lookup(DOL_ENTRY_POINT);
    if (entry) {
        g_ctx.lr = 0;
        entry(&g_ctx, &g_mem);
        printf("\n[Game] Entry point returned.\n");
    } else {
        fprintf(stderr, "[FATAL] Entry point 0x%08X not in function table.\n",
                DOL_ENTRY_POINT);
    }

    watchdog_stop.store(true);
    watchdog.join();
#else
    printf("\n[Game] Build IK_HAS_RECOMPILED to launch the recompiled game.\n");
    printf("[Game] Scaffolding is alive; press any key to exit.\n");
    fflush(stdout);
    getchar();
#endif

    // ---- Cleanup ----
    input::input_shutdown();
    audio::audio_shutdown();
    gx::GXShutdownBackend();
    runtime_shutdown();

    printf("\n[Done] Ikaruga shutdown complete.\n");
    return 0;
}
