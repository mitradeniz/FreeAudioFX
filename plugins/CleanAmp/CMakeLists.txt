oap_add_plugin(CleanAmp
    PRODUCT_NAME    "OAP Clean Amp"
    PLUGIN_CODE     Clna
    DESCRIPTION     "Vintage clean guitar amplifier with reverb"
    VST3_CATEGORIES Fx Distortion
    SOURCES
        Source/CleanAmpEngine.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
