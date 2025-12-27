//==============================================================================
// PluginEditor.cpp
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

            // Dim background
            g.setColour (Colour::fromRGBA (0, 0, 0, 150));
            g.fillRect (b);

            // Center panel
            const float panelW = jmin (520.0f, b.getWidth()  * 0.82f);
            const float panelH = jmin (280.0f, b.getHeight() * 0.62f);

            panel = Rectangle<float> (0, 0, panelW, panelH);
            panel.setCentre (b.getCentre());
            panel = panel.reduced (2.0f);

            // Panel fill
            {
                ColourGradient cg (Colour::fromRGB (28, 28, 28), panel.getX(), panel.getY(),
                                   Colour::fromRGB (12, 12, 12), panel.getRight(), panel.getBottom(),
                                   false);
                cg.addColour (0.35, Colour::fromRGB (20, 20, 20));
                cg.addColour (0.72, Colour::fromRGB (14, 14, 14));
                g.setGradientFill (cg);
                g.fillRoundedRectangle (panel, 14.0f);
            }

            // Border
            g.setColour (Colour::fromRGBA (255, 255, 255, 20));
            g.drawRoundedRectangle (panel, 14.0f, 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 180));
            g.drawRoundedRectangle (panel.reduced (0.8f), 13.0f, 1.8f);

            // Title
            auto inner = panel.reduced (20.0f);
            auto titleArea = inner.removeFromTop (40.0f);

            g.setColour (AuricTheme::goldText().withAlpha (0.92f));
            g.setFont (AuricHelpers::makeFont (20.0f, juce::Font::bold));
            g.drawText (titleText, titleArea, Justification::centredLeft);

            // Divider
            g.setColour (Colour::fromRGBA (255, 255, 255, 12));
            g.drawLine (inner.getX(), titleArea.getBottom() + 6.0f,
                        inner.getRight(), titleArea.getBottom() + 6.0f, 1.0f);

            inner.removeFromTop (16.0f);

            // Body
            g.setColour (Colour::fromRGBA (230, 230, 230, 210));
            g.setFont (AuricHelpers::makeFont (14.0f, juce::Font::plain));
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
        void buttonClicked (juce::Button*) override
        {
            requestClose();
        }

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
}

//==============================================================================
// UI scale
void AuricOmega76AudioProcessorEditor::applyUIScale (int idx)
{
    uiScaleIndex = juce::jlimit (0, 2, idx);

    struct S { int w, h; };
    const S sizes[3] =
    {
        {  920, 510 },  // S
        { 1040, 570 },  // M
        { 1120, 620 }   // L
    };

    setSize (sizes[uiScaleIndex].w, sizes[uiScaleIndex].h);
    auricLnf.uiScaleIndex = uiScaleIndex;

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

    auto clampU8 = [] (int v) -> juce::uint8
    {
        return (juce::uint8) juce::jlimit (0, 255, v);
    };

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
// Background cache render (matte hardware + screws + meter bevel)
void AuricOmega76AudioProcessorEditor::rebuildBackground()
{
    using namespace juce;

    const int W = jmax (1, getWidth());
    const int H = jmax (1, getHeight());

    bgCache = Image (Image::RGB, W, H, true);
    Graphics g (bgCache);
    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    const Rectangle<float> r (0.0f, 0.0f, (float) W, (float) H);

    auto addBrushed = [&] (Rectangle<float> rr, float alpha, float stepPx, bool horizontal)
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (rr.toNearestInt());

        Random rng (0xC0FFEEu);
        g.setColour (Colour::fromRGBA (255, 255, 255,
                                       (uint8) jlimit (0, 255, (int) (alpha * 255.0f))));

        if (horizontal)
        {
            for (float y = rr.getY() + 1.0f; y < rr.getBottom() - 1.0f; y += stepPx)
            {
                const float wob = 0.30f * std::sin (y * 0.22f) + (rng.nextFloat() - 0.5f) * 0.18f;
                g.drawLine (rr.getX() + 1.0f, y + wob, rr.getRight() - 1.0f, y + wob, 1.0f);
            }
        }
        else
        {
            for (float x = rr.getX() + 1.0f; x < rr.getRight() - 1.0f; x += stepPx)
            {
                const float wob = 0.30f * std::sin (x * 0.22f) + (rng.nextFloat() - 0.5f) * 0.18f;
                g.drawLine (x + wob, rr.getY() + 1.0f, x + wob, rr.getBottom() - 1.0f, 1.0f);
            }
        }
    };

    auto vignette = [&] (Rectangle<float> rr, float strength)
    {
        Path p; p.addRectangle (rr);

        ColourGradient vg (Colour::fromRGBA (0, 0, 0, 0),
                           rr.getCentreX(), rr.getCentreY(),
                           Colour::fromRGBA (0, 0, 0, (uint8) (140.0f * strength)),
                           rr.getCentreX(), rr.getCentreY(),
                           true);

        vg.addColour (0.62, Colour::fromRGBA (0, 0, 0, 0));
        vg.addColour (1.00, Colour::fromRGBA (0, 0, 0, (uint8) (175.0f * strength)));

        g.setGradientFill (vg);
        g.fillPath (p);
    };

    auto drawScrew = [&] (Point<float> c, float rad)
    {
        g.setColour (Colour::fromRGBA (0,0,0,170));
        g.fillEllipse (c.x - rad - 1.2f, c.y - rad + 0.8f, (rad + 1.2f) * 2.0f, (rad + 1.2f) * 2.0f);

        ColourGradient cg (Colour::fromRGB (26,26,26), c.x - rad, c.y - rad,
                           Colour::fromRGB (8,8,8),   c.x + rad, c.y + rad, false);
        cg.addColour (0.55, Colour::fromRGB (14,14,14));
        g.setGradientFill (cg);
        g.fillEllipse (c.x - rad, c.y - rad, rad * 2.0f, rad * 2.0f);

        g.setColour (Colour::fromRGBA (255,255,255,16));
        g.drawEllipse (c.x - rad + 0.6f, c.y - rad + 0.6f, (rad - 0.6f) * 2.0f, (rad - 0.6f) * 2.0f, 1.0f);

        g.setColour (Colour::fromRGBA (0,0,0,200));
        g.drawLine (c.x - rad * 0.55f, c.y, c.x + rad * 0.55f, c.y, 2.0f);
        g.setColour (Colour::fromRGBA (255,255,255,10));
        g.drawLine (c.x - rad * 0.50f, c.y - 0.6f, c.x + rad * 0.50f, c.y - 0.6f, 1.0f);
    };

    // 1) BASE MATTE BLACK METAL
    {
        g.fillAll (Colour::fromRGB (10, 10, 10));

        ColourGradient baseG (Colour::fromRGB (22, 21, 20), r.getX(), r.getY(),
                              Colour::fromRGB (7,  7,  7),  r.getRight(), r.getBottom(),
                              false);
        baseG.addColour (0.38, Colour::fromRGB (14, 14, 14));
        baseG.addColour (0.72, Colour::fromRGB (9,  9,  9));
        g.setGradientFill (baseG);
        g.fillRect (r);

        // header wash
        Rectangle<float> hdr = r.withHeight (r.getHeight() * 0.14f);
        ColourGradient wash (Colour::fromRGBA (255, 210, 120, 10),
                             hdr.getCentreX(), hdr.getY(),
                             Colour::fromRGBA (0, 0, 0, 0),
                             hdr.getCentreX(), hdr.getBottom(),
                             false);
        wash.addColour (0.55, Colour::fromRGBA (255, 210, 120, 4));
        g.setGradientFill (wash);
        g.fillRect (hdr);

        addBrushed (r.reduced (2.0f), 0.012f, 6.5f, true);
        vignette (r, 0.45f);
    }

    // 2) NOISE overlay skip meter area
    {
        const Rectangle<int> meterI = grMeter.getBounds();
        const float noiseAlpha = 0.032f;

        auto drawNoiseTiled = [&] ()
        {
            if (! noiseTile.isValid())
                return;

            for (int yy = 0; yy < H; yy += noiseTile.getHeight())
                for (int xx = 0; xx < W; xx += noiseTile.getWidth())
                    g.drawImageAt (noiseTile, xx, yy);
        };

        g.setOpacity (noiseAlpha);

        { Graphics::ScopedSaveState ss (g);
          g.reduceClipRegion (Rectangle<int> (0, 0, W, jmax (0, meterI.getY())));
          drawNoiseTiled(); }

        { Graphics::ScopedSaveState ss (g);
          g.reduceClipRegion (Rectangle<int> (0, meterI.getBottom(), W, jmax (0, H - meterI.getBottom())));
          drawNoiseTiled(); }

        { Graphics::ScopedSaveState ss (g);
          g.reduceClipRegion (Rectangle<int> (0, meterI.getY(), jmax (0, meterI.getX()), meterI.getHeight()));
          drawNoiseTiled(); }

        { Graphics::ScopedSaveState ss (g);
          g.reduceClipRegion (Rectangle<int> (meterI.getRight(), meterI.getY(),
                                             jmax (0, W - meterI.getRight()), meterI.getHeight()));
          drawNoiseTiled(); }

        g.setOpacity (1.0f);
    }

    // 3) Meter bezel
    {
        auto meter = grMeter.getBounds().toFloat().expanded (6.0f, 6.0f);

        g.setColour (Colour::fromRGBA (0,0,0,130));
        g.fillRoundedRectangle (meter.translated (0.0f, 1.4f), 16.0f);

        ColourGradient bezel (Colour::fromRGB (35,35,35), meter.getX(), meter.getY(),
                              Colour::fromRGB (7,7,7),   meter.getRight(), meter.getBottom(), false);
        bezel.addColour (0.55, Colour::fromRGB (18,18,18));
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (meter, 16.0f);

        g.setColour (Colour::fromRGBA (255,255,255,14));
        g.drawRoundedRectangle (meter.reduced (0.6f), 15.0f, 1.0f);

        g.setColour (Colour::fromRGBA (0,0,0,210));
        g.drawRoundedRectangle (meter.reduced (1.4f), 14.0f, 1.6f);
    }

    // 4) Frame + screws
    {
        g.setColour (Colour::fromRGBA (255, 255, 255, 7));
        g.drawRect (Rectangle<int> (1, 1, W - 2, H - 2), 1);

        g.setColour (Colour::fromRGBA (0, 0, 0, 190));
        g.drawRect (Rectangle<int> (0, 0, W, H), 2);

        const float srad = jlimit (6.0f, 10.0f, jmin ((float)W, (float)H) * 0.012f);
        const float pad  = 18.0f;

        drawScrew ({ pad, pad }, srad);
        drawScrew ({ r.getRight() - pad, pad }, srad);
        drawScrew ({ pad, r.getBottom() - pad }, srad);
        drawScrew ({ r.getRight() - pad, r.getBottom() - pad }, srad);
    }

    bgDirty = false;
}

//==============================================================================
// Editor ctor
AuricOmega76AudioProcessorEditor::AuricOmega76AudioProcessorEditor (AuricOmega76AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), presetManager (p)
{
    setLookAndFeel (&auricLnf);

    rebuildNoiseTile();

    // Labels
    titleLabel.setText (AuricHelpers::trackCaps ("AURIC"), juce::dontSendNotification);
    presetLabel.setText ("Preset", juce::dontSendNotification);

    inputLabel.setText ("INPUT", juce::dontSendNotification);
    releaseLabel.setText ("RELEASE", juce::dontSendNotification);

    edgeLabel.setText (omegaChar() + juce::String (" EDGE"), juce::dontSendNotification);
    modeLabel.setText ("MODE", juce::dontSendNotification);
    mixLabel .setText (omegaChar() + juce::String (" MIX"),  juce::dontSendNotification);
    omegaMixLabel.setText (omegaChar() + juce::String (" MIX"), juce::dontSendNotification);

    AuricHelpers::styleLabelGold (titleLabel, 24.0f, true, false);
    titleLabel.setColour (juce::Label::textColourId, AuricTheme::goldText().withAlpha (0.92f));

    AuricHelpers::styleLabelGold (presetLabel, 13.0f);
    AuricHelpers::styleLabelGold (inputLabel, 12.5f);
    AuricHelpers::styleLabelGold (releaseLabel, 12.5f);
    AuricHelpers::styleLabelGold (edgeLabel, 12.0f);
    AuricHelpers::styleLabelGold (modeLabel, 12.0f);
    AuricHelpers::styleLabelGold (mixLabel,  12.0f);
    AuricHelpers::styleLabelGold (omegaMixLabel, 12.0f);

    inputLabel.setJustificationType (juce::Justification::centred);
    releaseLabel.setJustificationType (juce::Justification::centred);
    edgeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setJustificationType (juce::Justification::centred);
    mixLabel .setJustificationType (juce::Justification::centred);
    omegaMixLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (presetLabel);
    addAndMakeVisible (inputLabel);
    addAndMakeVisible (releaseLabel);
    addAndMakeVisible (edgeLabel);
    addAndMakeVisible (modeLabel);
    addAndMakeVisible (mixLabel);
    addAndMakeVisible (omegaMixLabel);

    // LEDs
    addAndMakeVisible (led1);
    addAndMakeVisible (led2);

    // Segmented setup
    omegaModeSwitch.setLeftLegend (juce::String::fromUTF8 (u8"\u03A9"), "MODE", true);
    omegaModeSwitch.setSubLabels ("CLEAN", "IRON", "GRIT");

    routingSwitch.setDrawLeftLegend (false);
    routingSwitch.setSubLabels ("A", "D", juce::String::fromUTF8 (u8"\u03A9"));

    // Preset UI
    presetBox.setTextWhenNothingSelected ("DEFAULT");
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.setColour (juce::ComboBox::textColourId, AuricTheme::goldText().withAlpha (0.88f));
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0x00000000));
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x00000000));
    presetBox.setColour (juce::ComboBox::buttonColourId, juce::Colour (0x00000000));
    addAndMakeVisible (presetBox);

    addAndMakeVisible (presetSaveButton);
    addAndMakeVisible (presetLoadButton);
    addAndMakeVisible (presetDeleteButton);

    // Header IDs
    presetBox.setComponentID ("hdr_preset_box");
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

    // Sliders/Knobs
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

    // UX PRO knobs
    setupKnobsUx();

    // Buttons
    scHpfButton.setButtonText ("SC HPF");
    pwrButton.setButtonText ("PWR");
    scHpfButton.setClickingTogglesState (true);
    pwrButton.setClickingTogglesState (true);
    scHpfButton.setComponentID ("hw_sc_hpf");
    pwrButton.setComponentID   ("hw_pwr");
    scHpfButton.setWantsKeyboardFocus (false);
    pwrButton.setWantsKeyboardFocus   (false);
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
        if (! creditsOverlay)
            return;

        const bool show = ! creditsOverlay->isVisible();
        creditsOverlay->setBounds (getLocalBounds());
        creditsOverlay->setVisible (show);

        if (show)
            creditsOverlay->toFront (true);

        repaint();
    };

    // Segmented
    addAndMakeVisible (omegaModeSwitch);
    addAndMakeVisible (routingSwitch);

    routingSwitch.onChange = [this] (int) { updateLEDIndicators(); };

    addAndMakeVisible (uiScaleSwitch);
    uiScaleSwitch.onChange = [this] (int idx) { applyUIScale (idx); };

    // Meter
    grMeter.setUseBallistics (true);
    grMeter.setBallisticsMs (22.0f, 140.0f);
    addAndMakeVisible (grMeter);

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

    updateLEDIndicators();

    // Resizable
    setResizable (true, true);
    setResizeLimits (860, 470, 1600, 900);

    // Default scale (M)
    uiScaleSwitch.setSelectedIndex (1);
    applyUIScale (1);

    startTimerHz (30);
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
    grMeter.tick (30.0);

    updateLEDIndicators();
}

//==============================================================================
// Paint
void AuricOmega76AudioProcessorEditor::paint (juce::Graphics& g)
{
    if (bgDirty || ! bgCache.isValid()
        || bgCache.getWidth() != getWidth()
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

    constexpr float baseW = 1120.0f;
    constexpr float baseH = 620.0f;

    auto bounds = getLocalBounds().toFloat();

    const float sx = bounds.getWidth()  / baseW;
    const float sy = bounds.getHeight() / baseH;
    const float s  = jmin (sx, sy);

    Rectangle<float> ui (0, 0, baseW * s, baseH * s);
    ui.setCentre (bounds.getCentre());

    auto R = [&] (float x, float y, float w, float h)
    {
        return Rectangle<int> ((int) std::round (ui.getX() + x * s),
                               (int) std::round (ui.getY() + y * s),
                               (int) std::round (w * s),
                               (int) std::round (h * s));
    };

    // HEADER
    titleLabel.setBounds (R (14, 10, 210, 34));

    presetLabel.setBounds (R (240, 18, 60, 18));
    presetBox.setBounds   (R (302, 12, 360, 28));

    presetSaveButton.setBounds   (R (670, 12, 54, 28));
    presetLoadButton.setBounds   (R (730, 12, 54, 28));
    presetDeleteButton.setBounds (R (790, 12, 46, 28));

    infoButton.setBounds    (R (842, 12, 30, 28));
    uiScaleSwitch.setBounds (R (878, 14, 120, 24));

    // MAIN ROW
    inputKnob.setBounds   (R (55,  86, 230, 230));
    releaseKnob.setBounds (R (835, 86, 230, 230));
    grMeter.setBounds     (R (380, 94, 360, 220));

    inputLabel.setBounds   (R (55,  320, 230, 18));
    releaseLabel.setBounds (R (835, 320, 230, 18));

    scHpfButton.setBounds (R (40,  344, 260, 34));
    pwrButton.setBounds   (R (820, 344, 260, 34));

    // CENTER SMALL KNOBS
    edgeKnob.setBounds (R (420, 386, 88, 88));
    modeKnob.setBounds (R (516, 386, 88, 88));
    mixKnob .setBounds (R (612, 386, 88, 88));

    edgeLabel.setBounds (R (390, 486, 148, 20));
    modeLabel.setBounds (R (486, 486, 148, 20));
    mixLabel .setBounds (R (582, 486, 148, 20));

    // BOTTOM
    const float mL = 34.0f;
    const float mR = 34.0f;
    const float mB = 20.0f;

    const float omegaMixSz = 128.0f;
    const float switchH  = 60.0f;
    const float routingW = 310.0f;

    const float switchesY = baseH - mB - switchH;

    const float omegaMixX = mL + 44.0f;
    const float omegaMixY = baseH - mB - (omegaMixSz + 20.0f);

    omegaMixKnob.setBounds  (R (omegaMixX, omegaMixY, omegaMixSz, omegaMixSz));
    omegaMixLabel.setBounds (R (omegaMixX, omegaMixY + omegaMixSz + 6.0f, omegaMixSz, 18.0f));

    const float routingX = baseW - mR - routingW;
    routingSwitch.setBounds (R (routingX, switchesY, routingW, switchH));

    const float leftStop  = omegaMixX + omegaMixSz + 40.0f;
    const float rightStop = routingX - 40.0f;

    float omegaModeW = rightStop - leftStop;
    omegaModeW = jlimit (240.0f, 360.0f, omegaModeW);

    const float omegaModeX = leftStop + (rightStop - leftStop - omegaModeW) * 0.3f;
    omegaModeSwitch.setBounds (R (omegaModeX, switchesY, omegaModeW, switchH));

    // LEDs
    const float ledY = switchesY + 18.0f;
    const float ledSize = 10.0f;
    const float ledSpacing = 16.0f;
    const float ledsStartX = omegaModeX + 45.0f;

    led1.setBounds (R (ledsStartX, ledY, ledSize, ledSize));
    led2.setBounds (R (ledsStartX + ledSpacing, ledY, ledSize, ledSize));

    // Overlay full editor
    if (creditsOverlay)
        creditsOverlay->setBounds (getLocalBounds());

    bgDirty = true;
}

//==============================================================================
// LED update
void AuricOmega76AudioProcessorEditor::updateLEDIndicators()
{
    const int routing = routingSwitch.getSelectedIndex();

    switch (routing)
    {
        case 0: // A - Compressor only
            led1.setOn (true);
            led2.setOn (false);
            break;

        case 1: // D - Drive only
            led1.setOn (false);
            led2.setOn (true);
            break;

        case 2: // Ω - Both
            led1.setOn (true);
            led2.setOn (true);
            break;

        default:
            led1.setOn (false);
            led2.setOn (false);
            break;
    }

    led1.repaint();
    led2.repaint();
}
