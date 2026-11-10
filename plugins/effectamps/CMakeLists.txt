# One plugin per amp model in shared/amps/AmpModels.cpp. They all share
# EffectAmpPlugin.cpp and differ only in the model id, name and codes below.
#
# Plugin codes are part of saved sessions: never change them once released.

function(oap_add_effect_amp target model_id product_name plugin_code description)
    oap_add_plugin(${target}
        PRODUCT_NAME    "${product_name}"
        PLUGIN_CODE     ${plugin_code}
        DESCRIPTION     "${description}"
        VST3_CATEGORIES Fx Distortion
        STANDALONE
        FOLDER          effectamps
        SOURCES         "${CMAKE_CURRENT_SOURCE_DIR}/EffectAmpPlugin.cpp"
        LIBRARIES       oap_fx)
    target_compile_definitions(${target} PRIVATE OAP_AMP_MODEL_ID="${model_id}")
endfunction()

oap_add_effect_amp(DreamAmp      dream      "OAP Dream Amp"       Drem "Clean amp with chorus, echo and shimmer reverb")
oap_add_effect_amp(DubAmp        dub        "OAP Dub Amp"         Dubx "Reggae clean amp with dub tape echo and spring")
oap_add_effect_amp(SpaceDelayAmp spacedelay "OAP Space Delay Amp" Spdl "Crunch amp with three-head tape echo and spring")
oap_add_effect_amp(BlackMetalAmp blackmetal "OAP Black Metal Amp" Blkm "Buzzsaw high-gain amp with cold hall reverb")
oap_add_effect_amp(WarmRockAmp   warmrock   "OAP Warm Rock Amp"   Wrmr "British crunch amp with analog delay and plate")
oap_add_effect_amp(SurfAmp       surf       "OAP Surf Amp"        Surf "Bright clean amp with spring tank and tremolo")
oap_add_effect_amp(ShoegazeAmp   shoegaze   "OAP Shoegaze Amp"    Shgz "Fuzz wall, vibrato and huge hall reverb")
oap_add_effect_amp(ArenaAmp      arena      "OAP Arena Amp"       Aren "80s lead amp with chorus, ping-pong delay and plate")
oap_add_effect_amp(StereoJazzAmp stereojazz "OAP Stereo Jazz Amp" Stjz "Solid-state clean amp with stereo chorus and spring")
oap_add_effect_amp(DoomAmp       doom       "OAP Doom Amp"        Doom "Fuzz into a cranked amp with phaser and room")
oap_add_effect_amp(CrunchAmp     crunch     "OAP Crunch Amp"      Crch "Plexi-style crunch amp with treble booster and room")
oap_add_effect_amp(EightBitAmp   eightbit   "OAP 8-Bit Amp"       Chip "Chiptune synth, bit crusher and pixel echo")
oap_add_effect_amp(KnightAmp     knight     "OAP Knight Amp"      Kngt "Heroic lead amp with harmony, echo and hall")
oap_add_effect_amp(HeavyMetalAmp heavymetal "OAP Heavy Metal Amp" Hvym "80s heavy metal amp with boost, delay and plate")
oap_add_effect_amp(SynthwaveAmp  synthwave  "OAP Synthwave Amp"   Snwv "Neon clean amp with chorus, flanger and synced echo")
oap_add_effect_amp(WesternAmp    western    "OAP Western Amp"     Wstn "Twang amp with slapback, tremolo and spring")
oap_add_effect_amp(FunkAmp       funk       "OAP Funk Amp"        Funk "Clean amp with compressor, envelope filter and phaser")
oap_add_effect_amp(PharaohAmp    pharaoh    "OAP Pharaoh Amp"     Phrh "Desert crunch with tape echo, mirage and temple hall")
oap_add_effect_amp(SakuraAmp     sakura     "OAP Sakura Amp"      Skra "City-pop clean amp with compressor, chorus and plate")
oap_add_effect_amp(PersianAmp    persian    "OAP Persian Amp"     Prsn "Silky clean amp with octave sparkle, echo and hall")
oap_add_effect_amp(ChoirAmp      choir      "OAP Choir Amp"       Chir "Heavenly clean amp with choir shimmer and cathedral")
