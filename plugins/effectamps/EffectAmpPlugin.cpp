// Every effect amp plugin is built from this file; CMake sets OAP_AMP_MODEL_ID
// to the id of the amp model (see plugins/effectamps/CMakeLists.txt and
// shared/amps/AmpModels.cpp).

#include "amps/EffectAmpProcessor.h"

#ifndef OAP_AMP_MODEL_ID
 #error "OAP_AMP_MODEL_ID must be defined"
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new oap::amps::EffectAmpProcessor (OAP_AMP_MODEL_ID);
}
