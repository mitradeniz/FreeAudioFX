# Console test runner for the DSP (no GUI, no plugin wrapper).
juce_add_console_app(oap_dsp_tests PRODUCT_NAME "oap_dsp_tests")

target_sources(oap_dsp_tests PRIVATE
    DspTests.cpp
    FxTests.cpp
    EffectAmpTests.cpp
    LooperTests.cpp
    "${CMAKE_SOURCE_DIR}/shared/amps/AmpModels.cpp"
    "${CMAKE_SOURCE_DIR}/shared/amps/EffectAmpEngine.cpp"
    "${CMAKE_SOURCE_DIR}/plugins/DriveAmp/Source/DriveAmpEngine.cpp"
    "${CMAKE_SOURCE_DIR}/plugins/CleanAmp/Source/CleanAmpEngine.cpp")

target_include_directories(oap_dsp_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/plugins/DriveAmp/Source"
    "${CMAKE_SOURCE_DIR}/plugins/CleanAmp/Source")

target_link_libraries(oap_dsp_tests PRIVATE
    oap_shared_dsp
    oap_fx
    oap_juce
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

set_target_properties(oap_dsp_tests PROPERTIES FOLDER "tests")

add_test(NAME oap_dsp_tests COMMAND oap_dsp_tests)

# Console test runner for the plugin layer (state, files, host tempo, mapped
# switches, editor): the plugins as a host sees them, without a plugin wrapper.
juce_add_console_app(oap_plugin_tests PRODUCT_NAME "oap_plugin_tests")
target_sources(oap_plugin_tests PRIVATE PluginTests.cpp)
target_link_libraries(oap_plugin_tests PRIVATE
    oap_shared_ui
    oap_juce
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
set_target_properties(oap_plugin_tests PROPERTIES FOLDER "tests")

add_test(NAME oap_plugin_tests COMMAND oap_plugin_tests)

# Renders plugin editors to PNG for the documentation (not run as a test).
juce_add_console_app(oap_snapshots PRODUCT_NAME "oap_snapshots")
target_sources(oap_snapshots PRIVATE "${CMAKE_SOURCE_DIR}/tools/Snapshots.cpp")
target_link_libraries(oap_snapshots PRIVATE
    oap_shared_ui
    oap_juce
    juce::juce_recommended_config_flags)
set_target_properties(oap_snapshots PROPERTIES FOLDER "tools")
