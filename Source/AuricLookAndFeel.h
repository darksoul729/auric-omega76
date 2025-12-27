#pragma once
#include <JuceHeader.h>

class AuricLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AuricLookAndFeel();
    ~AuricLookAndFeel() override = default;

    // biar PluginEditor kamu gak error lagi
    // 0=S, 1=M, 2=L
    int uiScaleIndex = 1;

    // Sliders
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    // Popup bubble styling (value popup)
    juce::Font getSliderPopupFont (juce::Slider&) override;
    int getSliderPopupPlacement (juce::Slider&) override;

    // NOTE: signature JUCE bisa beda antar versi.
    // Kalau compiler komplain, lihat autocomplete di IDE, samakan signature-nya.
    void drawBubble (juce::Graphics& g,
                     juce::BubbleComponent& bubble,
                     const juce::Point<float>& tip,
                     const juce::Rectangle<float>& body) override;

    // Buttons
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool highlighted, bool down) override;

    juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool highlighted, bool down) override;

    // ComboBox
    void drawComboBox (juce::Graphics& g, int width, int height,
                       bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    // Labels (no box)
    void drawLabel (juce::Graphics& g, juce::Label& label) override;
};
