# ZTX — ORGANIC GLITCH LAB  (instrumento: AU + VST3 + Standalone)
if(NOT DEFINED ZTX_FORMATS)
    if(APPLE)
        set(ZTX_FORMATS AU VST3 Standalone)
    else()
        set(ZTX_FORMATS VST3 Standalone)
    endif()
endif()
option(ZTX_BUILD_TESTS "Build the ZTX offline test runner" OFF)

juce_add_plugin(ZTX
    COMPANY_NAME "ZTX Lab"
    BUNDLE_ID com.ztxlab.organicglitchlab
    PRODUCT_NAME "ZTX Organic Glitch Lab"
    DESCRIPTION "Experimental sound design instrument and generative sequencer"
    PLUGIN_MANUFACTURER_CODE Ztxl
    PLUGIN_CODE Zogl
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "ZTX can capture audio input into its granular buffer."
    COPY_PLUGIN_AFTER_BUILD FALSE
    VERSION 1.0.0
    FORMATS ${ZTX_FORMATS})

file(GLOB_RECURSE ZTX_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h")
target_sources(ZTX PRIVATE ${ZTX_SOURCES})
target_include_directories(ZTX PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_compile_definitions(ZTX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(ZTX
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(ZTX_BUILD_TESTS)
    juce_add_console_app(ZTXTests PRODUCT_NAME "ZTXTests")
    target_sources(ZTXTests PRIVATE Tests/OfflineTests.cpp ${ZTX_SOURCES})
    target_include_directories(ZTXTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_definitions(ZTXTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JucePlugin_VersionString="1.0.0" JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(ZTXTests PRIVATE
        juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
