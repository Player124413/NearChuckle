# libmain.so - Android application entry point (SDL_main lives here).
project(main LANGUAGES CXX)

set(SRC_FILES
    "../FARCRY/Main.cpp"
    "AndroidMain.cpp"
    "DiagLog.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
    ${CMAKE_SOURCE_DIR}/Externals/SDL/include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ANDROID_LAUNCHER
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3 android log)
