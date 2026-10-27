cmake_minimum_required(VERSION 3.24)
project(MXND3XBabyVoice VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(signalsmith-linear
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/linear.git
    GIT_TAG 7bb1c8f36794d6980080ce817c91f8da8fe56a66)
FetchContent_MakeAvailable(signalsmith-linear)
FetchContent_Declare(signalsmith-stretch
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
    GIT_TAG 88061fd4eda56a2353c5d65c74aa3f4ccf76f9da)
FetchContent_MakeAvailable(signalsmith-stretch)
set_property(TARGET signalsmith-stretch PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:signalsmith-stretch,INTERFACE_INCLUDE_DIRECTORIES>)
set_property(TARGET signalsmith-linear PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:signalsmith-linear,INTERFACE_INCLUDE_DIRECTORIES>)
add_library(BabyDSP STATIC Source/DSP/PitchProcessor.cpp Source/DSP/FormantProcessor.cpp)
set_target_properties(BabyDSP PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(BabyDSP PUBLIC Source)
target_link_libraries(BabyDSP PUBLIC signalsmith-stretch)
option(BABY_BUILD_PLUGIN "Build JUCE VST3" ON)
include(CTest)
if(BUILD_TESTING)
    add_executable(DSPTests Tests/DSPTests.cpp)
    target_link_libraries(DSPTests PRIVATE BabyDSP)
    add_test(NAME DSPTests COMMAND DSPTests)
endif()
if(BABY_BUILD_PLUGIN)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 29396c22c93392d6738e021b83196283d6e4d850)
    FetchContent_MakeAvailable(JUCE)
    juce_add_binary_data(BabyAssets SOURCES Assets/brand.png)
    juce_add_plugin(MXND3XBabyVoice
        COMPANY_NAME MXND3X COMPANY_WEBSITE "https://github.com/almazzz2311-jpg/MXND3X-BABY-PITCH"
        BUNDLE_ID com.mxnd3x.babyvoice PLUGIN_MANUFACTURER_CODE Mxnd PLUGIN_CODE Mbvy
        FORMATS VST3 PRODUCT_NAME "MXND3X BABY VOICE"
        IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Fx "Pitch Shift")
    target_sources(MXND3XBabyVoice PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/UI/CustomKnob.cpp)
    target_compile_definitions(MXND3XBabyVoice PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(MXND3XBabyVoice PRIVATE BabyDSP BabyAssets juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    if(BUILD_TESTING)
        add_executable(PluginTests Tests/PluginTests.cpp)
        set_target_properties(PluginTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/PluginTests_artefacts/$<CONFIG>")
        target_link_libraries(PluginTests PRIVATE MXND3XBabyVoice)
        target_include_directories(PluginTests PRIVATE Source $<TARGET_PROPERTY:MXND3XBabyVoice,INCLUDE_DIRECTORIES>)
        target_compile_definitions(PluginTests PRIVATE $<FILTER:$<TARGET_PROPERTY:MXND3XBabyVoice,COMPILE_DEFINITIONS>,EXCLUDE,^JUCE_STANDALONE_APPLICATION=>)
        target_include_directories(PluginTests SYSTEM PRIVATE $<TARGET_PROPERTY:signalsmith-stretch,INTERFACE_INCLUDE_DIRECTORIES> $<TARGET_PROPERTY:signalsmith-linear,INTERFACE_INCLUDE_DIRECTORIES>)
        add_test(NAME PluginTests COMMAND PluginTests)
        if(UNIX AND NOT APPLE)
            set(vst_sdk "${juce_SOURCE_DIR}/modules/juce_audio_processors_headless/format_types/VST3_SDK")
            add_executable(VST3Smoke Tests/VST3Smoke.cpp
                "${vst_sdk}/pluginterfaces/base/funknown.cpp"
                "${vst_sdk}/pluginterfaces/base/coreiids.cpp"
                "${vst_sdk}/public.sdk/source/vst/vstinitiids.cpp")
            target_include_directories(VST3Smoke PRIVATE "${vst_sdk}")
            target_compile_definitions(VST3Smoke PRIVATE RELEASE=1)
            target_link_libraries(VST3Smoke PRIVATE ${CMAKE_DL_LIBS})
            add_dependencies(VST3Smoke MXND3XBabyVoice_VST3)
            add_test(NAME VST3Smoke COMMAND VST3Smoke $<TARGET_FILE:MXND3XBabyVoice_VST3>)
        endif()
    endif()
endif()
