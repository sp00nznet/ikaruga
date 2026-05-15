# Ikaruga - Static Recompilation

**A static recompilation of a GameCube game to native PC.**

This project takes *Ikaruga* (GameCube, 2002) and statically recompiles its
PowerPC 750CXe (Gekko) machine code into native x86-64 C++ that runs directly
on Windows 11. No emulator. No dynarec. Just the shmup, running natively.

This project is primarily a **forcing function for [gcrecomp](https://github.com/sp00nznet/gcrecomp)** — the shared GameCube recompilation toolkit. Ikaruga is small, tight, deterministic, and float-heavy: every gap the recompiler has shows up fast here. Whatever Ikaruga needs that gcrecomp can't yet do becomes work upstream.

## Why Ikaruga?

| Property | Value |
|----------|-------|
| DOL size | **1.42 MB** (smallest of any tracked target) |
| Functions (CFG) | ~5,138 |
| Instructions | 264,680 — coverage **99.38%** in disasm |
| Paired Singles | 5,283 (heavy SIMD float work — great PS coverage test) |
| Streaming media | None (no FMVs, sequenced audio) |
| Camera | Fixed vertical scroller — trivial GX pipeline |
| Multiplayer | Local-only, no network code |
| Discs | Single |

Compared to Wind Waker / Luigi's Mansion (large 3D worlds with streaming
assets) and Super Monkey Ball (physics-heavy), Ikaruga is the smallest viable
"recompiled game running end-to-end" target on the GameCube. If the toolkit
can ship Ikaruga, it has cleared a real bar.

## Current Status

The scaffold is alive and the recompiled DOL **builds and links cleanly**.
The launcher boots through every host-side subsystem (D3D11, XAudio2, input,
runtime, mounted disc) and hands control off to the recompiled entry point.

| Stage | State |
|-------|-------|
| Project scaffold + CMake build | done |
| DOL extracted + parsed (99.38% disasm coverage) | done |
| Symbol-free static recompile via `gcrecomp_recompiler` | done — **5,138 functions in 26 files** |
| Recompiled code compiles cleanly under MSVC `/W3 /bigobj` | done (warnings only) |
| Runtime brings up D3D11 / XAudio2 / Input / disc mount | done |
| Function table populated; entry point `0x80003140` resolves | done |
| **Hands control into recompiled `__eabi`** | done — control transferred |
| SDK HLE wired (GX, VI, DVD, PAD, AI/DSP) | **next** — currently traps on first HW reg access |
| First frame rendered | pending |
| Title screen reached | pending |

## DOL Information

| Field | Value |
|-------|-------|
| Game ID | `GIKE70` (NTSC-U, maker code 70 = Atari) |
| DOL size | 1,489,152 bytes (1.42 MB) |
| Entry point | `0x80003140` |
| Text sections | 2 (T0 `0x80003100`, T1 `0x80005600`) |
| Data sections | 6 (D0..D5) |
| BSS | `0x8016B380` – `0x80225DC4` (746.6 KB) |
| Initial stack (r1) | `0x80235DC8` |
| SDA base (r13) | `0x80229D20` |
| SDA2 base (r2) | `0x8022ADA0` |

## What Is Static Recompilation?

Unlike an emulator that interprets or dynamically translates instructions at
runtime, static recompilation translates the *entire game binary* ahead of
time into compilable C++ source. Each PowerPC instruction becomes a C++
expression:

```c
// Original GameCube PowerPC:
//   addi r3, r4, 0x20
//   lwz  r5, 0x10(r6)
//   bl   some_function

// Recompiled native C++:
ctx->r[3] = (int32_t)ctx->r[4] + 0x20;
ctx->r[5] = MEM_READ32(ctx->r[6] + 0x10);
some_function(ctx, mem);
```

The generated code compiles with any modern C++ compiler (MSVC, Clang, GCC)
and runs at full native speed with all compiler optimizations applied.

## Architecture

```
                  +----------------+
                  |    Ikaruga     |
                  |   main.dol     |
                  | (PowerPC 750)  |
                  +-------+--------+
                          |
                  +-------v-----------+
                  | gcrecomp          |
                  |  recompiler       |
                  | (PPC disasm ->    |
                  |  CFG ->           |
                  |  C++ codegen)     |
                  +-------+-----------+
                          |
                  +-------v-----------+
                  | recompiled/       |
                  | recomp_*.cpp      |
                  | (x86-64 native)   |
                  +-------+-----------+
                          |
            +-------------+-------------+
            |             |             |
   +--------v-----+ +----v----+ +------v------+
   | gcrecomp_gx  | | _audio  | | _input      |
   | (D3D11 / TEV |  | XAudio2 | | XInput +    |
   |  HLSL)       | | (ADPCM) | | Keyboard    |
   +--------------+ +---------+ +-------------+
```

## Repository Layout

```
ikaruga/
  CMakeLists.txt        Top-level build
  main.dol              Extracted GIKE70 DOL (gitignored)
  Ikaruga.iso           User-provided ISO (gitignored)
  src/
    main.cpp            Launcher: init runtime + jump to entry point
    runtime/            Ikaruga-specific OS HLE, memory, func table
  include/ikaruga/      Thin re-exports of gcrecomp's headers
  tools/dol_info/       ik_dol_info — quick DOL inspector
  recompiled/           Generated C++ (gitignored — produced by gcrecomp_recompiler)
  lib/gcrecomp/         Junction to ../gcrecomp (the toolkit)
```

## Building

```bash
# 1. Configure
cmake -B build -G "Visual Studio 17 2022" -A x64

# 2. Build the toolkit + dol_info
cmake --build build --config Release --target ik_dol_info

# 3. Inspect the DOL
./build/Release/ik_dol_info.exe main.dol

# 4. Recompile the game (generates recompiled/*.cpp)
./build/lib/gcrecomp/Release/gcrecomp_recompiler.exe main.dol \
    --output recompiled/ --project Ikaruga --funcs-per-file 200

# 5. Re-configure (so CMake picks up recompiled/*.cpp), then build the launcher
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target ik_launcher

# 6. Run (looks for Ikaruga.iso + main.dol in the cwd)
./build/Release/ik_launcher.exe
```

## How This Drives gcrecomp

Every "currently failing" item on Ikaruga maps to a concrete improvement to
the toolkit. Working list (lives here so the next session can pick it up):

- [ ] **SDK HLE for GX boot path** — game traps on first `__GXInit` /
  hardware register access. Needs a generic HLE binding for `__GXInitHardware`
  and friends in `gcrecomp_gx`.
- [ ] **Symbol map heuristics** — without a decomp project, we have no
  function names. Investigate using BLR-terminated runs + prologue scan
  (already partly done in `gcrecomp_recompiler`'s phase 1.6) to name CRT
  init, libc, GX, VI, DVD entry points by signature matching.
- [ ] **Paired-Singles correctness pass** — 5,283 PS instructions; verify
  ps_madds0 / ps_madds1 / ps_sum0 / ps_sum1 code-gen matches the GC's
  `psq_l/psq_st` quantization registers, since Ikaruga leans on these.
- [ ] **DVD HLE FST integration** — game assets live on disc; needs
  `DVDOpen` / `DVDReadPrio` routed to the mounted ISO's FST.
- [ ] **VI present cadence** — without a symbol map, `VIWaitForRetrace` is
  not yet trapped; figure out the address by pattern-matching.

## Standing on the Shoulders of Giants

- [**gcrecomp**](https://github.com/sp00nznet/gcrecomp) — the toolkit this
  project is bootstrapping (own repo, own dev).
- [**N64Recomp**](https://github.com/N64Recomp/N64Recomp) — pioneered the
  static-recompilation approach for N64 games.
- [**Dolphin**](https://github.com/dolphin-emu/dolphin) — gold-standard
  GC/Wii emulator. Its GX shader generation informs our TEV implementation.
- [**decomp-toolkit**](https://github.com/encounter/decomp-toolkit) —
  GameCube binary analysis tooling.

## Legal

This project does not include any copyrighted game assets or code. You must
provide your own legally obtained copy of *Ikaruga* (GIKE70).

---

*"I... I'M GOING IN!"*
