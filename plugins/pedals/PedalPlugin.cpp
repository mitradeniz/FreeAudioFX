// Every pedal plugin is built from this file; CMake sets OAP_EFFECT_ID to the
// id of the effect (see plugins/pedals/CMakeLists.txt).

#include "plugin/PedalProcessor.h"

#ifndef OAP_EFFECT_ID
 #error "OAP_EFFECT_ID must be defined"
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new oap::PedalProcessor (OAP_EFFECT_ID);
}
