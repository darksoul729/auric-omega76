#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AuricOmega76AudioProcessor::AuricOmega76AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "PARAMS", createParameterLayout())
#endif
{
}

AuricOmega76AudioProcessor::~AuricOmega76AudioProcessor() {}

//==============================================================================
const juce::String AuricOmega76AudioProcessor::getName() const { return JucePlugin_Name; }

bool AuricOmega76AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AuricOmega76AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AuricOmega76AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AuricOmega76AudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AuricOmega76AudioProcessor::getNumPrograms() { return 1; }
int AuricOmega76AudioProcessor::getCurrentProgram() { return 0; }
void AuricOmega76AudioProcessor::setCurrentProgram (int) {}
const juce::String AuricOmega76AudioProcessor::getProgramName (int) { return {}; }
void AuricOmega76AudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void AuricOmega76AudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    sr = sampleRate;

    env = 0.0f;
    gainLin = 1.0f;
    grDb.store (0.0f);

    scHpfL.reset();
    scHpfR.reset();
    updateSidechainHPF();
}

void AuricOmega76AudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AuricOmega76AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
}
#endif

//==============================================================================
float AuricOmega76AudioProcessor::getParam (const juce::String& id, float fallback) const noexcept
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load();
    return fallback;
}

int AuricOmega76AudioProcessor::getChoice (const juce::String& id, int fallback) const noexcept
{
    if (auto* p = apvts.getRawParameterValue (id))
        return (int) juce::jlimit (0.0f, 1000.0f, p->load());
    return fallback;
}

bool AuricOmega76AudioProcessor::getBool (const juce::String& id, bool fallback) const noexcept
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load() >= 0.5f;
    return fallback;
}

void AuricOmega76AudioProcessor::updateSidechainHPF()
{
    // SC HPF around 120 Hz (detector only)
    const double fc = 120.0;
    scHpfCoeffs = juce::IIRCoefficients::makeHighPass (sr, fc);

    scHpfL.setCoefficients (scHpfCoeffs);
    scHpfR.setCoefficients (scHpfCoeffs);
}

//==============================================================================
void AuricOmega76AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // parameters
    const bool pwr   = getBool ("pwr", true);
    const bool scHpf = getBool ("sc_hpf", false);

    const float inputDb = getParam ("input", 0.0f);
    const float relMs   = getParam ("release", 150.0f);
    const float edge    = getParam ("edge", 0.0f);   // 0..1
    const float mode    = getParam ("mode", 0.5f);   // 0..1
    const float mix     = getParam ("mix",  1.0f);   // 0..1
    const float omegaMix= getParam ("omega_mix", 1.0f); // 0..1

    const int omegaMode = getChoice ("omega_mode", 0);   // 0 clean,1 iron,2 grit
    const int routing   = getChoice ("routing", 0);      // 0 A,1 D,2 Ω

    if (! pwr)
    {
        grDb.store (0.0f);
        return; // hard bypass
    }

    // compressor tuning
    const float thresholdDb = -18.0f;

    // ratio berdasarkan omegaMode switch (discrete)
    float ratio = 2.0f;
    if (omegaMode == 1) ratio = 4.0f;
    if (omegaMode == 2) ratio = 8.0f;

    const float atkMs = 10.0f;
    const float atkCoeff = std::exp (-1.0f / (float) (0.001 * atkMs * sr));
    const float relCoeff = std::exp (-1.0f / (float) (0.001 * relMs * sr));

    // drive berdasarkan mode knob (continuous) + omegaMode boost
    const float driveBase = juce::jmap (mode, 0.0f, 1.0f, 1.0f, 12.0f);
    const float driveModeBoost = (omegaMode == 0 ? 1.0f : (omegaMode == 1 ? 1.35f : 1.8f));
    const float drive = driveBase * driveModeBoost;

    const float hard = juce::jmap (edge, 0.0f, 1.0f, 1.0f, 2.6f);

    const float inGain = dbToLin (inputDb);

    auto numSamples = buffer.getNumSamples();
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    float grDbLocal = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        float xL = L[n] * inGain;
        float xR = (R != nullptr ? R[n] * inGain : xL);

        // detector source
        float dL = xL;
        float dR = xR;

        if (scHpf)
        {
            dL = scHpfL.processSingleSampleRaw (dL);
            dR = scHpfR.processSingleSampleRaw (dR);
        }

        const float detector = 0.5f * (std::abs (dL) + std::abs (dR));

        // envelope follower
        const float coeff = (detector > env ? atkCoeff : relCoeff);
        env = detector + coeff * (env - detector);

        // compute GR
        const float envDb = linToDb (env + 1.0e-8f);

        float targetGainDb = 0.0f;
        if (envDb > thresholdDb)
        {
            const float over = envDb - thresholdDb;
            const float compressedOver = over / ratio;
            targetGainDb = (thresholdDb + compressedOver) - envDb; // negative
        }

        const float targetGainLin = dbToLin (targetGainDb);

        // smooth gain
        const float gCoeff = (targetGainLin < gainLin ? atkCoeff : relCoeff);
        gainLin = targetGainLin + gCoeff * (gainLin - targetGainLin);

        // blocks
        auto doComp = [&] (float& a, float& b)
        {
            a *= gainLin;
            b *= gainLin;
        };

        auto doDrive = [&] (float& a, float& b)
        {
            auto sat = [drive, hard] (float v)
            {
                float y = v * drive;
                y = std::atan (y * hard) / std::atan (hard); // stable
                return y;
            };

            a = sat (a);
            b = sat (b);
        };

        float wetL = xL;
        float wetR = xR;

        if (routing == 0)            // A = comp
        {
            doComp (wetL, wetR);
        }
        else if (routing == 1)       // D = drive
        {
            doDrive (wetL, wetR);
        }
        else                         // Ω = comp -> drive (+ omegaMix)
        {
            doComp (wetL, wetR);
            doDrive (wetL, wetR);
            wetL = juce::jmap (omegaMix, xL, wetL);
            wetR = juce::jmap (omegaMix, xR, wetR);
        }

        // wet/dry
        const float outL = juce::jmap (mix, xL, wetL);
        const float outR = juce::jmap (mix, xR, wetR);

        L[n] = outL;
        if (R != nullptr) R[n] = outR;

        const float grPosDb = -linToDb (gainLin);
        if (grPosDb > grDbLocal) grDbLocal = grPosDb;
    }

    grDb.store (juce::jlimit (0.0f, 30.0f, grDbLocal));
}

//==============================================================================
bool AuricOmega76AudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AuricOmega76AudioProcessor::createEditor()
{
    return new AuricOmega76AudioProcessorEditor (*this);
}

//==============================================================================
void AuricOmega76AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AuricOmega76AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr)
    {
        auto vt = juce::ValueTree::fromXml (*xml);
        if (vt.isValid())
            apvts.replaceState (vt);
    }
}

//==============================================================================
// Parameter Layout
AuricOmega76AudioProcessor::APVTS::ParameterLayout AuricOmega76AudioProcessor::createParameterLayout()
{
    using namespace juce;

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "input", 1 }, "INPUT",
        NormalisableRange<float> (-60.0f, 10.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "release", 1 }, "RELEASE",
        NormalisableRange<float> (10.0f, 1000.0f, 0.01f, 0.5f),
        150.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "edge", 1 }, "EDGE",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mode", 1 }, "MODE",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "MIX",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        1.0f));

    // supaya knob Ω MIX di UI match & gak perlu comment attachment
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "omega_mix", 1 }, "Ω MIX",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        1.0f));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { "sc_hpf", 1 }, "SC HPF",
        false));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { "pwr", 1 }, "PWR",
        true));

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "omega_mode", 1 }, "Ω MODE",
        StringArray { "CLEAN", "IRON", "GRIT" },
        0));

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "routing", 1 }, "ROUTING",
        StringArray { "A", "D", "Ω" },
        0));

    return { params.begin(), params.end() };
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AuricOmega76AudioProcessor();
}
