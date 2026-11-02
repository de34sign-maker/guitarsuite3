cmake_minimum_required(VERSION 3.22)
project(GuitarSuiteClone VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)

# JUCE: use a local checkout if present (JUCE/ next to this file), otherwise download it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(GS_FORMATS AU VST3 Standalone)
else()
    set(GS_FORMATS VST3 Standalone)
endif()

juce_add_plugin(GuitarSuiteClone
    COMPANY_NAME "Bondar"
    PLUGIN_MANUFACTURER_CODE Bndr
    PLUGIN_CODE Gsc1
    FORMATS ${GS_FORMATS}
    PRODUCT_NAME "GuitarSuite3"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE     # installs into ~/Library/Audio/Plug-Ins on macOS
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(GuitarSuiteClone PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(GuitarSuiteClone PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(GuitarSuiteClone PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(GuitarSuiteClone PRIVATE
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
