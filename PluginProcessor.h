#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class SignatureVocalStripAudioProcessor : public juce::AudioProcessor
{
public:
    SignatureVocalStripAudioProcessor();
    ~SignatureVocalStripAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Signature Vocal Strip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Chains
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    // 1. Aphex HPF
    Filter aphexHpL, aphexHpR;

    // 2. CLA EQ & Dynamics
    Filter claBassL, claBassR;
    Filter claTrebleL, claTrebleR;
    juce::dsp::Compressor<float> claWallComp;

    // 3. CLA-2A Opto Comp
    juce::dsp::Compressor<float> optoComp;

    // 4. De-Esser Bandpass
    Filter deEssBpL, deEssBpR;
    float deEssEnv = 0.0f;

    // 5. FabFilter Pro-Q
    Filter proQHpL, proQHpR;
    Filter proQBand2L, proQBand2R;
    Filter proQBand3L, proQBand3R;
    Filter proQBand4L, proQBand4R;

    // 6. Pro EQ3
    Filter proEqLcL, proEqLcR;
    Filter proEqLfL, proEqLfR;
    Filter proEqMfL, proEqMfR;
    Filter proEqLmfL, proEqLmfR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignatureVocalStripAudioProcessor)
};
