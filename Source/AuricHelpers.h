#pragma once
#include <JuceHeader.h>

//==============================================================================
// Helper functions untuk font dan styling
namespace AuricHelpers
{
    // Font safe untuk JUCE lama/baru
    juce::Font makeFont (float size, int styleFlags = juce::Font::plain);
    
    // Style label dengan warna gold
    void styleLabelGold (juce::Label& l, float size, bool bold = false, bool centred = true);
    
    // "Tracking" sederhana: pakai spasi antar huruf untuk brand text
    juce::String trackCaps (const juce::String& s);
    
    // Style knob dengan parameter Auric
    void styleKnobAuric (juce::Slider& s);
}