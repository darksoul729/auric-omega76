#pragma once
#include <JuceHeader.h>

//==============================================================================
// GR Meter (clean / no grain)
class GainReductionMeterComponent : public juce::Component
{
public:
    // GR dB target (pakai nilai positif: 0..20 biasanya)
    // kalau kamu kirim negatif, aman: akan di-abs dan di-clamp.
    void setGainReductionDb (float db);

    void setUseBallistics (bool b) { useBallistics = b; }

    // attack/release feel analog (ms)
    void setBallisticsMs (float attackMs, float releaseMs);

    // panggil dari Timer editor (misal 30Hz / 60Hz)
    void tick (double rateHz);

    float getCurrentDb() const { return currentDb; }
    float getTargetDb()  const { return targetDb; }

    void paint (juce::Graphics& g) override;

private:
    bool  useBallistics = true;

    float targetDb  = 0.0f;   // target GR (positive dB)
    float currentDb = 0.0f;   // smoothed

    float attackCoeffMs  = 22.0f;
    float releaseCoeffMs = 140.0f;

    juce::Path scaleArc;
};
