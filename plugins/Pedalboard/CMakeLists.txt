oap_add_plugin(Pedalboard
    PRODUCT_NAME    "OAP Pedalboard"
    PLUGIN_CODE     Pdbd
    DESCRIPTION     "Eight-slot pedalboard with every OAP pedal"
    VST3_CATEGORIES Fx
    STANDALONE
    FOLDER          tools
    SOURCES         PedalboardPlugin.cpp
    LIBRARIES       oap_fx)
