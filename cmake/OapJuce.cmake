# Provides the `oap_juce` target that every plugin and test links against.
#
# With OAP_SHARED_JUCE=ON (default) the JUCE module sources are compiled ONCE
# into a static library that all plugins share. Normally JUCE compiles every
# module again for every plugin, which would mean building JUCE 30+ times for
# this collection. Only the thin per-plugin wrappers (VST3/AU/Standalone) are
# still compiled per plugin.
#
# With OAP_SHARED_JUCE=OFF each plugin compiles its own copy of JUCE, exactly as
# a stock JUCE CMake project does (slower, but useful if something misbehaves).

option(OAP_SHARED_JUCE "Compile the JUCE modules once and share them between all plugins" ON)

set(OAP_JUCE_ROOT_MODULES juce_audio_utils juce_dsp juce_audio_plugin_client)

# Collects the JUCE modules that `modules` depend on (recursively).
function(_oap_collect_juce_modules out_var)
    set(pending ${ARGN})
    set(found)
    while(pending)
        list(POP_FRONT pending module)
        string(REPLACE "juce::" "" module "${module}")
        if(NOT TARGET ${module} OR module IN_LIST found)
            continue()
        endif()
        get_target_property(module_path ${module} INTERFACE_JUCE_MODULE_PATH)
        if(NOT module_path)
            continue() # not a JUCE module (e.g. a helper or system library target)
        endif()
        list(APPEND found ${module})
        get_target_property(deps ${module} INTERFACE_LINK_LIBRARIES)
        if(deps)
            list(APPEND pending ${deps})
        endif()
    endwhile()
    set(${out_var} ${found} PARENT_SCOPE)
endfunction()

if(OAP_SHARED_JUCE)
    _oap_collect_juce_modules(oap_juce_modules ${OAP_JUCE_ROOT_MODULES})
    list(REMOVE_ITEM oap_juce_modules juce_audio_plugin_client) # wrappers stay per plugin

    add_library(oap_juce STATIC)

    foreach(module IN LISTS oap_juce_modules)
        get_target_property(sources ${module} INTERFACE_SOURCES)
        if(sources)
            target_sources(oap_juce PRIVATE ${sources})
            # Consumers keep the module's headers, definitions and system libraries,
            # but no longer compile its sources themselves.
            set_property(TARGET ${module} PROPERTY INTERFACE_SOURCES "")
        endif()
    endforeach()

    target_link_libraries(oap_juce
        PUBLIC
            juce::juce_audio_utils
            juce::juce_dsp
        PRIVATE
            juce::juce_recommended_config_flags)

    # Must match what juce_add_plugin() gives the per-plugin code, otherwise the
    # debug/release consistency checks inside JUCE fail to link. The public ones
    # are also needed by our own static libraries that include JUCE headers.
    _juce_get_debug_config_genex(oap_debug)
    target_compile_definitions(oap_juce
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
            $<${oap_debug}:DEBUG=1>
            $<${oap_debug}:_DEBUG=1>
            $<$<NOT:${oap_debug}>:NDEBUG=1>
            $<$<NOT:${oap_debug}>:_NDEBUG=1>
        PRIVATE
            JUCE_STANDALONE_APPLICATION=1
            "JucePlugin_Name=\"Open Audio Plugins\""
            JucePlugin_Build_VST3=1
            JucePlugin_Build_AU=$<BOOL:${APPLE}>
            JucePlugin_Build_Standalone=1
            JucePlugin_Build_AAX=0
            JucePlugin_Enable_ARA=0
            JucePlugin_Enable_IAA=0)

    target_compile_features(oap_juce PUBLIC cxx_std_17)

    set_target_properties(oap_juce PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE
        VISIBILITY_INLINES_HIDDEN TRUE
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        FOLDER "JUCE")

    list(LENGTH oap_juce_modules module_count)
    message(STATUS "OAP: compiling ${module_count} JUCE modules once, shared by all plugins")
else()
    add_library(oap_juce INTERFACE)
    target_link_libraries(oap_juce INTERFACE juce::juce_audio_utils juce::juce_dsp)
    target_compile_definitions(oap_juce INTERFACE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
endif()
