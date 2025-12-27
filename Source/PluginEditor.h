#pragma once
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "AuricHelpers.h"
#include "GainReductionMeter.h"
#include "AuricLookAndFeel.h"
#include "SegmentedSwitch.h"
#include "PresetManager.h"
#include "AuricKnob.h"   // << WAJIB

//==============================================================================
// Small helpers
static inline juce::String omegaChar()
{
    return juce::String::fromUTF8 (u8"\u03A9");
}

//==============================================================================
// Theme / Style
struct AuricTheme
{
    // Panel / metal
    static juce::Colour panelBase()      { return juce::Colour::fromRGB (18, 18, 18); }
    static juce::Colour panelDark()      { return juce::Colour::fromRGB (12, 12, 12); }
    static juce::Colour panelHighlight() { return juce::Colour::fromRGB (38, 38, 38); }

    // Gold ink / text
    static juce::Colour goldText()       { return juce::Colour::fromRGB (210, 180, 110); }
    static juce::Colour goldTextDim()    { return juce::Colour::fromRGB (150, 130, 90); }

    // Engrave + lines
    static juce::Colour inkLight()       { return juce::Colour::fromRGBA (240, 230, 210, 210); }
    static juce::Colour inkDim()         { return juce::Colour::fromRGBA (240, 230, 210, 120); }
    static juce::Colour shadow()         { return juce::Colour::fromRGBA (0, 0, 0, 190); }

    // Meter amber
    static juce::Colour meterAmberHi()   { return juce::Colour::fromRGB (245, 170, 85); }
    static juce::Colour meterAmberMid()  { return juce::Colour::fromRGB (205, 135, 60); }
    static juce::Colour meterAmberLow()  { return juce::Colour::fromRGB (120, 75, 35); }

    // Button states
    static juce::Colour buttonUp()       { return juce::Colour::fromRGB (28, 28, 28); }
    static juce::Colour buttonDown()     { return juce::Colour::fromRGB (20, 20, 20); }
    static juce::Colour buttonEdge()     { return juce::Colour::fromRGB (70, 60, 40); }
    static juce::Colour buttonActive()   { return juce::Colour::fromRGB (210, 180, 110); }
};

//==============================================================================
// Fonts helper (JUCE terbaru prefer FontOptions)
struct AuricFonts
{
    static juce::Font brand (float h)
    {
        return juce::Font (juce::FontOptions (h).withStyle ("Regular"));
    }

    static juce::Font labelCaps (float h)
    {
        return juce::Font (juce::FontOptions (h).withStyle ("Regular"));
    }

    static juce::Font button (float h)
    {
        return juce::Font (juce::FontOptions (h).withStyle ("Regular"));
    }
};

//==============================================================================
// LED
class LedComponent : public juce::Component
{
public:
    void setOn (bool b) { on = b; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);

        // Dark bezel/rim
        g.setColour (juce::Colour::fromRGB (8, 8, 8));
        g.fillEllipse (r);

        // LED body
        auto ledColor = on ? juce::Colour::fromRGB (0, 180, 255)
                           : juce::Colour::fromRGB (25, 35, 45);

        juce::ColourGradient cg (ledColor.withAlpha (0.95f), r.getCentreX(), r.getY(),
                                 ledColor.darker (0.6f), r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (cg);
        g.fillEllipse (r.reduced (1.5f));

        if (on)
        {
            g.setColour (juce::Colour::fromRGBA (255, 255, 255, 120));
            g.fillEllipse (r.getX() + r.getWidth() * 0.25f,
                           r.getY() + r.getHeight() * 0.2f,
                           r.getWidth() * 0.3f,
                           r.getHeight() * 0.3f);
        }

        g.setColour (juce::Colour::fromRGBA (0, 0, 0, 200));
        g.drawEllipse (r, 1.0f);
    }

private:
    bool on = false;
};

//==============================================================================
// Editor
class AuricOmega76AudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    explicit AuricOmega76AudioProcessorEditor (AuricOmega76AudioProcessor&);
    ~AuricOmega76AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // UI Scale helpers
    void applyUIScale (int idx);

    // LED update helper
    void updateLEDIndicators();

    // Knob UX
    void setupKnobsUx();

    // Background (CACHED)
    void rebuildNoiseTile();
    void rebuildBackground();

private:
    AuricOmega76AudioProcessor& audioProcessor;
    AuricLookAndFeel auricLnf;
    PresetManager presetManager;
    std::unique_ptr<juce::Component> creditsOverlay;

    // UI scale state
    int uiScaleIndex = 1; // 0=S, 1=M, 2=L

    // Background assets
    juce::Image bgCache;
    bool bgDirty = true;
    juce::Image noiseTile;
    juce::Image brushedTile;

    // Header - info button
    juce::TextButton infoButton { "i" };

    // Labels
    juce::Label titleLabel, presetLabel;
    juce::Label inputLabel, releaseLabel, edgeLabel, modeLabel, mixLabel, omegaMixLabel;

    // Preset UI
    juce::ComboBox presetBox;
    juce::TextButton presetSaveButton { "Save" };
    juce::TextButton presetLoadButton { "Load" };
    juce::TextButton presetDeleteButton { "Del" };

    std::unique_ptr<juce::FileChooser> presetChooser;
    std::vector<juce::File> presetFiles;

    // ===== FIX: Knobs are AuricKnob
    AuricKnob inputKnob, releaseKnob, edgeKnob, modeKnob, mixKnob, omegaMixKnob;

    // Buttons
    juce::ToggleButton scHpfButton, pwrButton;

    // Segmented
    SegmentedSwitch3 omegaModeSwitch { "A", "C", juce::String::fromUTF8 (u8"\u03A9") };
    SegmentedSwitch3 routingSwitch   { "A", "D", juce::String::fromUTF8 (u8"\u03A9") };
    SegmentedSwitch3 uiScaleSwitch   { "S", "M", "L" };

    // LEDs
    LedComponent led1, led2;

    // Meter
    GainReductionMeterComponent grMeter;

    // Attachments
    std::unique_ptr<APVTS::SliderAttachment> inputAtt, releaseAtt, edgeAtt, modeAtt, mixAtt, omegaMixAtt;
    std::unique_ptr<APVTS::ButtonAttachment> scHpfAtt, pwrAtt;
    std::unique_ptr<SegmentedSwitchAttachment> omegaModeAtt, routingAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuricOmega76AudioProcessorEditor)
};
