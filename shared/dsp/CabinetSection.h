#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

#include "CabinetModel.h"

#include <atomic>

namespace oap::dsp
{

//==============================================================================
/** Mono speaker-cabinet stage: built-in voicings or a user impulse response.

    - Switching cabinets crossfades, so there are no clicks while playing.
    - Impulse responses are read and level-matched on the message thread and
      handed to the audio thread through a lock-free slot, because
      juce::dsp::Convolution expects load calls to be synchronised with
      process calls.
    - If "User IR" is selected but no IR is loaded yet, the 4x12 voicing is
      used so the amp never suddenly sounds raw.
*/
class CabinetSection
{
public:
    enum Type { Off = 0, Closed4x12, Open2x12, Combo1x12, UserIR, NumTypes };

    static juce::StringArray getTypeNames()
    {
        return { "Off", "4x12 Closed", "2x12 Open", "1x12 Combo", "User IR" };
    }

    //==============================================================================
    void prepare (double sampleRate, int maxBlockSize)
    {
        fs = sampleRate;
        models[0].prepare (fs, CabinetModel::Voicing::Closed4x12);
        models[1].prepare (fs, CabinetModel::Voicing::Open2x12);
        models[2].prepare (fs, CabinetModel::Voicing::Combo1x12);

        convolution.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, maxBlockSize), 1 });
        scratch.assign ((size_t) juce::jmax (1, maxBlockSize), 0.0f);
        fadeLength = juce::jmax (1, (int) (0.02 * fs));
        reset();
    }

    void reset() noexcept
    {
        for (auto& m : models)
            m.reset();

        convolution.reset();
        fadeRemaining = 0;
        previousType = activeType.load();
    }

    void setType (int newType) noexcept { requestedType = juce::jlimit (0, NumTypes - 1, newType); }

    /** The cabinet actually being heard (differs from the requested one while an IR is loading). */
    int getActiveType() const noexcept { return activeType.load(); }

    bool hasUserIR() const noexcept { return userIRAvailable.load(); }

    //==============================================================================
    /** Call from the message thread. Returns an empty string on success, or an error message. */
    juce::String loadImpulseResponse (const juce::File& file)
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

        if (reader == nullptr)
            return "Could not read \"" + file.getFileName() + "\" as an audio file.";

        if (reader->sampleRate <= 0.0 || reader->lengthInSamples < 2)
            return "\"" + file.getFileName() + "\" is empty.";

        const auto maxLength = (juce::int64) (reader->sampleRate * maxIRSeconds);
        const int length = (int) juce::jmin (reader->lengthInSamples, maxLength);
        const int channels = (int) juce::jlimit (1u, 2u, reader->numChannels);

        juce::AudioBuffer<float> raw (channels, length);
        reader->read (&raw, 0, length, 0, true, channels > 1);

        // Mix to mono: guitar cab IRs are mono, and the amp runs in mono.
        juce::AudioBuffer<float> ir (1, length);
        ir.copyFrom (0, 0, raw, 0, 0, length);
        if (channels > 1)
        {
            ir.addFrom (0, 0, raw, 1, 0, length);
            ir.applyGain (0.5f);
        }

        // Fade out the tail if the file was truncated.
        if (reader->lengthInSamples > maxLength)
        {
            const int fade = juce::jmin (length, (int) (0.01 * reader->sampleRate));
            ir.applyGainRamp (0, length - fade, fade, 1.0f, 0.0f);
        }

        const double level = measureLevel (ir, reader->sampleRate);
        if (level < 1.0e-9)
            return "\"" + file.getFileName() + "\" is silent.";

        ir.applyGain ((float) (1.0 / level));

        if (! handOver (std::move (ir), reader->sampleRate))
            return "The audio engine is busy, please try again.";

        userIRAvailable = true;
        return {};
    }

    /** Message thread: forget the user IR (the "User IR" slot falls back to the 4x12 voicing). */
    void clearImpulseResponse() noexcept { userIRAvailable = false; }

    //==============================================================================
    void process (float* data, int numSamples) noexcept
    {
        jassert (numSamples <= (int) scratch.size());
        numSamples = juce::jmin (numSamples, (int) scratch.size());

        receivePendingIR();

        const int requested = requestedType.load();
        const bool wantsIR = requested == UserIR && userIRAvailable.load();
        const bool irReady = wantsIR && convolution.getCurrentIRSize() > 1;

        if (wantsIR && ! irReady)
            pumpConvolution(); // lets juce::dsp::Convolution install the engine built in the background

        const int target = requested == UserIR ? (irReady ? (int) UserIR : (int) Closed4x12)
                                               : requested;

        if (target != activeType.load() && fadeRemaining == 0)
        {
            previousType = activeType.load();
            activeType = target;
            resetType (target);
            fadeRemaining = fadeLength;
        }

        if (fadeRemaining == 0)
        {
            processType (activeType.load(), data, numSamples);
            return;
        }

        std::copy (data, data + numSamples, scratch.begin());
        processType (previousType, scratch.data(), numSamples);
        processType (activeType.load(), data, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const float t = fadeRemaining > 0 ? 1.0f - (float) fadeRemaining / (float) fadeLength : 1.0f;
            data[i] = scratch[(size_t) i] + t * (data[i] - scratch[(size_t) i]);

            if (fadeRemaining > 0)
                --fadeRemaining;
        }
    }

    /** Same level-matching measure the built-in voicings use. */
    static double measureLevel (const juce::AudioBuffer<float>& ir, double sampleRate)
    {
        const auto* x = ir.getReadPointer (0);
        const int n = ir.getNumSamples();
        double sum = 0.0;
        const auto freqs = CabinetModel::referenceFrequencies();

        for (double f : freqs)
        {
            const double w = -2.0 * kPi * f / sampleRate;
            std::complex<double> acc { 0.0, 0.0 };
            const std::complex<double> step = std::polar (1.0, w);
            std::complex<double> rot { 1.0, 0.0 };

            for (int i = 0; i < n; ++i)
            {
                acc += (double) x[i] * rot;
                rot *= step;
                if ((i & 1023) == 1023)
                    rot /= std::abs (rot); // keep the phasor on the unit circle
            }

            sum += std::norm (acc);
        }

        return std::sqrt (sum / (double) freqs.size());
    }

private:
    static constexpr double maxIRSeconds = 1.0;

    void processType (int type, float* data, int numSamples) noexcept
    {
        switch (type)
        {
            case Closed4x12:
            case Open2x12:
            case Combo1x12:
            {
                auto& m = models[(size_t) (type - Closed4x12)];
                for (int i = 0; i < numSamples; ++i)
                    data[i] = (float) m.process (data[i]);
                break;
            }

            case UserIR:
            {
                float* channels[] = { data };
                juce::dsp::AudioBlock<float> block (channels, 1, (size_t) numSamples);
                convolution.process (juce::dsp::ProcessContextReplacing<float> (block));
                break;
            }

            case Off:
            default:
                break;
        }
    }

    void resetType (int type) noexcept
    {
        if (type == UserIR)
            convolution.reset();
        else if (type >= Closed4x12 && type <= Combo1x12)
            models[(size_t) (type - Closed4x12)].reset();
    }

    void pumpConvolution() noexcept
    {
        float silence[1] = { 0.0f };
        float* channels[] = { silence };
        juce::dsp::AudioBlock<float> block (channels, 1, 1);
        convolution.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    // Lock-free single-slot handover: message thread -> audio thread.
    enum SlotState { slotIdle, slotWriting, slotReady, slotConsuming };

    bool handOver (juce::AudioBuffer<float>&& ir, double sampleRate)
    {
        for (int attempt = 0; attempt < 500; ++attempt)
        {
            int expected = slotIdle;
            if (slotState.compare_exchange_strong (expected, slotWriting))
                break;

            expected = slotReady; // replace an IR the audio thread hasn't picked up yet
            if (slotState.compare_exchange_strong (expected, slotWriting))
                break;

            if (attempt == 499)
                return false;

            juce::Thread::sleep (1);
        }

        pendingIR = std::move (ir);
        pendingRate = sampleRate;
        slotState.store (slotReady, std::memory_order_release);
        return true;
    }

    void receivePendingIR() noexcept
    {
        int expected = slotReady;
        if (! slotState.compare_exchange_strong (expected, slotConsuming, std::memory_order_acquire))
            return;

        convolution.loadImpulseResponse (std::move (pendingIR), pendingRate,
                                         juce::dsp::Convolution::Stereo::no,
                                         juce::dsp::Convolution::Trim::yes,
                                         juce::dsp::Convolution::Normalise::no);
        slotState.store (slotIdle, std::memory_order_release);
    }

    double fs = 48000.0;
    std::array<CabinetModel, 3> models;
    juce::dsp::Convolution convolution;
    std::vector<float> scratch;

    std::atomic<int> requestedType { Closed4x12 };
    std::atomic<int> activeType { Closed4x12 };
    int previousType = Closed4x12;
    int fadeLength = 960, fadeRemaining = 0;

    std::atomic<bool> userIRAvailable { false };
    std::atomic<int> slotState { slotIdle };
    juce::AudioBuffer<float> pendingIR;
    double pendingRate = 48000.0;
};

} // namespace oap::dsp
