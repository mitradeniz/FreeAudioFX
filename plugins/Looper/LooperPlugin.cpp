#include "plugin/LooperProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new oap::LooperProcessor();
}
