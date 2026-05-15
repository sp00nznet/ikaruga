cmake_minimum_required(VERSION 3.20)
project(IkarugaRecomp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
endif()

option(IK_BUILD_RUNTIME "Build the game runtime" ON)
option(IK_BUILD_TOOLS "Build auxiliary tools (DOL info)" ON)

# ===========================================================================
# gcrecomp — Generic GameCube recompilation toolkit (junction/submodule)
# Provides: gcrecomp_gx, gcrecomp_audio, gcrecomp_input, gcrecomp_runtime,
#           gcrecomp_tools (DOL parser, PPC disasm), gcrecomp_recompiler (exe).
# Use:  build/lib/gcrecomp/Release/gcrecomp_recompiler.exe main.dol \
#         --output recompiled/ --project Ikaruga --funcs-per-file 200
# ===========================================================================
add_subdirectory(lib/gcrecomp)

# ===========================================================================
# Runtime Library: Ikaruga-specific OS/runtime layer
# Game-specific OS HLE, memory map, function table. Links gcrecomp for
# GX graphics, audio, and input.
# ===========================================================================
if(IK_BUILD_RUNTIME)
    add_library(ik_runtime STATIC
        src/runtime/ppc_context.cpp
        src/runtime/memory.cpp
        src/runtime/func_table.cpp
        src/runtime/os_funcs.cpp
    )
    target_include_directories(ik_runtime PUBLIC include)
    target_link_libraries(ik_runtime PUBLIC gcrecomp_runtime gcrecomp_tools)

    # =======================================================================
    # Recompiled Game Code - Auto-generated PPC->C translation
    # Built once gcrecomp_recompiler has been run against main.dol.
    # =======================================================================
    option(IK_BUILD_RECOMPILED "Build the recompiled game code" ON)
    if(IK_BUILD_RECOMPILED AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/recompiled/recomp_funcs.h")
        file(GLOB RECOMP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/recompiled/recomp_*.cpp")
        add_library(ik_recompiled STATIC ${RECOMP_SOURCES})
        target_include_directories(ik_recompiled PRIVATE
            include
            "${CMAKE_CURRENT_SOURCE_DIR}/recompiled"
        )
        target_link_libraries(ik_recompiled PRIVATE ik_runtime)
        if(MSVC)
            target_compile_options(ik_recompiled PRIVATE /bigobj)
        endif()
    endif()

    # Main launcher executable
    add_executable(ik_launcher
        src/main.cpp
    )
    target_link_libraries(ik_launcher PRIVATE
        ik_runtime
        gcrecomp_gx
        gcrecomp_audio
        gcrecomp_input
    )
    if(TARGET ik_recompiled)
        target_link_libraries(ik_launcher PRIVATE ik_recompiled)
        target_compile_definitions(ik_launcher PRIVATE IK_HAS_RECOMPILED)
    endif()
    target_include_directories(ik_launcher PRIVATE include)
    if(WIN32)
        target_link_libraries(ik_launcher PRIVATE d3d11 d3dcompiler dxgi)
    endif()
endif()

# ===========================================================================
# Auxiliary Tools
# ===========================================================================
if(IK_BUILD_TOOLS)
    add_executable(ik_dol_info
        tools/dol_info/main.cpp
    )
    target_link_libraries(ik_dol_info PRIVATE gcrecomp_tools)
    target_include_directories(ik_dol_info PRIVATE include)
endif()
