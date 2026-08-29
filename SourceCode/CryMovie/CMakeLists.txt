project(CryMovie)

add_definitions(-D_USRDLL -DCRYMOVIE_EXPORTS)

set(SRC_FILES
    "AnimCameraNode.cpp"
    "AnimNode.cpp"
    "AnimSequence.cpp"
    "AnimSplineTrack.cpp"
    "AnimTrack.cpp"
    "BoolTrack.cpp"
    "CharacterTrack.cpp"
    "ConsoleTrack.cpp"
    "CryMovie.cpp"
    "CVarNode.cpp"
    "EntityNode.cpp"
    "EventTrack.cpp"
    "ExprTrack.cpp"
    "MaterialNode.cpp"
    "Movie.cpp"
    "MusicTrack.cpp"
    "SceneNode.cpp"
    "ScriptVarNode.cpp"
    "SelectTrack.cpp"
    "SequenceIt.cpp"
    "SoundTrack.cpp"
    "StdAfx.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
)

# CryCommon/Linux_Win32Wrapper.h implements timeGetTime() via
# SDL_GetTicks() - every module that includes it must link SDL3
# (on Android undefined symbols resolve ONLY through DT_NEEDED).
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3)
