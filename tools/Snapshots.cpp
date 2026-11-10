// Renders the editor of every generic plugin (pedals, Pedalboard, effect amps,
// Looper) to PNG files. Used to produce the documentation images:
//
//   oap_snapshots <output-folder> [scale]

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "amps/EffectAmpProcessor.h"
#include "fx/Effect.h"
#include "plugin/LooperProcessor.h"
#include "plugin/PedalProcessor.h"
#include "plugin/PedalboardProcessor.h"
#include "ui/LooperEditor.h"

#include <cmath>
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

// The Looper is shown at work: a short session is played into it, with a
// picture taken at each stage.
void saveLooper (const juce::File& dir, float scale)
{
    using Switch = oap::LooperProcessor::Switch;
    constexpr double rate = 48000.0;
    constexpr int block = 480;

    oap::LooperProcessor looper;
    looper.prepareToPlay (rate, block);

    std::unique_ptr<juce::AudioProcessorEditor> editor (looper.createEditorAndMakeActive());
    auto* view = dynamic_cast<oap::ui::LooperEditor*> (editor.get());
    if (view == nullptr)
        return;

    const auto shot = [&] (const char* name)
    {
        view->refresh();
        const auto file = dir.getChildFile (name);
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat png;
        png.writeImageToStream (image, out);
        std::printf ("wrote %s (%dx%d)\n", file.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
    };

    // Something like a guitar: plucked notes, two to the beat at 120 BPM.
    std::int64_t now = 0;
    const auto play = [&] (double seconds, float level, int transpose)
    {
        static constexpr int notes[] = { 0, 7, 12, 7, 3, 10, 15, 10 };
        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;

        for (int blocks = (int) std::lround (seconds * rate / block); blocks > 0; --blocks)
        {
            for (int i = 0; i < block; ++i)
            {
                const double t = (double) (now + i) / rate;
                const double index = std::floor (t * 4.0), inNote = t - index * 0.25;
                const double freq = 110.0 * std::pow (2.0, (notes[(int) index % 8] + transpose) / 12.0);
                const double phase = juce::MathConstants<double>::twoPi * freq * inNote;
                const auto x = (float) (level * std::exp (-inNote * 9.0) * (std::sin (phase) + 0.4 * std::sin (2.0 * phase)));
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, x);
            }

            looper.processBlock (buffer, midi);
            now += block;
            if (blocks % 3 == 0)
                view->refresh(); // what the editor's timer does
        }
    };

    const auto tap = [&] { looper.sendSwitch (Switch::mainDown); looper.sendSwitch (Switch::mainUp); };

    shot ("looper-empty.png");

    tap();                       // record
    play (3.0, 0.6f, 0);
    shot ("looper-recording.png");
    play (1.0, 0.6f, 0);

    tap();                       // play
    play (1.0, 0.0f, 0);
    tap();                       // overdub
    play (4.0, 0.45f, 12);
    tap();                       // play
    play (0.5, 0.0f, 0);
    juce::Thread::sleep (500);   // the picture of the loop is made in the background
    play (2.3, 0.0f, 0);
    shot ("looper.png");

    play (1.2, 0.0f, 0);
    tap();                       // overdub again
    play (2.2, 0.5f, 19);
    shot ("looper-overdub.png");

    looper.editorBeingDeleted (editor.get());
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

    for (const auto& model : oap::amps::allAmpModels())
    {
        oap::amps::EffectAmpProcessor amp (model.id);
        amp.prepareToPlay (48000.0, 512);
        save (amp, dir.getChildFile ("amp-" + juce::String (model.id) + ".png"), scale);
    }

    {
        oap::PedalboardProcessor board;
        board.prepareToPlay (48000.0, 512);
        save (board, dir.getChildFile ("pedalboard.png"), scale);
    }

    saveLooper (dir, scale);

    return 0;
}
