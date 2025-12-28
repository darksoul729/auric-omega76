#include "AuricValueTooltip.h"
#include "AuricHelpers.h"
#include <cmath>

namespace
{
    inline juce::Colour gold()     { return juce::Colour::fromRGB (226, 195, 129); }
    inline juce::Colour goldHi()   { return juce::Colour::fromRGB (251, 247, 175); }
}

AuricValueTooltip::AuricValueTooltip()
{
    setInterceptsMouseClicks (false, false);
    setWantsKeyboardFocus (false);
    setAlwaysOnTop (true);
    setOpaque (false);
    setVisible (false);
}

void AuricValueTooltip::setUiScale (float s)
{
    uiScale = juce::jmax (0.5f, s);
    if (isVisible())
        updateBounds();
}

bool AuricValueTooltip::isShowingFor (const juce::Component* anchor) const
{
    return anchorComp == anchor && isVisible();
}

void AuricValueTooltip::showFor (juce::Component& anchor, const juce::String& text)
{
    anchorComp = &anchor;
    displayText = text;
    alpha = 1.0f;
    fadingOut = false;

    stopTimer();
    setVisible (true);
    updateBounds();
    repaint();
}

void AuricValueTooltip::updateText (const juce::String& text)
{
    if (displayText == text)
        return;

    displayText = text;
    if (isVisible())
    {
        updateBounds();
        repaint();
    }
}

void AuricValueTooltip::beginFadeOut()
{
    if (! isVisible())
        return;

    fadingOut = true;
    fadeStartMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

void AuricValueTooltip::timerCallback()
{
    if (! fadingOut)
    {
        stopTimer();
        return;
    }

    const double now = juce::Time::getMillisecondCounterHiRes();
    const float t = (float) ((now - fadeStartMs) / fadeDurationMs);

    if (t >= 1.0f)
    {
        alpha = 0.0f;
        setVisible (false);
        stopTimer();
        return;
    }

    alpha = 1.0f - t;
    repaint();
}

void AuricValueTooltip::updateBounds()
{
    if (anchorComp == nullptr || getParentComponent() == nullptr)
        return;

    auto font = AuricHelpers::makeFont (13.0f * uiScale, juce::Font::plain);
    const float textW = font.getStringWidthFloat (displayText);
    const float padX = 8.0f * uiScale;
    const float padY = 4.0f * uiScale;
    const float w = textW + padX * 2.0f;
    const float h = font.getHeight() + padY * 2.0f;

    const auto parent = getParentComponent()->getLocalBounds().toFloat();
    const auto anchor = anchorComp->getBounds().toFloat();

    float x = anchor.getCentreX() - w * 0.5f;
    float y = anchor.getY() - h - 8.0f * uiScale;

    if (y < parent.getY() + 2.0f)
        y = anchor.getBottom() + 8.0f * uiScale;

    x = juce::jlimit (parent.getX() + 2.0f, parent.getRight() - w - 2.0f, x);
    y = juce::jlimit (parent.getY() + 2.0f, parent.getBottom() - h - 2.0f, y);

    setBounds ((int) std::round (x), (int) std::round (y),
               (int) std::round (w), (int) std::round (h));
}

void AuricValueTooltip::paint (juce::Graphics& g)
{
    using namespace juce;

    if (displayText.isEmpty())
        return;

    Graphics::ScopedSaveState ss (g);
    g.setOpacity (alpha);

    auto r = getLocalBounds().toFloat().reduced (0.6f);
    const float rr = jlimit (4.0f, 10.0f, r.getHeight() * 0.45f);

    // soft shadow
    g.setColour (Colour::fromRGBA (0, 0, 0, 150));
    g.fillRoundedRectangle (r.translated (0.0f, 1.4f).expanded (0.6f), rr + 0.8f);

    // dark body
    ColourGradient bg (Colour::fromRGB (26, 26, 24), r.getCentreX(), r.getY(),
                       Colour::fromRGB (10, 10, 10), r.getCentreX(), r.getBottom(), false);
    bg.addColour (0.55, Colour::fromRGB (18, 18, 16));
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, rr);

    g.setColour (Colour::fromRGBA (0, 0, 0, 185));
    g.drawRoundedRectangle (r.reduced (0.7f), rr - 0.4f, 1.2f);

    g.setColour (gold().withAlpha (0.55f));
    g.drawRoundedRectangle (r, rr, 1.0f);

    // text
    g.setColour (goldHi().withAlpha (0.92f));
    g.setFont (AuricHelpers::makeFont (13.0f * uiScale, Font::plain));
    g.drawText (displayText, getLocalBounds(), Justification::centred, false);
}
