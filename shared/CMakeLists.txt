# Header-only amp DSP building blocks (plain C++17; CabinetSection also needs juce_dsp).
add_library(oap_shared_dsp INTERFACE)
target_include_directories(oap_shared_dsp INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(oap_shared_dsp INTERFACE cxx_std_17)

# Helper for our own static libraries: same flags and symbol visibility as the
# plugin code that links them.
function(oap_configure_static_library target)
    set_target_properties(${target} PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE
        VISIBILITY_INLINES_HIDDEN TRUE
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        FOLDER "shared")
    target_link_libraries(${target} PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endfunction()

# Effects library (plain C++17, no JUCE): every pedal's DSP plus the registry
# used by the pedal plugins, the Pedalboard and the effect amps.
add_library(oap_fx STATIC
    fx/Effect.cpp
    fx/Registry.cpp
    fx/DriveEffects.cpp
    fx/ModulationEffects.cpp
    fx/TimeEffects.cpp
    fx/FilterPitchEffects.cpp)
target_link_libraries(oap_fx PUBLIC oap_shared_dsp)
oap_configure_static_library(oap_fx)

# Shared plugin base classes and UI components.
set(oap_ui_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/AmpLookAndFeel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/AmpControls.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/PedalEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugin/AmpProcessorBase.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugin/PedalProcessor.cpp")

if(OAP_SHARED_JUCE)
    # Compiled once and linked into every plugin.
    add_library(oap_shared_ui STATIC ${oap_ui_sources})
    target_link_libraries(oap_shared_ui PUBLIC oap_shared_dsp oap_fx oap_juce)
    oap_configure_static_library(oap_shared_ui)
else()
    # Compiled into each plugin together with its own copy of JUCE.
    add_library(oap_shared_ui INTERFACE)
    target_sources(oap_shared_ui INTERFACE ${oap_ui_sources})
    target_link_libraries(oap_shared_ui INTERFACE oap_shared_dsp oap_fx)
endif()
