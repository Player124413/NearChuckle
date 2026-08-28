cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
cmake_policy(VERSION 3.12)

project(farcry LANGUAGES C CXX)

if (ANDROID)
    # 64-bit ARM reuses the engine's existing 64-bit Linux configuration
    # (LINUX64 + _AMD64_); all x86-only code is already excluded by LINUX
    # guards, and Linux64Specific.h provides the base typedefs.
    if (ANDROID_ABI STREQUAL "arm64-v8a")
        set(ARCH_TYPE "arm64")
        add_definitions(-DLINUX64 -D_AMD64_)
    else()
        set(ARCH_TYPE "arm32")
        add_definitions(-DLINUX32)
    endif()
elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_TYPE "x64")
else (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH_TYPE "x86")
endif()
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message("ARCH_TYPE=${ARCH_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/${ARCH_TYPE}-${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/${ARCH_TYPE}-${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})

if (MSVC)
    if (${ARCH_TYPE} STREQUAL "x64")
        add_definitions(-DWIN64 -D_AMD64_)
    endif()
    # Always generate PDBs
    add_compile_options(/Zi)
    add_link_options(/DEBUG)

    add_compile_options(/W3) # Warning level
else()
    add_definitions(-DLINUX)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        
    endif()
    if (${ARCH_TYPE} STREQUAL "x64")
        add_definitions(-DLINUX64 -D_AMD64_)
    endif()

    # SDL3 headers from Externals (built from source below)
    include_directories(${CMAKE_SOURCE_DIR}/Externals/SDL/include)
    # OpenAL-soft headers (built from source in Externals)
    include_directories(${CMAKE_SOURCE_DIR}/Externals/openal-soft/include)

    # NOTE: no -fpermissive here on purpose: the codebase is kept strict-clean
    # (required for clang/Android-NDK builds; verified clean with GCC too).
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -D_DEBUG -g")
endif()

add_subdirectory(Externals)
add_subdirectory(SourceCode)
