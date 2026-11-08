# Shared helper so every plugin in the collection is configured the same way.
#
#   oap_add_plugin(<target>
#       PRODUCT_NAME    "OAP Something"
#       PLUGIN_CODE     Abcd            # 4 chars, exactly one upper-case letter, unique
#       DESCRIPTION     "What it does"
#       VST3_CATEGORIES Fx Distortion
#       [STANDALONE]                    # also build a standalone app
#       [FOLDER amps|pedals|tools]
#       SOURCES ...
#       LIBRARIES ...)                  # extra libraries to link (e.g. oap_fx)

set(OAP_COMPANY_NAME      "Open Audio Plugins")
set(OAP_MANUFACTURER_CODE "Oapl")
set(OAP_BUNDLE_PREFIX     "org.openaudioplugins")

set(OAP_ALL_PLUGINS "" CACHE INTERNAL "")

function(oap_add_plugin target)
    cmake_parse_arguments(ARG "STANDALONE" "PRODUCT_NAME;PLUGIN_CODE;DESCRIPTION;FOLDER" "VST3_CATEGORIES;SOURCES;LIBRARIES" ${ARGN})

    set(formats VST3)
    if(APPLE)
        list(APPEND formats AU)
    endif()
    if(ARG_STANDALONE)
        list(APPEND formats Standalone)
    endif()

    juce_add_plugin(${target}
        COMPANY_NAME                "${OAP_COMPANY_NAME}"
        BUNDLE_ID                   "${OAP_BUNDLE_PREFIX}.${target}"
        PLUGIN_MANUFACTURER_CODE    ${OAP_MANUFACTURER_CODE}
        PLUGIN_CODE                 ${ARG_PLUGIN_CODE}
        PRODUCT_NAME                "${ARG_PRODUCT_NAME}"
        DESCRIPTION                 "${ARG_DESCRIPTION}"
        VERSION                     ${PROJECT_VERSION}
        FORMATS                     ${formats}
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

    target_link_libraries(${target}
        PRIVATE
            ${ARG_LIBRARIES}
            oap_shared_ui
            oap_juce
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    if(NOT ARG_FOLDER)
        set(ARG_FOLDER plugins)
    endif()
    set_target_properties(${target} PROPERTIES FOLDER "${ARG_FOLDER}")

    set(OAP_ALL_PLUGINS ${OAP_ALL_PLUGINS} ${target} CACHE INTERNAL "")
endfunction()
