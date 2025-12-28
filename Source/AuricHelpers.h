#pragma once
#include <JuceHeader.h>

//==============================================================================
// Helper functions untuk font dan styling
namespace AuricHelpers
{
    // Font safe untuk JUCE lama/baru
    juce::Font makeFont (float size, int styleFlags = juce::Font::plain);
    
    // Style label dengan warna gold
    void styleLabelGold (juce::Label& l, float size, bool bold = false, bool centred = true,
                         float tracking = 0.0f);
    
    // "Tracking" sederhana: pakai spasi antar huruf untuk brand text
    juce::String trackCaps (const juce::String& s);
    
    // Style knob dengan parameter Auric
    void styleKnobAuric (juce::Slider& s);

    // Background helpers
    void drawVignetteNoiseOverlay (juce::Graphics& g, juce::Rectangle<float> area, float amount);
    void drawInsetWell (juce::Graphics& g, juce::Rectangle<float> area,
                        float cornerRadius, float depth);
    void drawRaisedBezel (juce::Graphics& g, juce::Rectangle<float> area,
                          float cornerRadius, float depth);

    // Text helper for gold typography + tracking
    void styleGoldText (juce::Graphics& g, juce::Font base, float tracking);
}
