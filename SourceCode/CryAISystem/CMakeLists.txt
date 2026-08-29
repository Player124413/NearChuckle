project(CryAISystem)

add_definitions(-D_USRDLL -DCRYAISYSTEM_EXPORTS)

set(SRC_FILES
    "AIAttribute.cpp"
    "AIAutoBalance.cpp"
    "AIMemStats.cpp"
    "AIObject.cpp"
    "AIPlayer.cpp"
    "AIVehicle.cpp"
    "BuildingIDManager.cpp"
    "CAISystem.cpp"
    "CryAISystem.cpp"
    "CTriangulator.cpp"
    "Formation.cpp"
    "GoalOp.cpp"
    "GoalPipe.cpp"
    "Graph.cpp"
    "GraphUtility.cpp"
    "Heuristic.cpp"
    "IAgent.cpp"
    "PipeUser.cpp"
    "Puppet.cpp"
    "StdAfx.cpp"
    "VertexList.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
)

# CryCommon/Linux_Win32Wrapper.h implements timeGetTime() via
# SDL_GetTicks() - every module that includes it must link SDL3
# (on Android undefined symbols resolve ONLY through DT_NEEDED).
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3)
