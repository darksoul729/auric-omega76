#pragma once
#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "AuricHelpers.h"
#include "GainReductionMeter.h"
#include "AuricLookAndFeel.h"
#include "SegmentedSwitch.h"
#include "PresetManager.h"
#include "AuricKnob.h"   // << WAJIB
#include "AuricValueTooltip.h"

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
    static juce::Colour panelBase()      { return juce::Colour::fromRGB (26, 26, 22); }
    static juce::Colour panelDark()      { return juce::Colour::fromRGB (14, 14, 12); }
    static juce::Colour panelHighlight() { return juce::Colour::fromRGB (40, 40, 34); }

    // Gold ink / text
    static juce::Colour goldText()       { return juce::Colour::fromRGB (226, 195, 129); }
    static juce::Colour goldTextDim()    { return juce::Colour::fromRGB (170, 145, 95); }
    static juce::Colour goldTextHi()     { return juce::Colour::fromRGB (251, 247, 175); }

    // Engrave + lines
    static juce::Colour inkLight()       { return juce::Colour::fromRGBA (245, 238, 220, 210); }
    static juce::Colour inkDim()         { return juce::Colour::fromRGBA (245, 238, 220, 120); }
    static juce::Colour shadow()         { return juce::Colour::fromRGBA (0, 0, 0, 190); }

    // Meter amber
    static juce::Colour meterAmberHi()   { return juce::Colour::fromRGB (255, 240, 62); }
    static juce::Colour meterAmberMid()  { return juce::Colour::fromRGB (201, 121, 41); }
    static juce::Colour meterAmberLow()  { return juce::Colour::fromRGB (110, 68, 30); }

    // Button states
    static juce::Colour buttonUp()       { return juce::Colour::fromRGB (28, 28, 24); }
    static juce::Colour buttonDown()     { return juce::Colour::fromRGB (18, 18, 16); }
    static juce::Colour buttonEdge()     { return juce::Colour::fromRGB (74, 64, 42); }
    static juce::Colour buttonActive()   { return juce::Colour::fromRGB (226, 195, 129); }
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
// Simple help line (under meter)
class AuricHelpLine : public juce::Component
{
public:
    AuricHelpLine()
    {
        setInterceptsMouseClicks (false, false);
        setWantsKeyboardFocus (false);
        setVisible (false);
    }

    void setUiScale (float s)
    {
        uiScale = juce::jmax (0.5f, s);
        repaint();
    }

    void setText (juce::String newText)
    {
        text = std::move (newText);
        setVisible (text.isNotEmpty());
        repaint();
    }

    const juce::String& getText() const { return text; }

    void paint (juce::Graphics& g) override
    {
        if (text.isEmpty())
            return;

        auto r = getLocalBounds().toFloat().reduced (1.0f);
        const float rr = juce::jlimit (4.0f, 8.0f, r.getHeight() * 0.45f);

        AuricHelpers::drawInsetWell (g, r, rr, 2.0f * uiScale);

        g.setColour (juce::Colour::fromRGBA (255, 255, 255, 10));
        g.drawRoundedRectangle (r.reduced (0.4f), rr, 1.0f);

        g.setColour (AuricTheme::goldText().withAlpha (0.88f));
        g.setFont (AuricHelpers::makeFont (12.0f * uiScale, juce::Font::plain));
        g.drawFittedText (text, getLocalBounds(), juce::Justification::centred, 1, 0.9f);
    }

private:
    juce::String text;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuricHelpLine)
};

//==============================================================================
// ToggleButton with hover callback + help text
class AuricHelpToggleButton : public juce::ToggleButton
{
public:
    AuricHelpToggleButton() = default;
    
    void setHelpText (juce::String text) { helpText = std::move (text); }
    const juce::String& getHelpText() const { return helpText; }

    std::function<void (AuricHelpToggleButton&, bool)> onHoverChange;

    void mouseEnter (const juce::MouseEvent& e) override
    {
        if (onHoverChange) onHoverChange (*this, true);
        juce::ToggleButton::mouseEnter (e);
    }

    void mouseExit (const juce::MouseEvent& e) override
    {
        if (onHoverChange) onHoverChange (*this, false);
        juce::ToggleButton::mouseExit (e);
    }

private:
    juce::String helpText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuricHelpToggleButton)
};

//==============================================================================
// Layout (DESIGN COORDINATES = screenshot reference)
// 1024 x 684  (match gambar #2)
//==============================================================================
struct LayoutRects
{
    static constexpr float designW = 1024.0f;
    static constexpr float designH = 684.0f;

    float scale = 1.0f;

    juce::Rectangle<float> ui;

    // Header
    juce::Rectangle<float> brand;
    juce::Rectangle<float> presetLabel;
    juce::Rectangle<float> presetBox;
    juce::Rectangle<float> qualityBox;

    juce::Rectangle<float> presetSaveButton;
    juce::Rectangle<float> presetLoadButton;
    juce::Rectangle<float> presetDeleteButton;
    juce::Rectangle<float> infoButton;
    juce::Rectangle<float> uiScaleSwitch;

    float separatorY = 0.0f; // header separator

    // Meter
    juce::Rectangle<float> meterHousing;
    juce::Rectangle<float> meterScreen;
    juce::Rectangle<float> helpLine;

    // Knobs
    juce::Rectangle<float> inputKnob;
    juce::Rectangle<float> releaseKnob;

    juce::Rectangle<float> edgeKnob;
    juce::Rectangle<float> modeKnob;
    juce::Rectangle<float> mixKnob;

    juce::Rectangle<float> omegaMixKnob;

    // Labels
    juce::Rectangle<float> inputLabel;
    juce::Rectangle<float> releaseLabel;
    juce::Rectangle<float> edgeLabel;
    juce::Rectangle<float> modeLabel;
    juce::Rectangle<float> mixLabel;
    juce::Rectangle<float> omegaMixLabel;
    juce::Rectangle<float> toneGroupLabel;
    juce::Rectangle<float> toneGroupPanel;

    // Buttons
    juce::Rectangle<float> scHpfButton;
    juce::Rectangle<float> pwrButton;

    // Switches
    juce::Rectangle<float> omegaModeSwitch;
    juce::Rectangle<float> routingSwitch;

static LayoutRects make (juce::Rectangle<float> outerBounds)
{
    LayoutRects lr;

    const float sx = outerBounds.getWidth()  / designW;
    const float sy = outerBounds.getHeight() / designH;
    lr.scale = juce::jmin (sx, sy);

    const float S = lr.scale;
    const auto ui = outerBounds.withSizeKeepingCentre (designW * S, designH * S);
    lr.ui = ui;

    // HEADER
    lr.brand       = { ui.getX() + 28*S,  ui.getY() + 20*S, 160*S, 32*S };
    lr.presetLabel = { ui.getX() + 285*S, ui.getY() + 22*S,  80*S, 22*S };
    lr.presetBox   = { ui.getX() + 365*S, ui.getY() + 16*S, 300*S, 28*S };
    lr.qualityBox  = { ui.getX() + 690*S, ui.getY() + 16*S, 290*S, 28*S };

    const float btnY = ui.getY() + 52*S;
    lr.presetSaveButton   = { ui.getX() + 460*S, btnY, 50*S, 20*S };
    lr.presetLoadButton   = { ui.getX() + 515*S, btnY, 50*S, 20*S };
    lr.presetDeleteButton = { ui.getX() + 570*S, btnY, 50*S, 20*S };
    lr.infoButton         = { ui.getX() + 630*S, btnY, 24*S, 20*S };
    lr.uiScaleSwitch      = { ui.getX() + 665*S, btnY, 110*S, 20*S };

    lr.separatorY = ui.getY() + 90*S;

    // ============================================================
    // Sizing constants (SLOT sizes) — layout tetap
    const float bigSlotD   = 280*S;
    const float midSlotD   = 140*S;
    const float smallSlotD = 150*S;

    // Knob scale inside slot — ini yang bikin knob mengecil
    const float bigKnobScale   = 0.93f; // coba 0.92–0.95
    const float midKnobScale   = 0.90f; // coba 0.88–0.92
    const float smallKnobScale = 0.90f; // Ω MIX knob: 0.88–0.92

    auto shrinkInSlot = [] (juce::Rectangle<float> slot, float s)
    {
        return slot.withSizeKeepingCentre (slot.getWidth() * s, slot.getHeight() * s);
    };

    const float sideMargin = 14*S;
    const float bigTop     = ui.getY() + 110*S;

    // pakai slot center biar meter tetap align “secara desain”
    const float bigCY      = bigTop + bigSlotD * 0.5f;

    // METER (centered)
    const float meterW = 368*S;
    const float meterH = 240*S;
    lr.meterHousing = { ui.getCentreX() - meterW * 0.5f,
                        bigCY - meterH * 0.5f,
                        meterW, meterH };
    lr.meterScreen  = lr.meterHousing.reduced (20*S, 20*S);
    {
        const float helpH = 16.0f * S;
        lr.helpLine = { lr.meterScreen.getX(),
                        lr.meterScreen.getBottom() + 4.0f * S,
                        lr.meterScreen.getWidth(), helpH };
    }

    // BIG KNOBS (slot tetap, knob diperkecil di tengah slot)
    {
        auto inputSlot   = juce::Rectangle<float> (ui.getX() + sideMargin, bigTop, bigSlotD, bigSlotD);
        auto releaseSlot = juce::Rectangle<float> (ui.getRight() - sideMargin - bigSlotD, bigTop, bigSlotD, bigSlotD);

        lr.inputKnob   = shrinkInSlot (inputSlot,   bigKnobScale);
        lr.releaseKnob = shrinkInSlot (releaseSlot, bigKnobScale);

        // BIG LABELS (pakai slot, biar posisi label gak ikut naik turun)
        const float labelGapBig = 6*S;
        const float labelHBig   = 18*S;
        lr.inputLabel   = { inputSlot.getX(),   inputSlot.getBottom()   + labelGapBig, bigSlotD, labelHBig };
        lr.releaseLabel = { releaseSlot.getX(), releaseSlot.getBottom() + labelGapBig, bigSlotD, labelHBig };
    }

    // BUTTONS (aligned baseline)
    const float hwBtnW = 190*S;
    const float hwBtnH = 32*S;
    const float btnGap = 10*S;
    const float hwBtnY = lr.inputLabel.getBottom() + btnGap;
    lr.scHpfButton = { (ui.getX() + sideMargin + bigSlotD * 0.5f) - hwBtnW * 0.5f, hwBtnY, hwBtnW, hwBtnH };
    lr.pwrButton   = { (ui.getRight() - sideMargin - bigSlotD * 0.5f) - hwBtnW * 0.5f, hwBtnY, hwBtnW, hwBtnH };

    // MID KNOBS (slot tetap, knob diperkecil di tengah slot)
    {
        const float midGap  = 8*S;
        const float midRowW = (midSlotD * 3.0f) + (midGap * 2.0f);
        const float midX    = ui.getCentreX() - midRowW * 0.5f;
        const float midY    = lr.meterHousing.getBottom() + 28*S;

        auto edgeSlot = juce::Rectangle<float> (midX, midY, midSlotD, midSlotD);
        auto modeSlot = juce::Rectangle<float> (midX + (midSlotD + midGap), midY, midSlotD, midSlotD);
        auto mixSlot  = juce::Rectangle<float> (midX + (midSlotD + midGap) * 2.0f, midY, midSlotD, midSlotD);

        lr.edgeKnob = shrinkInSlot (edgeSlot, midKnobScale);
        lr.modeKnob = shrinkInSlot (modeSlot, midKnobScale);
        lr.mixKnob  = shrinkInSlot (mixSlot,  midKnobScale);

        const float labelGapMid = 8*S;
        const float labelHMid   = 18*S;
        lr.edgeLabel = { edgeSlot.getX(), edgeSlot.getBottom() + labelGapMid, midSlotD, labelHMid };
        lr.modeLabel = { modeSlot.getX(), modeSlot.getBottom() + labelGapMid, midSlotD, labelHMid };
        lr.mixLabel  = { mixSlot.getX(),  mixSlot.getBottom()  + labelGapMid, midSlotD, labelHMid };

        const float toneLabelH = 14.0f * S;
        lr.toneGroupLabel = { midX,
                              lr.meterHousing.getBottom() + 6.0f * S,
                              midRowW, toneLabelH };
    }

    // Bottom row: Ω MIX (left), Ω MODE (center), ROUTING (right)
    const float bottomMargin = 24*S;
    const float omegaModeW = 520*S;
    const float omegaModeH = 82*S;
    const float routingW   = 210*S;
    const float routingH   = 62*S;
    const float bottomY    = ui.getBottom() - bottomMargin;

    lr.omegaModeSwitch = { ui.getCentreX() - omegaModeW * 0.5f,
                           bottomY - omegaModeH,
                           omegaModeW, omegaModeH };

    lr.routingSwitch   = { ui.getRight() - sideMargin - routingW,
                           bottomY - routingH,
                           routingW, routingH };

    // Ω MIX label + knob (slot tetap, knob diperkecil di tengah slot)
    {
        const float omegaLabelH   = 16*S;
        const float omegaLabelGap = 6*S;

        lr.omegaMixLabel = { ui.getX() + sideMargin,
                             bottomY - omegaLabelH,
                             smallSlotD, omegaLabelH };

        auto omegaSlot = juce::Rectangle<float> (ui.getX() + sideMargin,
                                                 lr.omegaMixLabel.getY() - omegaLabelGap - smallSlotD,
                                                 smallSlotD, smallSlotD);

    lr.omegaMixKnob = shrinkInSlot (omegaSlot, smallKnobScale);
    }

    // Tone group panel (subtle)
    {
        const float panelTop    = lr.toneGroupLabel.getY() - 6.0f * S;
        const float panelBottom = bottomY + 6.0f * S;
        const float panelLeft   = ui.getX() + sideMargin - 6.0f * S;
        const float panelRight  = ui.getRight() - sideMargin + 6.0f * S;

        lr.toneGroupPanel = { panelLeft,
                              panelTop,
                              panelRight - panelLeft,
                              panelBottom - panelTop };
    }

    return lr;
}


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

    // Knob UX
    void setupKnobsUx();
    juce::String formatKnobValueText (AuricKnob& knob) const;
    void showKnobTooltip (AuricKnob& knob);
    void updateKnobTooltip (AuricKnob& knob);
    void hideKnobTooltip();
    void showHelpText (const juce::String& text);
    void clearHelpText();

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
    juce::Label toneGroupLabel;

    // Preset UI
    juce::ComboBox presetBox;
    juce::ComboBox qualityBox;
    juce::TextButton presetSaveButton { "Save" };
    juce::TextButton presetLoadButton { "Load" };
    juce::TextButton presetDeleteButton { "Del" };

    std::unique_ptr<juce::FileChooser> presetChooser;
    std::vector<juce::File> presetFiles;

    // ===== FIX: Knobs are AuricKnob
    AuricKnob inputKnob, releaseKnob, edgeKnob, modeKnob, mixKnob, omegaMixKnob;

    // Buttons
    AuricHelpToggleButton scHpfButton, pwrButton;

    // Segmented
    SegmentedSwitch3 omegaModeSwitch { "A", "C", juce::String::fromUTF8 (u8"\u03A9") };
    SegmentedSwitch3 routingSwitch   { "A", "D", juce::String::fromUTF8 (u8"\u03A9") };
    SegmentedSwitch3 uiScaleSwitch   { "S", "M", "L" };

    // Meter
    GainReductionMeterComponent grMeter;
    AuricHelpLine helpLine;
    AuricValueTooltip valueTooltip;

    // Help text maps
    juce::String omegaModeHelpText;
    juce::String routingHelpText;
    juce::String omegaModeSegmentHelp[3];
    juce::String routingSegmentHelp[3];

    // Attachments
    std::unique_ptr<APVTS::SliderAttachment> inputAtt, releaseAtt, edgeAtt, modeAtt, mixAtt, omegaMixAtt;
    std::unique_ptr<APVTS::ButtonAttachment> scHpfAtt, pwrAtt;
    std::unique_ptr<SegmentedSwitchAttachment> omegaModeAtt, routingAtt;
    std::unique_ptr<APVTS::ComboBoxAttachment> qualityAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuricOmega76AudioProcessorEditor)
};
