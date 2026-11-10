oap_add_plugin(Looper
    PRODUCT_NAME    "OAP Looper"
    PLUGIN_CODE     Loop
    DESCRIPTION     "Looper with overdub, undo, tempo sync and loops saved with the project"
    VST3_CATEGORIES Fx Tools
    STANDALONE
    FOLDER          tools
    SOURCES         LooperPlugin.cpp
    LIBRARIES       oap_fx)
