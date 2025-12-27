#pragma once

#include <JuceHeader.h>

//==============================================================================
class AuricOmega76AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AuricOmega76AudioProcessor();
    ~AuricOmega76AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==================== APVTS ====================
    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();
    APVTS apvts;

    // meter for UI
    float getGainReductionDb() const noexcept { return grDb.load(); }

private:
    //==============================================================================
    std::atomic<float> grDb { 0.0f };

    double sr { 44100.0 };

    // sidechain HPF for detector (NO juce_dsp)
    juce::IIRFilter scHpfL, scHpfR;
    juce::IIRCoefficients scHpfCoeffs;

    void updateSidechainHPF();

    // envelope + gain smoothing
    float env { 0.0f };
    float gainLin { 1.0f };

    // helpers
    static inline float dbToLin (float db) noexcept { return std::pow (10.0f, db / 20.0f); }
    static inline float linToDb (float lin) noexcept
    {
        return 20.0f * std::log10 (juce::jmax (lin, 1.0e-8f));
    }

    float getParam (const juce::String& id, float fallback) const noexcept;
    int   getChoice (const juce::String& id, int fallback) const noexcept;
    bool  getBool   (const juce::String& id, bool fallback) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuricOmega76AudioProcessor)
};
