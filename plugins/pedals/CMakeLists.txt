# One plugin per effect in the oap_fx library. They all share PedalPlugin.cpp
# and differ only in the effect id, name and codes below.
#
# Plugin codes are part of saved sessions: never change them once released.

function(oap_add_pedal target effect_id product_name plugin_code description)
    oap_add_plugin(${target}
        PRODUCT_NAME    "${product_name}"
        PLUGIN_CODE     ${plugin_code}
        DESCRIPTION     "${description}"
        VST3_CATEGORIES ${ARGN}
        FOLDER          pedals
        SOURCES         "${CMAKE_CURRENT_SOURCE_DIR}/PedalPlugin.cpp"
        LIBRARIES       oap_fx)
    target_compile_definitions(${target} PRIVATE OAP_EFFECT_ID="${effect_id}")
endfunction()

# Everyday pedals
oap_add_pedal(CompressorPedal compressor "OAP Compressor"  Comp "Optical-style compressor"            Fx Dynamics)
oap_add_pedal(BoostPedal      boost      "OAP Boost"       Bost "Clean boost with EQ"                 Fx Distortion)
oap_add_pedal(OverdrivePedal  overdrive  "OAP Overdrive"   Ovdr "Mid-hump overdrive"                  Fx Distortion)
oap_add_pedal(DistortionPedal distortion "OAP Distortion"  Dist "Hard-clipping distortion"            Fx Distortion)
oap_add_pedal(FuzzPedal       fuzz       "OAP Fuzz"        Fuzz "Two-stage transistor fuzz"           Fx Distortion)
oap_add_pedal(ChorusPedal     chorus     "OAP Chorus"      Chor "Analog-style chorus and vibrato"     Fx Modulation)
oap_add_pedal(FlangerPedal    flanger    "OAP Flanger"     Flng "Through-zero style flanger"          Fx Modulation)
oap_add_pedal(PhaserPedal     phaser     "OAP Phaser"      Phsr "4 to 12 stage phaser"                Fx Modulation)
oap_add_pedal(TremoloPedal    tremolo    "OAP Tremolo"     Trem "Tremolo, harmonic tremolo, panner"   Fx Modulation)
oap_add_pedal(DelayPedal      delay      "OAP Delay"       Dlay "Digital, analog and ping-pong delay" Fx Delay)
oap_add_pedal(ReverbPedal     reverb     "OAP Reverb"      Revb "Room, hall, plate and spring reverb" Fx Reverb)
oap_add_pedal(WahPedal        wah        "OAP Wah"         Wahx "Manual, auto and envelope wah"       Fx Filter)

# Advanced series
oap_add_pedal(ShimmerPedal    shimmer    "OAP Shimmer"         Shim "Pitch-shifted ambient reverb"     Fx Reverb)
oap_add_pedal(TapeEchoPedal   tapeecho   "OAP Tape Echo"       Tape "Multi-head tape echo"             Fx Delay)
oap_add_pedal(PitchPedal      pitch      "OAP Pitch Shifter"   Ptch "Octaves, harmonies and bends"     Fx "Pitch Shift")
oap_add_pedal(EnvFilterPedal  envfilter  "OAP Envelope Filter" Envf "Dynamics-controlled filter sweep" Fx Filter)
oap_add_pedal(RotaryPedal     rotary     "OAP Rotary"          Rota "Rotating speaker cabinet"         Fx Modulation)
oap_add_pedal(FreezePedal     freeze     "OAP Freeze"          Frez "Granular sound-on-sound freeze"   Fx Delay)
oap_add_pedal(SynthPedal      synth      "OAP Guitar Synth"    Synt "Monophonic guitar synthesizer"    Fx Generator)
oap_add_pedal(LoFiPedal       lofi       "OAP Lo-Fi"           Lofi "Bit crusher and tape wobble"      Fx Distortion)
