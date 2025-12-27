#include "AuricHelpers.h"

//==============================================================================
// Helper functions untuk font dan styling
namespace AuricHelpers
{
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
    void styleLabelGold (juce::Label& l, float size, bool bold, bool centred)
    {
        l.setJustificationType (centred ? juce::Justification::centred
                                        : juce::Justification::centredLeft);

        l.setColour (juce::Label::textColourId, juce::Colour::fromRGB (200, 170, 110));

        l.setFont (makeFont (size, bold ? juce::Font::bold : juce::Font::plain));
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
}
