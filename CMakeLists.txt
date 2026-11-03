cmake_minimum_required(VERSION 3.22)
project(RogerThat VERSION 1.2.1)

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
    set(RT_FORMATS AU VST3 Standalone)
    set(RT_COPY_AFTER_BUILD TRUE)
else()
    set(RT_FORMATS VST3 Standalone)
    set(RT_COPY_AFTER_BUILD FALSE)
endif()

# Fonts embedded into the binary (Inter and JetBrains Mono, both SIL Open Font License — see Assets/LICENSE-*.txt)
juce_add_binary_data(RogerThatAssets
    HEADER_NAME RogerThatAssets.h
    NAMESPACE RogerThatAssets
    SOURCES
        Assets/Inter-Medium.ttf
        Assets/Inter-SemiBold.ttf
        Assets/JetBrainsMono-Medium.ttf)
set_target_properties(RogerThatAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

juce_add_plugin(RogerThat
    COMPANY_NAME "vst34"
    PLUGIN_MANUFACTURER_CODE Vs34
    PLUGIN_CODE Rgth
    FORMATS ${RT_FORMATS}
    PRODUCT_NAME "Roger That"         # bundle / file name ("Roger That.vst3")
    PLUGIN_NAME "Roger That"          # name shown in the DAW
    DESCRIPTION "Radio / telephone voice distortion"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${RT_COPY_AFTER_BUILD}   # macOS: installs into ~/Library/Audio/Plug-Ins; Windows: off (needs admin)
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(RogerThat PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Widgets.cpp)

target_include_directories(RogerThat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(RogerThat PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(RogerThat PRIVATE
    RogerThatAssets
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
