# Console test runner for the amp DSP (no GUI, no plugin wrapper).
juce_add_console_app(oap_dsp_tests PRODUCT_NAME "oap_dsp_tests")

target_sources(oap_dsp_tests PRIVATE
    DspTests.cpp
    FxTests.cpp
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
