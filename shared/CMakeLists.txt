# Header-only DSP building blocks (plain C++17, CabinetSection additionally needs juce_dsp).
add_library(oap_shared_dsp INTERFACE)
target_include_directories(oap_shared_dsp INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

# Shared UI + plugin base classes. Compiled into each plugin target because JUCE
# modules are configured per target.
add_library(oap_shared_ui INTERFACE)
target_link_libraries(oap_shared_ui INTERFACE oap_shared_dsp)
target_sources(oap_shared_ui INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/AmpLookAndFeel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/AmpControls.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugin/AmpProcessorBase.cpp")
