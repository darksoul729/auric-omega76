#include "AuricHelpers.h"

//==============================================================================
// Helper functions untuk font dan styling
namespace AuricHelpers
{
    static juce::Font applyTracking (juce::Font f, float trackingPx)
    {
        if (trackingPx == 0.0f)
            return f;

        const float denom = juce::jmax (1.0f, f.getHeight());
        const float factor = trackingPx / denom;

       #if defined (JUCE_MAJOR_VERSION) && JUCE_MAJOR_VERSION >= 7
        f = f.withExtraKerningFactor (factor);
       #else
        f.setExtraKerningFactor (factor);
       #endif

        return f;
    }

    // Convert JUCE Font style flags (int) -> style String for FontOptions
    static juce::String flagsToStyleString (int styleFlags)
    {
        const bool bold   = (styleFlags & juce::Font::bold)   != 0;
        const bool italic = (styleFlags & juce::Font::italic) != 0;

        if (bold && italic) return "Bold Italic";
        if (bold)           return "Bold";
        if (italic)         return "Italic";
        return "Regular";
    }

    // Font safe untuk JUCE lama/baru
    juce::Font makeFont (float size, int styleFlags)
    {
       #if defined (JUCE_MAJOR_VERSION) && JUCE_MAJOR_VERSION >= 7
        // JUCE 7+ pakai FontOptions (withStyle butuh String)
        juce::FontOptions opts;
        opts = opts.withHeight (size)
                   .withStyle  (flagsToStyleString (styleFlags));

        return juce::Font (opts);
       #else
        // JUCE 6 kebawah: ctor lama
        return juce::Font (size, styleFlags);
       #endif
    }

    // Style label dengan warna gold
    void styleLabelGold (juce::Label& l, float size, bool bold, bool centred, float tracking)
    {
        l.setJustificationType (centred ? juce::Justification::centred
                                        : juce::Justification::centredLeft);

        l.setColour (juce::Label::textColourId, juce::Colour::fromRGB (226, 195, 129));

        auto f = makeFont (size, bold ? juce::Font::bold : juce::Font::plain);
        l.setFont (applyTracking (f, tracking));
    }

    // "Tracking" sederhana: pakai spasi antar huruf untuk brand text
    juce::String trackCaps (const juce::String& s)
    {
        juce::String out;
        for (int i = 0; i < s.length(); ++i)
        {
            out << s.substring (i, i + 1);
            if (i != s.length() - 1)
                out << " ";
        }
        return out;
    }

    // Style knob dengan parameter Auric
    void styleKnobAuric (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setMouseDragSensitivity (160);

        // Range putar 7 pagi -> 5 sore (sweep 300 derajat)
        const float start = juce::MathConstants<float>::pi * (120.0f / 180.0f);             // 120°
        const float end   = start + juce::MathConstants<float>::twoPi * (300.0f / 360.0f);  // +300°
        s.setRotaryParameters (start, end, true);
    }

    void drawVignetteNoiseOverlay (juce::Graphics& g, juce::Rectangle<float> area, float amount)
    {
        using namespace juce;

        amount = jlimit (0.0f, 1.0f, amount);
        if (amount <= 0.001f || area.getWidth() < 4.0f || area.getHeight() < 4.0f)
            return;

        // Vignette
        ColourGradient vg (Colour::fromRGBA (0, 0, 0, 0),
                           area.getCentreX(), area.getCentreY(),
                           Colour::fromRGBA (0, 0, 0, (uint8) (175.0f * amount)),
                           area.getCentreX(), area.getCentreY(),
                           true);
        vg.addColour (0.65, Colour::fromRGBA (0, 0, 0, (uint8) (45.0f * amount)));
        g.setGradientFill (vg);
        g.fillRect (area);

        // Fine noise
        const int seed = ((int) area.getWidth() * 73856093) ^ ((int) area.getHeight() * 19349663) ^ 0x5bd1e995;
        Random rng ((int64) seed);

        const int dots = jlimit (280, 1400, (int) (area.getWidth() * area.getHeight() * 0.0012f));
        for (int i = 0; i < dots; ++i)
        {
            const float x = area.getX() + rng.nextFloat() * area.getWidth();
            const float y = area.getY() + rng.nextFloat() * area.getHeight();

            const bool bright = rng.nextFloat() > 0.52f;
            const float a = (bright ? 0.028f : 0.022f) * amount;

            g.setColour ((bright ? Colours::white : Colours::black).withAlpha (a));
            g.fillRect (x, y, 1.0f, 1.0f);
        }
    }

    void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> area,
                        float cornerRadius, float depth)
    {
        using namespace juce;

        if (area.getWidth() < 6.0f || area.getHeight() < 6.0f)
            return;

        const float d = jmax (0.5f, depth);

        g.setColour (Colour::fromRGBA (0, 0, 0, 160));
        g.fillRoundedRectangle (area.expanded (d * 0.55f).translated (0.0f, d * 0.35f),
                                cornerRadius + d * 0.45f);

        ColourGradient well (Colour::fromRGB (20, 20, 20), area.getCentreX(), area.getY(),
                             Colour::fromRGB (6, 6, 6),    area.getCentreX(), area.getBottom(), false);
        well.addColour (0.55, Colour::fromRGB (12, 12, 12));
        g.setGradientFill (well);
        g.fillRoundedRectangle (area, cornerRadius);

        g.setColour (Colour::fromRGBA (255, 255, 255, 12));
        g.drawRoundedRectangle (area.reduced (0.5f), cornerRadius, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 210));
        g.drawRoundedRectangle (area.reduced (1.4f), cornerRadius - 0.9f, 1.6f);

        auto topSheen = area.reduced (d * 1.2f).withHeight (area.getHeight() * 0.25f);
        g.setColour (Colour::fromRGBA (255, 255, 255, 8));
        g.fillRoundedRectangle (topSheen, cornerRadius - 1.2f);
    }

    void drawRaisedBezel (juce::Graphics& g, juce::Rectangle<float> area,
                          float cornerRadius, float depth)
    {
        using namespace juce;

        if (area.getWidth() < 6.0f || area.getHeight() < 6.0f)
            return;

        const float d = jmax (0.5f, depth);

        g.setColour (Colour::fromRGBA (0, 0, 0, 175));
        g.fillRoundedRectangle (area.expanded (d * 0.65f).translated (0.0f, d * 0.50f),
                                cornerRadius + d * 0.55f);

        ColourGradient bezel (Colour::fromRGB (38, 38, 34), area.getCentreX(), area.getY(),
                              Colour::fromRGB (10, 10, 10), area.getCentreX(), area.getBottom(), false);
        bezel.addColour (0.50, Colour::fromRGB (20, 20, 18));
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (area, cornerRadius);

        g.setColour (Colour::fromRGBA (255, 255, 255, 16));
        g.drawRoundedRectangle (area.reduced (0.6f), cornerRadius, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 210));
        g.drawRoundedRectangle (area.reduced (1.6f), cornerRadius - 0.9f, 1.7f);

        auto sheen = area.reduced (d * 1.3f).withHeight (area.getHeight() * 0.22f);
        g.setColour (Colour::fromRGBA (255, 255, 255, 10));
        g.fillRoundedRectangle (sheen, cornerRadius - 1.4f);
    }

    void styleGoldText (juce::Graphics& g, juce::Font base, float tracking)
    {
        g.setColour (juce::Colour::fromRGB (226, 195, 129));
        g.setFont (applyTracking (base, tracking));
    }
}
