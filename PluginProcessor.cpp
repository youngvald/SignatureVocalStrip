#include "PluginProcessor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SignatureVocalStripAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("Body", "Body", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Clarity", "Clarity", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DeEsser", "De-Esser", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("WallComp", "Wall Comp", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OptoGlue", "Opto Glue", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("AirExciter", "Air Exciter", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Output", "Output", -12.0f, 12.0f, 0.0f));

    return { params.begin(), params.end() };
}

SignatureVocalStripAudioProcessor::SignatureVocalStripAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

SignatureVocalStripAudioProcessor::~SignatureVocalStripAudioProcessor() {}

void SignatureVocalStripAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    claWallComp.prepare(spec);
    claWallComp.setAttack(1.5f);
    claWallComp.setRelease(75.0f);
    claWallComp.setRatio(10.0f);
    claWallComp.setThreshold(-18.0f);

    optoComp.prepare(spec);
    optoComp.setAttack(10.0f);
    optoComp.setRelease(120.0f);
    optoComp.setRatio(3.2f);
    optoComp.setThreshold(-22.0f);

    // Filters initial setup
    auto sr = (float)sampleRate;
    auto hpCoeff = Coefficients::makeHighPass(sr, 3500.0f, 0.707f);
    aphexHpL.coefficients = hpCoeff; aphexHpR.coefficients = hpCoeff;

    auto claB = Coefficients::makeLowShelf(sr, 180.0f, 0.707f, juce::Decibels::decibelsToGain(-6.1f));
    claBassL.coefficients = claB; claBassR.coefficients = claB;

    auto claT = Coefficients::makeHighShelf(sr, 8000.0f, 0.707f, juce::Decibels::decibelsToGain(7.9f));
    claTrebleL.coefficients = claT; claTrebleR.coefficients = claT;

    auto deEssCoeff = Coefficients::makeBandPass(sr, 5760.0f, 3.5f);
    deEssBpL.coefficients = deEssCoeff; deEssBpR.coefficients = deEssCoeff;

    auto qHp = Coefficients::makeHighPass(sr, 60.0f, 0.707f);
    proQHpL.coefficients = qHp; proQHpR.coefficients = qHp;

    auto qB2 = Coefficients::makePeakFilter(sr, 1282.8f, 2.929f, juproQBand2L.coefficients = qB2; proQBand2R.coefficients = qB2;

    auto qB3 = Coefficients::makePeakFilter(sr, 2262.5f, 2.929f, juce::Decibels::decibelsToGain(3.54f));
    proQBand3L.coefficients = qB3; proQBand3R.coefficients = qB3;

    auto qB4 = Coefficients::makePeakFilter(sr, 3517.5f, 4.399f, juce::Decibels::decibelsToGain(2.13f));
    proQBand4L.coefficients = qB4; proQBand4R.coefficients = qB4;

    auto eqLc = Coefficients::makeHighPass(sr, 152.0f, 0.707f);
    proEqLcL.coefficients = eqLc; proEqLcR.coefficients = eqLc;

    auto eqLf = Coefficients::makePeakFilter(sr, 841.0f, 2.20f, juce::Decibels::decibelsToGain(2.58f));
    proEqLfL.coefficients = eqLf; proEqLfR.coefficients = eqLf;

    auto eqMf = Coefficients::makePeakFilter(sr, 2590.0f, 4.00f,proEqMfL.coefficients = eqMf; proEqMfR.coefficients = eqMf;

    auto eqLmf = Coefficients::makePeakFilter(sr, 5410.0f, 6.10f, juce::Decibels::decibelsToGain(-4.80f));
    proEqLmfL.coefficients = eqLmf; proEqLmfR.coefficients = eqLmf;
}

void SignatureVocalStripAudioProcessor::releaseResources() {}

void SignatureVocalStripAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    if (numChannels < 2) return;

    float bodyMacro = apvts.getRawParameterValue("Body")->load() * 0.02f;
    float clarityMacro = apvts.getRawParameterValue("Clarity")->load() * 0.02f;
    float deEssMacro = apvts.getRawParameterValue("DeEsser")->load() * 0.02f;
    float wallMacro = apvts.getRawParameterValue("WallComp")->load() * 0.02f;
    float optoMacro = apvts.getRawParameterValue("OptoGlue")->load() * 0.02f;
    float airMacro = apvts.getRawParameterValue("AirExciter")->load() * 0.02f;
    float outGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("Output")->load());

    float aphexIn = juce::Decibels::decibelsToGain(-2.2f);
    float aphexBlend = (4.77f * 0.1f) * airMacro;
    float aphexOut = juce::Decibels::decibelsToGain(1.5f);

    float claSens = juce::Decibels::decibelsToGain(2.8f);
    float claOut = juce::Decibels::decibelsToGain(2.0f);
    float optoMakeup = juce::Decibels::decibelsToGain(8.5f);
    float deEssThresh = juce::Decibels::decibelsToGain(-20.31f);

    claWallComp.setThreshold(-18.0f - (wallMacro - 1.0f) * 6.0f);
    optoComp.setThreshold(-22.0f - (optoMacro - 1.0f) * 6.0f);

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        float sL = left[i];
        float sR = right[i];

        // 1. Aphex Exciter
        float pAxL = sL * aphexIn;
        float pAxR = sR * aphexIn;
        float hL = std::tanh(aphexHpL.processSample(pAxL) * 2.2f + 0.1f) - 0.08f;
        float hR = std::tanh(aphexHpR.processSample(pAxR) * 2.2f + 0.1f) - 0.08f;
        sL = (pAxL + hL * aphexBlend) * aphexOut;
        sR = (pAxR + hR * aphexBlend) * aphexOut;

        // 2. CLA Vocals
        sL = claTrebleL.processSample(claBassL.processSample(sL * claSens));
        sR = claTrebleR.processSample(claBassR.processSample(sR * claSens));
        sL = claWallComp.processSample(0, sL) * claOut;
        sR = claWallComp.processSample(1, sR) * claOut;

        // 3. CLA-2A Opto Comp
        sL = optoComp.processSample(0, sL) * optoMakeup;
        sR = optoComp.processSample(1, sR) * optoMakeup;

        // 4. De-Esser (5.76 kHz)
        float sib = std::max(std::abs(deEssBpL.processSample(sL)), std::abs(deEssBpR.processSample(sR)));
        deEssEnv = deEssEnv * 0.95f + sib * 0.05f;
        if (deEssEnv > deEssThresh && deEssMacro > 0.05f)
        {
            float duck = 1.0f / (1.0f + (deEssEnv / deEssThresh - 1.0f) * 1.8f * deEssMacro);
            sL *= duck;
            sR *= duck;
        }

        // 5. FabFilter Pro-Q
        sL = proQBand4L.processSample(proQBand3L.processSample(proQBand2L.processSample(proQHpL.processSample(sL))));
        sR = proQBand4R.processSample(proQBand3R.processSample(proQBand2R.processSample(proQHpR.processSample(sR))));

        // 6. Pro EQ3
        sL = proEqLmfL.processSample(proEqMfL.processSample(proEqLfL.processSample(proEqLcL.processSample(sL))));
        sR = proEqLmfR.processSample(proEqMfR.processSample(proEqLfR.processSample(proEqLcR.processSample(sR))));

        left[i]  = std::clamp(sL * outGain, -1.0f, 1.0f);
        right[i] = std::clamp(sR * outGain, -1.0f, 1.0f);
    }
}

juce::AudioProcessorEditor* SignatureVocalStripAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void SignatureVocalStripAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SignatureVocalStripAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SignatureVocalStripAudioProcessor();
}
