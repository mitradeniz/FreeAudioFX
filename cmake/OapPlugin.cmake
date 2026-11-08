# Shared helper so every plugin in the collection is configured the same way.
#
#   oap_add_plugin(<target>
#       PRODUCT_NAME "OAP Something"
#       PLUGIN_CODE  Abcd                # 4 chars, exactly one upper-case letter
#       VST3_CATEGORIES Fx Distortion
#       SOURCES ...)

set(OAP_COMPANY_NAME      "Open Audio Plugins")
set(OAP_MANUFACTURER_CODE "Oapl")
set(OAP_BUNDLE_PREFIX     "org.openaudioplugins")

set(OAP_PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND OAP_PLUGIN_FORMATS AU)
endif()

function(oap_add_plugin target)
    cmake_parse_arguments(ARG "" "PRODUCT_NAME;PLUGIN_CODE;DESCRIPTION" "VST3_CATEGORIES;SOURCES" ${ARGN})

    juce_add_plugin(${target}
        COMPANY_NAME                "${OAP_COMPANY_NAME}"
        BUNDLE_ID                   "${OAP_BUNDLE_PREFIX}.${target}"
        PLUGIN_MANUFACTURER_CODE    ${OAP_MANUFACTURER_CODE}
        PLUGIN_CODE                 ${ARG_PLUGIN_CODE}
        PRODUCT_NAME                "${ARG_PRODUCT_NAME}"
        DESCRIPTION                 "${ARG_DESCRIPTION}"
        VERSION                     ${PROJECT_VERSION}
        FORMATS                     ${OAP_PLUGIN_FORMATS}
        IS_SYNTH                    FALSE
        NEEDS_MIDI_INPUT            FALSE
        NEEDS_MIDI_OUTPUT           FALSE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES             ${ARG_VST3_CATEGORIES}
        AU_MAIN_TYPE                kAudioUnitType_Effect
        COPY_PLUGIN_AFTER_BUILD     ${OAP_COPY_PLUGINS}
        NEEDS_WEB_BROWSER           FALSE
        NEEDS_CURL                  FALSE)

    target_sources(${target} PRIVATE ${ARG_SOURCES})
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(${target}
        PRIVATE
            oap_shared_ui
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    set_target_properties(${target} PROPERTIES FOLDER "plugins")
endfunction()
