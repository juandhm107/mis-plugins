if(NOT DEFINED DEMBOW_FORMATS)
    if(APPLE)
        set(DEMBOW_FORMATS AU VST3)
    else()
        set(DEMBOW_FORMATS VST3)
    endif()
endif()

juce_add_plugin(Dembow
    COMPANY_NAME "JDHM Audio"
    BUNDLE_ID "com.jdhmaudio.dembow"
    PLUGIN_MANUFACTURER_CODE Jdhm
    PLUGIN_CODE Dmbw
    FORMATS ${DEMBOW_FORMATS}
    PRODUCT_NAME "Dembow"
    DESCRIPTION "Sintetizador de reggaeton"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE
    VERSION 1.0.0)

target_sources(Dembow PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Dembow PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Dembow
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
