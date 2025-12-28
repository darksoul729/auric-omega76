#pragma once
#include <JuceHeader.h>

//==============================================================================
// SegmentedSwitch3: 3 pilihan (A / C / Ω) + optional legend kiri + optional sublabel bawah
class SegmentedSwitch3 : public juce::Component
{
public:
    SegmentedSwitch3 (juce::String a = "A", juce::String b = "B", juce::String c = "C");

    void setSelectedIndex (int idx);
    int  getSelectedIndex() const { return selected; }

    void setLabels (juce::String a, juce::String b, juce::String c);

    // CLEAN / IRON / GRIT (optional)
    void setSubLabels (juce::String a, juce::String b, juce::String c);

    // Legend kiri: symbol "Ω" dan text "MODE" (optional)
    void setLeftLegend (juce::String symbol, juce::String text, bool enable = true);
    void setDrawLeftLegend (bool enable);
    void setLegendWidth (int px); // 0 = auto

    std::function<void (int)> onChange;
    // hover index: 0..2, -2 = control hover (non-segment), -1 = exit
    std::function<void (int)> onHover;

    // interaction
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

    void paint (juce::Graphics&) override;

private:
    int hitTestIndex (juce::Point<int> p) const;

private:
    juce::String labels[3];
    juce::String subLabels[3];

    int selected = 0;
    int hovered  = -1;
    int pressed  = -1;
    int hoverInfo = -1;

    bool drawLeftLegend = false;
    juce::String leftSymbol, leftText;
    int legendWidthOverride = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegmentedSwitch3)
};

//==============================================================================
// Attachment ke AudioParameterChoice
class SegmentedSwitchAttachment : private juce::AudioProcessorValueTreeState::Listener
{
public:
    SegmentedSwitchAttachment (juce::AudioProcessorValueTreeState& state,
                              const juce::String& paramID,
                              SegmentedSwitch3& control);
    ~SegmentedSwitchAttachment() override;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void pushControlFromParam();
    void pushParamFromControl (int idx);

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String id;
    SegmentedSwitch3& ctrl;

    juce::RangedAudioParameter* param = nullptr;
    juce::AudioParameterChoice* choice = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegmentedSwitchAttachment)
};
