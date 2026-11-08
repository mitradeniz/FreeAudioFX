// Renders the editor of every generic plugin (pedals, Pedalboard, effect amps)
// to PNG files. Used to produce the documentation images:
//
//   oap_snapshots <output-folder> [scale]

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "fx/Effect.h"
#include "plugin/PedalProcessor.h"
#include "plugin/PedalboardProcessor.h"

#include <cstdio>

namespace
{
void save (juce::AudioProcessor& processor, const juce::File& file, float scale)
{
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditorAndMakeActive());
    if (editor == nullptr)
        return;

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);
    file.deleteFile();
    juce::FileOutputStream out (file);
    juce::PNGImageFormat png;
    png.writeImageToStream (image, out);
    std::printf ("wrote %s (%dx%d)\n", file.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
    processor.editorBeingDeleted (editor.get());
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "snapshots");
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 1.0f;
    dir.createDirectory();

    for (const auto* info : oap::fx::allEffects())
    {
        oap::PedalProcessor pedal (info->id);
        save (pedal, dir.getChildFile ("pedal-" + juce::String (info->id) + ".png"), scale);
    }

    {
        oap::PedalboardProcessor board;
        board.prepareToPlay (48000.0, 512);
        save (board, dir.getChildFile ("pedalboard.png"), scale);
    }

    return 0;
}
