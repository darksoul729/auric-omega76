#pragma once
#include <JuceHeader.h>

//==============================================================================
// Floating value readout tooltip (non-blocking, fades out)
class AuricValueTooltip : public juce::Component,
                          private juce::Timer
{
public:
    AuricValueTooltip();

    void setUiScale (float s);

    void showFor (juce::Component& anchor, const juce::String& text);
    void updateText (const juce::String& text);
    void beginFadeOut();

    bool isShowingFor (const juce::Component* anchor) const;
    bool isActive() const { return isVisible(); }

private:
    void paint (juce::Graphics& g) override;
    void timerCallback() override;
    void updateBounds();

    juce::Component* anchorComp = nullptr;
    juce::String displayText;

    float uiScale = 1.0f;
    float alpha = 1.0f;
    bool fadingOut = false;
    double fadeStartMs = 0.0;
    double fadeDurationMs = 200.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuricValueTooltip)
};
