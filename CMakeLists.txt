cmake_minimum_required(VERSION 3.22)

# macOS settings must be set before project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
option(OAP_UNIVERSAL_BINARY "Build macOS universal binaries (arm64 + x86_64)" OFF)
if(OAP_UNIVERSAL_BINARY)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

project(OpenAudioPlugins VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(OAP_BUILD_PLUGINS "Build the plugins" ON)
option(OAP_BUILD_TESTS "Build the DSP test runner" ON)
option(OAP_COPY_PLUGINS "Copy built plugins to the system plug-in folders" ${APPLE})
set(OAP_JUCE_PATH "" CACHE PATH "Use a local JUCE checkout instead of downloading it")

# ---------------------------------------------------------------------------
# JUCE (pinned version, downloaded on first configure)
# ---------------------------------------------------------------------------
if(OAP_JUCE_PATH)
    add_subdirectory("${OAP_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        9.0.5
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

include(cmake/OapJuce.cmake)
include(cmake/OapPlugin.cmake)

add_subdirectory(shared)

if(OAP_BUILD_PLUGINS)
    add_subdirectory(plugins/DriveAmp)
    add_subdirectory(plugins/CleanAmp)
    add_subdirectory(plugins/effectamps)
    add_subdirectory(plugins/pedals)
    add_subdirectory(plugins/Pedalboard)
endif()

if(OAP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
