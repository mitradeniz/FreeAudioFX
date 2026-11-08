#include "plugin/PedalboardProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new oap::PedalboardProcessor();
}
