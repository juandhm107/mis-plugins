if(NOT DEFINED CALOR_FORMATS)
    if(APPLE)
        set(CALOR_FORMATS AU VST3)
    else()
        set(CALOR_FORMATS VST3)
    endif()
endif()

juce_add_plugin(Calor
    COMPANY_NAME "JDHM Audio"
    BUNDLE_ID "com.jdhmaudio.calor"
    PLUGIN_MANUFACTURER_CODE Jdhm
    PLUGIN_CODE Clor
    FORMATS ${CALOR_FORMATS}
    PRODUCT_NAME "Calor"
    DESCRIPTION "Distorsion multietapa"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
    COPY_PLUGIN_AFTER_BUILD FALSE
    VERSION 1.0.0)

target_sources(Calor PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Calor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Calor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
