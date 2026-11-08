oap_add_plugin(DriveAmp
    PRODUCT_NAME    "OAP Drive Amp"
    PLUGIN_CODE     Drva
    DESCRIPTION     "High-gain guitar amplifier"
    VST3_CATEGORIES Fx Distortion
    STANDALONE
    FOLDER          amps
    SOURCES
        Source/DriveAmpEngine.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
