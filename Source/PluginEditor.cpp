//==============================================================================
// PluginEditor.cpp  (AURIC Ω76) — HARDWARE FEEL pass
//  - Background draws ONLY pocket (no double bezel)
//  - GR meter bounds match pocket opening
//  - Pocket trimmed + raised slightly (potong atas + meter naik)
//==============================================================================

#include "PluginEditor.h"
#include "AuricHelpers.h"
#include "AuricKnob.h"

#include <cmath>
#include <memory>

//==============================================================================
// Custom Credits Overlay (no JUCE AlertWindow)
namespace
{
    class CreditsOverlay : public juce::Component,
                           private juce::Button::Listener
    {
    public:
        CreditsOverlay()
        {
            setInterceptsMouseClicks (true, true);

            closeButton.setButtonText ("X");
            closeButton.setWantsKeyboardFocus (false);
            closeButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
            closeButton.addListener (this);
            addAndMakeVisible (closeButton);
        }

        void setText (juce::String title, juce::String body)
        {
            titleText = std::move (title);
            bodyText  = std::move (body);
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            using namespace juce;

            auto b = getLocalBounds().toFloat();

            g.setColour (Colour::fromRGBA (0, 0, 0, 150));
            g.fillRect (b);

            const float panelW = jmin (520.0f, b.getWidth()  * 0.82f);
            const float panelH = jmin (280.0f, b.getHeight() * 0.62f);

            panel = Rectangle<float> (0, 0, panelW, panelH);
            panel.setCentre (b.getCentre());
            panel = panel.reduced (2.0f);

            // Panel fill
            {
                ColourGradient cg (Colour::fromRGB (28, 28, 28), panel.getX(), panel.getY(),
                                   Colour::fromRGB (12, 12, 12), panel.getRight(), panel.getBottom(), false);
                cg.addColour (0.35, Colour::fromRGB (20, 20, 20));
                cg.addColour (0.72, Colour::fromRGB (14, 14, 14));
                g.setGradientFill (cg);
                g.fillRoundedRectangle (panel, 14.0f);
            }

            g.setColour (Colour::fromRGBA (255, 255, 255, 20));
            g.drawRoundedRectangle (panel, 14.0f, 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 180));
            g.drawRoundedRectangle (panel.reduced (0.8f), 13.0f, 1.8f);

            auto inner = panel.reduced (20.0f);
            auto titleArea = inner.removeFromTop (40.0f);

            g.setColour (AuricTheme::goldText().withAlpha (0.92f));
            g.setFont (AuricHelpers::makeFont (20.0f, Font::bold));
            g.drawText (titleText, titleArea, Justification::centredLeft);

            g.setColour (Colour::fromRGBA (255, 255, 255, 12));
            g.drawLine (inner.getX(), titleArea.getBottom() + 6.0f,
                        inner.getRight(), titleArea.getBottom() + 6.0f, 1.0f);

            inner.removeFromTop (16.0f);

            g.setColour (Colour::fromRGBA (230, 230, 230, 210));
            g.setFont (AuricHelpers::makeFont (14.0f, Font::plain));
            g.drawFittedText (bodyText, inner.toNearestInt(), Justification::topLeft, 8, 0.95f);
        }

        void resized() override
        {
            if (panel.isEmpty())
            {
                closeButton.setBounds (getLocalBounds().removeFromTop (1).removeFromRight (1));
                return;
            }

            auto p = panel.toNearestInt();
            auto area = p.removeFromTop (34).removeFromRight (40).reduced (6);
            closeButton.setBounds (area);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (! panel.contains (e.position))
                requestClose();
        }

        std::function<void()> onClose;

    private:
        void buttonClicked (juce::Button*) override { requestClose(); }

        void requestClose()
        {
            if (onClose) onClose();
        }

        juce::TextButton closeButton;
        juce::String titleText { "Auric Ω76 - Credits" };
        juce::String bodyText  { "Auric Omega 76\nDesign/Code: Auric Project\nThanks for using Auric Ω76!" };
        juce::Rectangle<float> panel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CreditsOverlay)
    };

    // IMPORTANT: keep meter pocket math in ONE place (so background + bounds always match)
    static inline juce::Rectangle<float> makeMeterPocketRect (const LayoutRects& lr)
    {
        const float trimTop    = 9.0f * lr.scale;   // potong atas dikit
        const float trimBottom = 3.0f * lr.scale;   // potong bawah dikit
        const float raiseY     = 5.0f * lr.scale;   // naikkan pocket sedikit

        return lr.meterHousing
                .withTrimmedTop (trimTop)
                .withTrimmedBottom (trimBottom)
                .translated (0.0f, -raiseY);
    }
}

//==============================================================================
// Knob UX setup
void AuricOmega76AudioProcessorEditor::setupKnobsUx()
{
    setupAuricKnob (inputKnob, this, 0.0, 220, 3.5f,
                    [] (double v) { return juce::String (v, 1) + " dB"; });
    inputKnob.setName ("input");

    setupAuricKnob (releaseKnob, this, 120.0, 220, 3.5f,
                    [] (double v) { return juce::String (v, 0) + " ms"; });
    releaseKnob.setName ("release");

    setupAuricKnob (edgeKnob, this, 0.0, 200, 3.0f,
                    [] (double v) { return juce::String (v, 1); });
    edgeKnob.setName ("edge");

    setupAuricKnob (modeKnob, this, 0.0, 200, 3.0f,
                    [] (double v) { return juce::String (v, 1); });
    modeKnob.setName ("mode");

    setupAuricKnob (mixKnob, this, 100.0, 200, 3.0f,
                    [] (double v) { return juce::String (v, 0) + " %"; });
    mixKnob.setName ("mix");

    setupAuricKnob (omegaMixKnob, this, 100.0, 200, 3.0f,
                    [] (double v) { return juce::String (v, 0) + " %"; });
    omegaMixKnob.setName ("omega_mix");

    auto hookKnob = [this] (AuricKnob& knob,
                            const juce::String& valueLabel,
                            const juce::String& helpText)
    {
        knob.setValueTextLabel (valueLabel);
        knob.setHelpText (helpText);

        knob.onHoverChange = [this] (AuricKnob& k, bool isHovering)
        {
            if (isHovering)
            {
                showKnobTooltip (k);
                showHelpText (k.getHelpText());
            }
            else
            {
                if (! k.isDraggingByMouse())
                {
                    hideKnobTooltip();
                    clearHelpText();
                }
            }
        };

        knob.onDragStart = [this, &knob]
        {
            showKnobTooltip (knob);
            showHelpText (knob.getHelpText());
        };

        knob.onDragEnd = [this, &knob]
        {
            if (! knob.isMouseOverOrDragging())
            {
                hideKnobTooltip();
                clearHelpText();
            }
        };

        knob.onValueChange = [this, &knob]
        {
            updateKnobTooltip (knob);
        };
    };

    hookKnob (inputKnob,   "INPUT", "Drives compression amount");
    hookKnob (releaseKnob, "RELEASE", "How fast gain recovers");
    hookKnob (edgeKnob,    omegaChar() + juce::String (" EDGE"), "Adds bite / edge harmonics");
    hookKnob (modeKnob,    "MODE", "Mode amount");
    hookKnob (mixKnob,     omegaChar() + juce::String (" MIX"), "Blend harmonic path");
    hookKnob (omegaMixKnob, omegaChar() + juce::String (" MIX"), "Blend harmonic path");
}

juce::String AuricOmega76AudioProcessorEditor::formatKnobValueText (AuricKnob& knob) const
{
    auto label = knob.getValueTextLabel();
    auto value = knob.getTextFromValue (knob.getValue());
    return label.isNotEmpty() ? (label + ": " + value) : value;
}

void AuricOmega76AudioProcessorEditor::showKnobTooltip (AuricKnob& knob)
{
    valueTooltip.showFor (knob, formatKnobValueText (knob));
}

void AuricOmega76AudioProcessorEditor::updateKnobTooltip (AuricKnob& knob)
{
    if (valueTooltip.isShowingFor (&knob))
        valueTooltip.updateText (formatKnobValueText (knob));
}

void AuricOmega76AudioProcessorEditor::hideKnobTooltip() { valueTooltip.beginFadeOut(); }
void AuricOmega76AudioProcessorEditor::showHelpText (const juce::String& text) { helpLine.setText (text); }
void AuricOmega76AudioProcessorEditor::clearHelpText() { helpLine.setText ({ }); }

//==============================================================================
// UI scale
void AuricOmega76AudioProcessorEditor::applyUIScale (int idx)
{
    uiScaleIndex = juce::jlimit (0, 2, idx);

    constexpr float baseW = LayoutRects::designW;
    constexpr float baseH = LayoutRects::designH;
    const float scales[3] = { 0.85f, 1.0f, 1.15f };

    const float s = scales[uiScaleIndex];
    setSize ((int) std::round (baseW * s), (int) std::round (baseH * s));

    auricLnf.uiScaleIndex = uiScaleIndex;
    auricLnf.uiScale = s;

    resized();
    bgDirty = true;
    repaint();
}

//==============================================================================
// Noise tile
void AuricOmega76AudioProcessorEditor::rebuildNoiseTile()
{
    constexpr int W = 192;
    constexpr int H = 192;

    noiseTile = juce::Image (juce::Image::RGB, W, H, true);
    juce::Random rng (0xA0C176u);

    auto clampU8 = [] (int v) -> juce::uint8 { return (juce::uint8) juce::jlimit (0, 255, v); };

    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            const float g1 = rng.nextFloat();
            const float g2 = rng.nextFloat();
            const float fine = (g1 * 0.6f + g2 * 0.4f);

            const int hx = (x / 8);
            const int hy = (y / 8);
            const int h  = (hx * 73856093) ^ (hy * 19349663) ^ 0x5bd1e995;
            const float blotch = (float) ((h & 255) / 255.0f);

            const float v = 12.0f + fine * 14.0f + blotch * 2.0f;
            const int vi = (int) std::round (v);

            noiseTile.setPixelAt (x, y, juce::Colour::fromRGB (clampU8 (vi),
                                                               clampU8 (vi),
                                                               clampU8 (vi)));
        }
}

//==============================================================================
// Background cache render
void AuricOmega76AudioProcessorEditor::rebuildBackground()
{
    using namespace juce;

    const int W = jmax (1, getWidth());
    const int H = jmax (1, getHeight());

    bgCache = Image (Image::RGB, W, H, true);
    Graphics g (bgCache);
    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    const Rectangle<float> bounds (0.0f, 0.0f, (float) W, (float) H);
    const auto lr = LayoutRects::make (bounds);

    g.fillAll (AuricTheme::panelDark());

    // Base panel gradient
    {
        ColourGradient base (AuricTheme::panelHighlight().withAlpha (0.96f),
                             lr.ui.getCentreX(), lr.ui.getY(),
                             AuricTheme::panelDark().darker (0.28f),
                             lr.ui.getCentreX(), lr.ui.getBottom(), false);
        base.addColour (0.45, AuricTheme::panelBase());
        g.setGradientFill (base);
        g.fillRect (lr.ui);
    }

    // Micro texture (low)
    if (noiseTile.isValid())
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (lr.ui.toNearestInt());

        auto xf = AffineTransform::scale (1.65f, 1.65f);
        g.setFillType (FillType (noiseTile, xf));
        g.setOpacity (0.055f);
        g.fillRect (lr.ui);
        g.setOpacity (1.0f);
    }

    // Soft top highlight
    {
        auto top = lr.ui.withHeight (lr.ui.getHeight() * 0.18f);
        ColourGradient wash (Colour::fromRGBA (255, 232, 190, 16),
                             top.getCentreX(), top.getY(),
                             Colour::fromRGBA (0, 0, 0, 0),
                             top.getCentreX(), top.getBottom(), false);
        wash.addColour (0.55, Colour::fromRGBA (255, 232, 190, 6));
        g.setGradientFill (wash);
        g.fillRect (top);
    }

    // Bottom tint
    {
        auto bottom = lr.ui.withTrimmedTop (lr.separatorY - lr.ui.getY());
        ColourGradient bot (AuricTheme::panelBase().darker (0.10f),
                            bottom.getCentreX(), bottom.getY(),
                            AuricTheme::panelDark().darker (0.38f),
                            bottom.getCentreX(), bottom.getBottom(), false);
        bot.addColour (0.45, AuricTheme::panelBase().darker (0.20f));
        g.setGradientFill (bot);
        g.fillRect (bottom);
    }

    // Tone group panel
    {
        auto panel = lr.toneGroupPanel;
        const float rr = 12.0f * lr.scale;

        ColourGradient tg (AuricTheme::panelHighlight().withAlpha (0.11f),
                           panel.getCentreX(), panel.getY(),
                           AuricTheme::panelDark().withAlpha (0.16f),
                           panel.getCentreX(), panel.getBottom(), false);
        tg.addColour (0.55, AuricTheme::panelBase().withAlpha (0.10f));
        g.setGradientFill (tg);
        g.fillRoundedRectangle (panel, rr);

        g.setColour (Colour::fromRGBA (255, 255, 255, 9));
        g.drawRoundedRectangle (panel.reduced (0.6f), rr, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 125));
        g.drawRoundedRectangle (panel.reduced (1.6f), rr - 1.0f, 1.2f);
    }

    // Panel separator (bevel)
    {
        const float x1 = lr.ui.getX() + 24.0f * lr.scale;
        const float x2 = lr.ui.getRight() - 24.0f * lr.scale;

        g.setColour (Colour::fromRGBA (255, 255, 255, 12));
        g.drawLine (x1, lr.separatorY, x2, lr.separatorY, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 195));
        g.drawLine (x1, lr.separatorY + 1.3f * lr.scale,
                    x2, lr.separatorY + 1.3f * lr.scale, 1.3f);
    }

    //==============================================================================
    // METER POCKET ONLY (trim + raise)
    //==============================================================================
    {
        auto pocket = makeMeterPocketRect (lr);
        const float rr = 14.0f * lr.scale;

        g.setColour (Colour::fromRGBA (0, 0, 0, 185));
        g.fillRoundedRectangle (pocket.expanded (3.0f * lr.scale)
                                .translated (0.0f, 2.2f * lr.scale),
                                rr + 3.2f * lr.scale);

        AuricHelpers::drawInsetWell (g, pocket, rr, 4.0f * lr.scale);

        g.setColour (Colour::fromRGBA (255, 255, 255, 10));
        g.drawRoundedRectangle (pocket.reduced (0.6f), rr - 0.6f, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 200));
        g.drawRoundedRectangle (pocket.reduced (1.5f), rr - 1.2f, 1.2f);

        const float y = pocket.getY() + 0.9f * lr.scale;
        g.setColour (AuricTheme::goldTextHi().withAlpha (0.10f));
        g.drawLine (pocket.getX() + 10.0f * lr.scale, y,
                    pocket.getRight() - 10.0f * lr.scale, y, 1.0f);
    }



    // Frame + vignette/noise
    g.setColour (Colour::fromRGBA (0, 0, 0, 195));
    g.drawRect (lr.ui.toNearestInt(), 2);

    AuricHelpers::drawVignetteNoiseOverlay (g, lr.ui, 0.32f);

    bgDirty = false;
}

//==============================================================================
// Ctor / Dtor
AuricOmega76AudioProcessorEditor::AuricOmega76AudioProcessorEditor (AuricOmega76AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), presetManager (p)
{
    setLookAndFeel (&auricLnf);
    rebuildNoiseTile();

    // Labels
    titleLabel.setText (AuricHelpers::trackCaps ("AURIC"), juce::dontSendNotification);
    presetLabel.setText (AuricHelpers::trackCaps ("PRESET"), juce::dontSendNotification);

    inputLabel.setText (AuricHelpers::trackCaps ("INPUT"), juce::dontSendNotification);
    releaseLabel.setText (AuricHelpers::trackCaps ("RELEASE"), juce::dontSendNotification);

    edgeLabel.setText (omegaChar() + " " + AuricHelpers::trackCaps ("EDGE"), juce::dontSendNotification);
    modeLabel.setText (AuricHelpers::trackCaps ("MODE"), juce::dontSendNotification);
    mixLabel .setText (omegaChar() + " " + AuricHelpers::trackCaps ("MIX"), juce::dontSendNotification);
    omegaMixLabel.setText (omegaChar() + " " + AuricHelpers::trackCaps ("MIX"), juce::dontSendNotification);

    toneGroupLabel.setText ("TONE / CHARACTER", juce::dontSendNotification);

    AuricHelpers::styleLabelGold (titleLabel, 24.0f, true, false, 2.0f);
    titleLabel.setColour (juce::Label::textColourId, AuricTheme::goldTextHi().withAlpha (0.95f));

    AuricHelpers::styleLabelGold (presetLabel, 13.0f, false, false, 1.1f);
    AuricHelpers::styleLabelGold (inputLabel,  13.5f, false, true,  0.9f);
    AuricHelpers::styleLabelGold (releaseLabel,13.5f, false, true,  0.9f);
    AuricHelpers::styleLabelGold (edgeLabel,   12.0f, false, true,  0.8f);
    AuricHelpers::styleLabelGold (modeLabel,   12.0f, false, true,  0.8f);
    AuricHelpers::styleLabelGold (mixLabel,    12.0f, false, true,  0.8f);
    AuricHelpers::styleLabelGold (omegaMixLabel,12.0f,false, true,  0.8f);
    AuricHelpers::styleLabelGold (toneGroupLabel,11.0f,false, true,  1.0f);

    inputLabel.setColour (juce::Label::textColourId, AuricTheme::goldTextHi().withAlpha (0.96f));
    releaseLabel.setColour (juce::Label::textColourId, AuricTheme::goldTextHi().withAlpha (0.96f));
    toneGroupLabel.setColour (juce::Label::textColourId, AuricTheme::goldTextDim().withAlpha (0.86f));

    inputLabel.setJustificationType (juce::Justification::centred);
    releaseLabel.setJustificationType (juce::Justification::centred);
    edgeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setJustificationType (juce::Justification::centred);
    mixLabel.setJustificationType (juce::Justification::centred);
    omegaMixLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (presetLabel);
    addAndMakeVisible (inputLabel);
    addAndMakeVisible (releaseLabel);
    addAndMakeVisible (edgeLabel);
    addAndMakeVisible (modeLabel);
    addAndMakeVisible (mixLabel);
    addAndMakeVisible (omegaMixLabel);
    addAndMakeVisible (toneGroupLabel);

    // Segmented
    omegaModeSwitch.setLeftLegend (juce::String::fromUTF8 (u8"\u03A9"), "MODE", true);
    omegaModeSwitch.setSubLabels ("CLEAN", "IRON", "GRIT");

    routingSwitch.setDrawLeftLegend (false);
    routingSwitch.setSubLabels ("A", "D", juce::String::fromUTF8 (u8"\u03A9"));

    omegaModeHelpText = "Tone character";
    omegaModeSegmentHelp[0] = "Clean";
    omegaModeSegmentHelp[1] = "Iron";
    omegaModeSegmentHelp[2] = "Grit";

    routingHelpText = "Routing mode";
    routingSegmentHelp[0] = "Analog path";
    routingSegmentHelp[1] = "Digital path";
    routingSegmentHelp[2] = omegaChar() + " path";

    omegaModeSwitch.onHover = [this] (int idx)
    {
        if (idx == -1) { clearHelpText(); return; }
        if (idx >= 0 && idx < 3 && omegaModeSegmentHelp[idx].isNotEmpty()) { showHelpText (omegaModeSegmentHelp[idx]); return; }
        showHelpText (omegaModeHelpText);
    };

    routingSwitch.onHover = [this] (int idx)
    {
        if (idx == -1) { clearHelpText(); return; }
        if (idx >= 0 && idx < 3 && routingSegmentHelp[idx].isNotEmpty()) { showHelpText (routingSegmentHelp[idx]); return; }
        showHelpText (routingHelpText);
    };

    // Preset UI
    presetBox.setTextWhenNothingSelected ("DEFAULT");
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.setColour (juce::ComboBox::textColourId, AuricTheme::goldText().withAlpha (0.88f));
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x00000000));
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x00000000));
    presetBox.setColour (juce::ComboBox::buttonColourId, juce::Colour (0x00000000));
    presetBox.setComponentID ("hdr_preset_box");
    addAndMakeVisible (presetBox);

    qualityBox.setTextWhenNothingSelected (omegaChar() + " Auto");
    qualityBox.setJustificationType (juce::Justification::centredLeft);
    qualityBox.setColour (juce::ComboBox::textColourId, AuricTheme::goldText().withAlpha (0.88f));
    qualityBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x00000000));
    qualityBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x00000000));
    qualityBox.setColour (juce::ComboBox::buttonColourId, juce::Colour (0x00000000));
    qualityBox.setComponentID ("hdr_quality_box");

    qualityBox.addItem (omegaChar() + " Auto", 1);
    qualityBox.addItem ("O1 x1", 2);
    qualityBox.addItem ("O3 x2", 3);
    qualityBox.addItem ("O3 x4", 4);
    qualityBox.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (qualityBox);

    addAndMakeVisible (presetSaveButton);
    addAndMakeVisible (presetLoadButton);
    addAndMakeVisible (presetDeleteButton);

    presetSaveButton.setComponentID   ("hdr_btn");
    presetLoadButton.setComponentID   ("hdr_btn");
    presetDeleteButton.setComponentID ("hdr_btn");

    // Preset logic
    presetManager.ensurePresetDirectory();
    presetManager.rebuildPresetMenu (presetBox, presetFiles);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetManager.updatePresetButtonsEnabled (presetBox, presetDeleteButton);

    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id == 1) presetManager.loadFactoryDefault();
        else
        {
            const int idx = id - 2;
            if (idx >= 0 && idx < (int) presetFiles.size())
                presetManager.loadPresetFile (presetFiles[(size_t) idx]);
        }

        presetManager.updatePresetButtonsEnabled (presetBox, presetDeleteButton);
        bgDirty = true;
        repaint();
    };

    presetSaveButton.onClick = [this]
    {
        presetManager.ensurePresetDirectory();
        auto start = presetManager.getPresetDirectory().getChildFile ("My Preset.xml");

        presetChooser = std::make_unique<juce::FileChooser> ("Save Preset", start, "*.xml");
        presetChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                    | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == juce::File()) { presetChooser.reset(); return; }

                if (file.getFileExtension().isEmpty())
                    file = file.withFileExtension (".xml");

                auto state = audioProcessor.apvts.copyState();
                if (auto xml = state.createXml())
                    xml->writeTo (file, {});

                presetManager.rebuildPresetMenu (presetBox, presetFiles);
                for (int i = 0; i < (int) presetFiles.size(); ++i)
                    if (presetFiles[(size_t) i] == file)
                        presetBox.setSelectedId (i + 2, juce::dontSendNotification);

                presetManager.updatePresetButtonsEnabled (presetBox, presetDeleteButton);
                presetChooser.reset();

                bgDirty = true;
                repaint();
            });
    };

    presetLoadButton.onClick = [this]
    {
        presetManager.ensurePresetDirectory();
        presetChooser = std::make_unique<juce::FileChooser> ("Load Preset",
                                                             presetManager.getPresetDirectory(), "*.xml");
        presetChooser->launchAsync (juce::FileBrowserComponent::openMode
                                    | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    presetManager.loadPresetFile (file);
                    presetManager.rebuildPresetMenu (presetBox, presetFiles);
                    for (int i = 0; i < (int) presetFiles.size(); ++i)
                        if (presetFiles[(size_t) i] == file)
                            presetBox.setSelectedId (i + 2, juce::dontSendNotification);
                }

                presetManager.updatePresetButtonsEnabled (presetBox, presetDeleteButton);
                presetChooser.reset();

                bgDirty = true;
                repaint();
            });
    };

    presetDeleteButton.onClick = [this]
    {
        presetManager.deleteSelectedPreset (presetBox, presetFiles, this);
        bgDirty = true;
        repaint();
    };

    // Knobs
    AuricHelpers::styleKnobAuric (inputKnob);
    AuricHelpers::styleKnobAuric (releaseKnob);
    AuricHelpers::styleKnobAuric (edgeKnob);
    AuricHelpers::styleKnobAuric (modeKnob);
    AuricHelpers::styleKnobAuric (mixKnob);
    AuricHelpers::styleKnobAuric (omegaMixKnob);

    addAndMakeVisible (inputKnob);
    addAndMakeVisible (releaseKnob);
    addAndMakeVisible (edgeKnob);
    addAndMakeVisible (modeKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (omegaMixKnob);

    setupKnobsUx();

    // Buttons (hardware)
    scHpfButton.setButtonText ("SC HPF");
    pwrButton.setButtonText ("PWR");
    scHpfButton.setClickingTogglesState (true);
    pwrButton.setClickingTogglesState (true);
    scHpfButton.setComponentID ("hw_sc_hpf");
    pwrButton.setComponentID   ("hw_pwr");
    scHpfButton.setWantsKeyboardFocus (false);
    pwrButton.setWantsKeyboardFocus   (false);

    scHpfButton.setHelpText ("Filters sidechain low end");
    pwrButton.setHelpText ("Power / bypass");

    scHpfButton.onHoverChange = [this] (AuricHelpToggleButton& b, bool isHovering)
    {
        if (isHovering) showHelpText (b.getHelpText());
        else            clearHelpText();
    };
    pwrButton.onHoverChange = [this] (AuricHelpToggleButton& b, bool isHovering)
    {
        if (isHovering) showHelpText (b.getHelpText());
        else            clearHelpText();
    };

    addAndMakeVisible (scHpfButton);
    addAndMakeVisible (pwrButton);

    // Info button + overlay
    infoButton.setButtonText ("i");
    infoButton.setComponentID ("hdr_btn");
    infoButton.setWantsKeyboardFocus (false);
    addAndMakeVisible (infoButton);

    creditsOverlay = std::make_unique<CreditsOverlay>();
    if (auto* co = static_cast<CreditsOverlay*> (creditsOverlay.get()))
    {
        co->setVisible (false);
        co->setAlwaysOnTop (true);
        co->setText ("Auric Ω76 - Credits",
                     "Auric Omega 76\n"
                     "Design/Code: Auric Project\n"
                     "Thanks for using Auric Ω76!");
        co->onClose = [this]
        {
            if (creditsOverlay)
                creditsOverlay->setVisible (false);
            repaint();
        };
        addChildComponent (*co);
    }

    infoButton.onClick = [this]
    {
        if (! creditsOverlay) return;

        const bool show = ! creditsOverlay->isVisible();
        creditsOverlay->setBounds (getLocalBounds());
        creditsOverlay->setVisible (show);

        if (show)
            creditsOverlay->toFront (true);

        repaint();
    };

    // Segmented + scale switch
    addAndMakeVisible (omegaModeSwitch);
    addAndMakeVisible (routingSwitch);

    addAndMakeVisible (uiScaleSwitch);
    uiScaleSwitch.onChange = [this] (int idx) { applyUIScale (idx); };

    // Meter
    grMeter.setUseBallistics (true);
    grMeter.setBallisticsMs (18.0f, 180.0f);
    addAndMakeVisible (grMeter);

    addAndMakeVisible (helpLine);
    addChildComponent (valueTooltip);

    // Attachments
    auto& apvts = audioProcessor.apvts;
    inputAtt    = std::make_unique<APVTS::SliderAttachment> (apvts, "input",   inputKnob);
    releaseAtt  = std::make_unique<APVTS::SliderAttachment> (apvts, "release", releaseKnob);
    edgeAtt     = std::make_unique<APVTS::SliderAttachment> (apvts, "edge",    edgeKnob);
    modeAtt     = std::make_unique<APVTS::SliderAttachment> (apvts, "mode",    modeKnob);
    mixAtt      = std::make_unique<APVTS::SliderAttachment> (apvts, "mix",     mixKnob);
    omegaMixAtt = std::make_unique<APVTS::SliderAttachment> (apvts, "omega_mix", omegaMixKnob);

    scHpfAtt    = std::make_unique<APVTS::ButtonAttachment> (apvts, "sc_hpf", scHpfButton);
    pwrAtt      = std::make_unique<APVTS::ButtonAttachment> (apvts, "pwr",    pwrButton);

    omegaModeAtt = std::make_unique<SegmentedSwitchAttachment> (apvts, "omega_mode", omegaModeSwitch);
    routingAtt   = std::make_unique<SegmentedSwitchAttachment> (apvts, "routing",    routingSwitch);
    qualityAtt   = std::make_unique<APVTS::ComboBoxAttachment> (apvts, "quality", qualityBox);

    // Resizable
    setResizable (true, true);
    {
        const float scales[3] = { 0.85f, 1.0f, 1.15f };
        setResizeLimits ((int) std::round (LayoutRects::designW * scales[0]),
                         (int) std::round (LayoutRects::designH * scales[0]),
                         (int) std::round (LayoutRects::designW * scales[2]),
                         (int) std::round (LayoutRects::designH * scales[2]));
    }

    uiScaleSwitch.setSelectedIndex (1);
    applyUIScale (1);

    startTimerHz (60);
}

AuricOmega76AudioProcessorEditor::~AuricOmega76AudioProcessorEditor()
{
    stopTimer();
    presetChooser.reset();
    creditsOverlay.reset();
    setLookAndFeel (nullptr);
}

//==============================================================================
// Timer
void AuricOmega76AudioProcessorEditor::timerCallback()
{
    const float gr = audioProcessor.getGainReductionDb();
    grMeter.setGainReductionDb (gr);
    grMeter.tick (60.0);
}

//==============================================================================
// Paint
void AuricOmega76AudioProcessorEditor::paint (juce::Graphics& g)
{
    if (bgDirty || ! bgCache.isValid()
        || bgCache.getWidth()  != getWidth()
        || bgCache.getHeight() != getHeight())
    {
        rebuildBackground();
    }

    g.drawImageAt (bgCache, 0, 0);
}

//==============================================================================
// Layout
void AuricOmega76AudioProcessorEditor::resized()
{
    using namespace juce;

    auto lr = LayoutRects::make (getLocalBounds().toFloat());
    auricLnf.uiScale = lr.scale;

    auto toInt = [] (Rectangle<float> r) { return r.toNearestInt(); };

    // Header
    titleLabel.setBounds (toInt (lr.brand));
    presetLabel.setBounds (toInt (lr.presetLabel));
    presetBox.setBounds (toInt (lr.presetBox));
    qualityBox.setBounds (toInt (lr.qualityBox));

    presetSaveButton.setBounds (toInt (lr.presetSaveButton));
    presetLoadButton.setBounds (toInt (lr.presetLoadButton));
    presetDeleteButton.setBounds (toInt (lr.presetDeleteButton));
    infoButton.setBounds (toInt (lr.infoButton));
    uiScaleSwitch.setBounds (toInt (lr.uiScaleSwitch));

    // Main row
    inputKnob.setBounds (toInt (lr.inputKnob));
    releaseKnob.setBounds (toInt (lr.releaseKnob));

    // Meter bounds MUST match pocket opening
    grMeter.setBounds (makeMeterPocketRect (lr).toNearestInt());

    helpLine.setBounds (toInt (lr.helpLine));

    inputLabel.setBounds (toInt (lr.inputLabel));
    releaseLabel.setBounds (toInt (lr.releaseLabel));

    scHpfButton.setBounds (toInt (lr.scHpfButton));
    pwrButton.setBounds (toInt (lr.pwrButton));

    // Mid knobs
    edgeKnob.setBounds (toInt (lr.edgeKnob));
    modeKnob.setBounds (toInt (lr.modeKnob));
    mixKnob.setBounds  (toInt (lr.mixKnob));

    edgeLabel.setBounds (toInt (lr.edgeLabel));
    modeLabel.setBounds (toInt (lr.modeLabel));
    mixLabel.setBounds  (toInt (lr.mixLabel));
    toneGroupLabel.setBounds (toInt (lr.toneGroupLabel));

    // Bottom
    omegaMixKnob.setBounds (toInt (lr.omegaMixKnob));
    omegaMixLabel.setBounds (toInt (lr.omegaMixLabel));

    omegaModeSwitch.setBounds (toInt (lr.omegaModeSwitch));
    routingSwitch.setBounds (toInt (lr.routingSwitch));

    // Scale fonts
    AuricHelpers::styleLabelGold (titleLabel, 24.0f * lr.scale, true, false, 2.0f * lr.scale);
    titleLabel.setColour (Label::textColourId, AuricTheme::goldTextHi().withAlpha (0.95f));

    AuricHelpers::styleLabelGold (presetLabel,  13.0f * lr.scale, false, false, 1.1f * lr.scale);
    AuricHelpers::styleLabelGold (inputLabel,   13.5f * lr.scale, false, true,  0.9f * lr.scale);
    AuricHelpers::styleLabelGold (releaseLabel, 13.5f * lr.scale, false, true,  0.9f * lr.scale);
    AuricHelpers::styleLabelGold (edgeLabel,    12.0f * lr.scale, false, true,  0.8f * lr.scale);
    AuricHelpers::styleLabelGold (modeLabel,    12.0f * lr.scale, false, true,  0.8f * lr.scale);
    AuricHelpers::styleLabelGold (mixLabel,     12.0f * lr.scale, false, true,  0.8f * lr.scale);
    AuricHelpers::styleLabelGold (omegaMixLabel,11.0f * lr.scale, false, true,  0.8f * lr.scale);
    AuricHelpers::styleLabelGold (toneGroupLabel,11.0f * lr.scale,false, true,  1.0f * lr.scale);

    inputLabel.setColour (Label::textColourId, AuricTheme::goldTextHi().withAlpha (0.96f));
    releaseLabel.setColour (Label::textColourId, AuricTheme::goldTextHi().withAlpha (0.96f));
    toneGroupLabel.setColour (Label::textColourId, AuricTheme::goldTextDim().withAlpha (0.86f));

    helpLine.setUiScale (lr.scale);
    valueTooltip.setUiScale (lr.scale);

    if (creditsOverlay)
        creditsOverlay->setBounds (getLocalBounds());

    bgDirty = true;
}
