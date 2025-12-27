#pragma once
#include <JuceHeader.h>

// Slider rotary khusus:
// - Shift = fine adjust (lebih lambat)
// - Popup value saat drag (built-in bubble)
// - Double click reset value
class AuricKnob : public juce::Slider
{
public:
    AuricKnob()
    : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag,
                    juce::Slider::NoTextBox)
    {
        setScrollWheelEnabled (true);
        setPopupDisplayEnabled (true, false, nullptr); // parent akan kamu set dari editor
        setMouseDragSensitivity (normalSensitivityPx); // default
    }

    void setPopupParent (juce::Component* parent)
    {
        setPopupDisplayEnabled (true, false, parent);
    }

    void setFineDragMultiplier (float mult)
    {
        // mult > 1 => makin halus (butuh drag lebih jauh)
        fineSensitivityPx = juce::jlimit (normalSensitivityPx + 20, 5000,
                                          (int) std::round (normalSensitivityPx * mult));
    }

    void setNormalSensitivity (int px)
    {
        normalSensitivityPx = juce::jlimit (30, 2000, px);
        fineSensitivityPx   = juce::jlimit (normalSensitivityPx + 20, 5000,
                                            (int) std::round (normalSensitivityPx * 3.0f));
        setMouseDragSensitivity (normalSensitivityPx);
    }

    // Helper: set default reset value (double click)
    void setDoubleClickDefault (double v)
    {
        setDoubleClickReturnValue (true, v);
    }

protected:
    void mouseDown (const juce::MouseEvent& e) override
    {
        updateSensitivityForMods (e.mods);
        juce::Slider::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        updateSensitivityForMods (e.mods);
        juce::Slider::mouseDrag (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        // balik ke normal
        setMouseDragSensitivity (normalSensitivityPx);
        juce::Slider::mouseUp (e);
    }

    void mouseExit (const juce::MouseEvent& e) override
    {
        // safety: kalau cursor keluar saat shift, balikin normal
        setMouseDragSensitivity (normalSensitivityPx);
        juce::Slider::mouseExit (e);
    }

private:
    void updateSensitivityForMods (juce::ModifierKeys mods)
    {
        // Shift = fine
        if (mods.isShiftDown())
            setMouseDragSensitivity (fineSensitivityPx);
        else
            setMouseDragSensitivity (normalSensitivityPx);
    }

    int normalSensitivityPx = 220; // makin besar = makin halus
    int fineSensitivityPx   = 660; // default 3x
};


// Helper konfigurasi cepat
inline void setupAuricKnob (AuricKnob& k,
                            juce::Component* popupParent,
                            double defaultValue,
                            int normalSensitivityPx = 220,
                            float fineMult = 3.0f,
                            std::function<juce::String(double)> formatter = {})
{
    k.setPopupParent (popupParent);
    k.setNormalSensitivity (normalSensitivityPx);
    k.setFineDragMultiplier (fineMult);
    k.setDoubleClickDefault (defaultValue);

    if (formatter)
        k.textFromValueFunction = [formatter](double v) { return formatter (v); };
}
