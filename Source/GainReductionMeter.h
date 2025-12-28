//==============================================================================
// GainReductionMeter.h  (AURIC Ω76) — 1176-ish GR meter (filled, not empty)
//==============================================================================

#pragma once
#include <JuceHeader.h>

class GainReductionMeterComponent : public juce::Component
{
public:
    GainReductionMeterComponent();

    void setGainReductionDb (float db);                 // expects positive GR (0..maxDb)
    void setBallisticsMs (float attackMs, float releaseMs);
    void setUseBallistics (bool shouldUse) { useBallistics = shouldUse; }

    void tick (double hz); // call from editor timer (e.g. 60Hz)

    void paint (juce::Graphics& g) override;
    void resized() override {}

    void setMaxDb (float m) { maxDb = juce::jmax (1.0f, m); repaint(); }
    float getMaxDb() const  { return maxDb; }

private:
    // ballistics
    float currentDb = 0.0f;
    float targetDb  = 0.0f;

    bool  useBallistics  = true;
    float attackCoeffMs  = 18.0f;
    float releaseCoeffMs = 180.0f;

    // look
    float maxDb = 20.0f;

    juce::Image noise; // subtle screen texture

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeterComponent)
};
