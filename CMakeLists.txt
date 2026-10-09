cmake_minimum_required(VERSION 3.22)
project(SignatureVocalStrip VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SignatureVocalStrip
    COMPANY_NAME "StudioUser"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    FORMATS VST3
    PRODUCT_NAME "Signature Vocal Strip"
)

target_sources(SignatureVocalStrip PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
)

target_compile_definitions(SignatureVocalStrip PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SignatureVocalStrip PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)
