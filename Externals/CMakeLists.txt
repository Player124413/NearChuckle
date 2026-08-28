
# Engine external dependencies.
# SDL is required. OpenAL-soft / libogg / libvorbis are built from source
# when their folders are present (they are needed by CrySoundSystem on
# non-Windows platforms).

add_subdirectory(SDL)

# The engine modules link a plain "SDL3" target; SDL's own CMake exports
# "SDL3-shared"/"SDL3::SDL3-shared", so bridge the name.
if (NOT TARGET SDL3)
	if (TARGET SDL3-shared)
		add_library(SDL3 ALIAS SDL3-shared)
	elseif (TARGET SDL3::SDL3-static)
		add_library(SDL3 ALIAS SDL3::SDL3-static)
	endif()
endif()

if (NOT MSVC)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/openal-soft/CMakeLists.txt)
		set(LIBTYPE STATIC CACHE STRING "openal-soft library type" FORCE)
		set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
		set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
		set(ALSOFT_CONFIG OFF CACHE BOOL "" FORCE)
		set(ALSOFT_HRTF_DEFS OFF CACHE BOOL "" FORCE)
		set(ALSOFT_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
		set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_OSS OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_ALSA OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_JACK OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_PIPE OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_PULSEAUDIO OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_PORTAUDIO OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_SNDIO OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_SOLARIS OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_SDL2 OFF CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_WAVE ON CACHE BOOL "" FORCE)
		set(ALSOFT_BACKEND_NULL ON CACHE BOOL "" FORCE)
		set(CMAKE_POSITION_INDEPENDENT_CODE ON)
		add_subdirectory(openal-soft EXCLUDE_FROM_ALL)
		# CrySoundSystem links a plain "openal"; openal-soft's target is "OpenAL"
		if (NOT TARGET openal AND TARGET OpenAL)
			add_library(openal ALIAS OpenAL)
		endif()
	else()
		message(WARNING "openal-soft sources not found in Externals - CrySoundSystem will not link")
	endif()

	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ogg/CMakeLists.txt AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vorbis/lib/CMakeLists.txt)
		set(OGG_LIB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ogg CACHE PATH "" FORCE)
		set(OGG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ogg/include CACHE PATH "" FORCE)

		# generate the config headers (normally done by the projects' own cmake)
		set(OGG_CONFIG_DIR ${CMAKE_CURRENT_BINARY_DIR}/ogggen)
		file(MAKE_DIRECTORY ${OGG_CONFIG_DIR}/ogg)
		if (NOT EXISTS ${OGG_CONFIG_DIR}/ogg/config_types.h)
			file(WRITE ${OGG_CONFIG_DIR}/ogg/config_types.h
"#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

#define INCLUDE_INTTYPES_H 1
#define INCLUDE_STDINT_H 1
#define INCLUDE_SYS_TYPES_H 1

#if INCLUDE_INTTYPES_H
#  include <inttypes.h>
#endif
#if INCLUDE_STDINT_H
#  include <stdint.h>
#endif
#if INCLUDE_SYS_TYPES_H
#  include <sys/types.h>
#endif

typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;
typedef uint64_t ogg_uint64_t;

#endif
")
		endif()
		if (NOT EXISTS ${OGG_CONFIG_DIR}/vorbis/config_types.h)
			file(WRITE ${OGG_CONFIG_DIR}/vorbis/config_types.h
"#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

typedef int16_t vorbis_int16_t;
typedef uint16_t vorbis_uint16_t;
typedef int32_t vorbis_int32_t;
typedef uint32_t vorbis_uint32_t;
typedef int64_t vorbis_int64_t;
typedef uint64_t vorbis_uint64_t;

#endif
")
		endif()

		add_library(ogg STATIC
			ogg/src/bitwise.c
			ogg/src/framing.c
		)
		target_include_directories(ogg PUBLIC
			ogg/include
			${OGG_CONFIG_DIR}
		)

		add_library(vorbis STATIC
			vorbis/lib/analysis.c
			vorbis/lib/bitrate.c
			vorbis/lib/block.c
			vorbis/lib/codebook.c
			vorbis/lib/envelope.c
			vorbis/lib/floor0.c
			vorbis/lib/floor1.c
			vorbis/lib/info.c
			vorbis/lib/lookup.c
			vorbis/lib/lpc.c
			vorbis/lib/lsp.c
			vorbis/lib/mapping0.c
			vorbis/lib/mdct.c
			vorbis/lib/psy.c
			vorbis/lib/registry.c
			vorbis/lib/res0.c
			vorbis/lib/sharedbook.c
			vorbis/lib/smallft.c
			vorbis/lib/synthesis.c
			vorbis/lib/vorbisenc.c
			vorbis/lib/window.c
		)
		target_compile_definitions(vorbis PRIVATE
			VORBIS_FPU_NOT_PRESENT=0
		)
		target_include_directories(vorbis PUBLIC
			vorbis/include
			vorbis/lib
			${OGG_CONFIG_DIR}
			ogg/include
		)
		target_link_libraries(vorbis PUBLIC ogg)

		add_library(vorbisfile STATIC
			vorbis/lib/vorbisfile.c
		)
		target_include_directories(vorbisfile PUBLIC
			vorbis/include
			vorbis/lib
		)
		target_link_libraries(vorbisfile PUBLIC vorbis)
	else()
		message(WARNING "libogg/libvorbis sources not found in Externals - CrySoundSystem will not link")
	endif()
endif()
