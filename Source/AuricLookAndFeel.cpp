#include "AuricLookAndFeel.h"
#include "AuricHelpers.h"
#include <cmath>

//==============================================================================
AuricLookAndFeel::AuricLookAndFeel() {}

//==============================================================================
// Local palette + helpers
namespace
{
    inline juce::Colour hexRGB (int rgb)
    {
        return juce::Colour::fromRGB ((juce::uint8) ((rgb >> 16) & 0xFF),
                                      (juce::uint8) ((rgb >>  8) & 0xFF),
                                      (juce::uint8) ((rgb      ) & 0xFF));
    }

    inline juce::Colour bgDark()   { return hexRGB (0x111111); }
    inline juce::Colour midDark()  { return hexRGB (0x1E1E1E); }
    inline juce::Colour rimHi()    { return hexRGB (0x3B3B3B); }
    inline juce::Colour rimLo()    { return hexRGB (0x070707); }

    inline juce::Colour gold()     { return hexRGB (0xB08A3A); }
    inline juce::Colour goldHi()   { return hexRGB (0xD1B15B); }

    inline juce::Colour needleCol(){ return juce::Colour::fromRGB (240, 232, 220); }

    inline juce::Point<float> polar (juce::Point<float> c, float ang, float r)
    {
        return { c.x + std::cos (ang) * r,
                 c.y + std::sin (ang) * r };
    }

    inline float clampf (float v, float lo, float hi)
    {
        return juce::jlimit (lo, hi, v);
    }

    inline bool isHardwareButton (const juce::Button& b)
    {
        const auto id = b.getComponentID();
        return id == "hw_sc_hpf" || id == "hw_pwr";
    }

    inline bool isHeaderButton (const juce::Button& b)
    {
        const auto id = b.getComponentID();
        if (id == "hdr_btn"
            || id == "hdr_save" || id == "hdr_load" || id == "hdr_del"
            || id == "hdr_s"    || id == "hdr_m"    || id == "hdr_l"
            || id == "ui_s"     || id == "ui_m"     || id == "ui_l")
            return true;

        const auto t = b.getButtonText().trim().toLowerCase();
        return (t == "save" || t == "load" || t == "del" || t == "s" || t == "m" || t == "l");
    }

    inline bool isHeaderPresetBox (const juce::ComboBox& c)
    {
        return c.getComponentID() == "hdr_preset_box";
    }

    inline void addMicroBrushed (juce::Graphics& g, juce::Rectangle<float> rr, float alpha, int seed)
    {
        using namespace juce;

        if (alpha <= 0.0001f || rr.getWidth() < 8.0f || rr.getHeight() < 6.0f)
            return;

        Graphics::ScopedSaveState ss (g);

        Path clip;
        clip.addRoundedRectangle (rr, rr.getHeight() * 0.35f);
        g.reduceClipRegion (clip);

        Random rng ((int64) (seed ^ 0x51C0FFEE));
        g.setColour (Colour::fromRGBA (255, 255, 255, (uint8) jlimit (0, 255, (int) (alpha * 255.0f))));

        const int lines = jlimit (8, 18, (int) (rr.getHeight() * 0.75f));
        for (int i = 0; i < lines; ++i)
        {
            const float y = rr.getY() + (i + 0.5f) * (rr.getHeight() / (float) lines);
            const float wob = (rng.nextFloat() - 0.5f) * 0.9f;
            g.drawLine (rr.getX() + 6.0f, y + wob, rr.getRight() - 6.0f, y + wob, 1.0f);
        }
    }

    // SC HPF / PWR : deep hardware bar
    inline void drawHardwareBarButton (juce::Graphics& g,
                                       juce::Button& b,
                                       juce::Rectangle<float> r,
                                       bool highlighted,
                                       bool down)
    {
        using namespace juce;

        r = r.reduced (1.0f);
        if (r.getWidth() < 8.0f || r.getHeight() < 8.0f) return;

        const bool on = b.getToggleState();
        const auto gld   = gold();
        const auto gldHi = goldHi();

        const float rr    = clampf (r.getHeight() * 0.30f, 7.0f, 13.0f);
        const float bezel = clampf (r.getHeight() * 0.11f, 2.2f, 4.4f);

        auto outer = r;
        auto well  = r.reduced (bezel);
        auto cap   = r.reduced (bezel + 2.8f);

        const float pressY = (down ? 1.25f : 0.0f) + (on ? 0.55f : 0.0f);
        cap = cap.translated (0.0f, pressY);

        g.setImageResamplingQuality (Graphics::highResamplingQuality);

        g.setColour (Colour::fromRGBA (0, 0, 0, 190));
        g.fillRoundedRectangle (outer.expanded (1.8f).translated (0.0f, 0.8f), rr + 1.6f);

        // bezel
        {
            ColourGradient bezelG (rimHi().withAlpha (0.94f), outer.getCentreX(), outer.getY(),
                                   rimLo().withAlpha (0.99f), outer.getCentreX(), outer.getBottom(), false);
            bezelG.addColour (0.55, midDark().withAlpha (0.90f));
            bezelG.addColour (0.78, bgDark().withAlpha (0.90f));

            g.setGradientFill (bezelG);
            g.fillRoundedRectangle (outer, rr);

            g.setColour (Colour::fromRGBA (255, 255, 255, 16));
            g.drawRoundedRectangle (outer.reduced (0.4f), rr, 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 230));
            g.drawRoundedRectangle (outer.reduced (1.2f), rr - 0.8f, 1.3f);
        }

        // well
        {
            ColourGradient wellG (bgDark().darker (0.18f), well.getCentreX(), well.getY(),
                                  Colour::fromRGB (0, 0, 0),  well.getCentreX(), well.getBottom(), false);
            wellG.addColour (0.55, bgDark().darker (0.42f));

            g.setGradientFill (wellG);
            g.fillRoundedRectangle (well, rr - 1.2f);

            g.setColour (Colour::fromRGBA (255, 255, 255, 10));
            g.drawRoundedRectangle (well.reduced (0.5f), rr - 1.4f, 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 240));
            g.drawRoundedRectangle (well.reduced (1.2f), rr - 2.2f, 1.2f);
        }

        // cap surface
        {
            auto capTop = on ? midDark().brighter (0.030f).interpolatedWith (gldHi, 0.12f)
                             : midDark().brighter (0.022f);
            auto capBot = on ? bgDark().darker (0.42f).interpolatedWith (gld,   0.07f)
                             : bgDark().darker (0.42f);

            if (down)
            {
                capTop = capTop.darker (0.10f);
                capBot = capBot.darker (0.12f);
            }

            ColourGradient capG (capTop, cap.getCentreX(), cap.getY(),
                                 capBot, cap.getCentreX(), cap.getBottom(), false);

            if (highlighted && ! down)
                capG.addColour (0.18, Colour::fromRGBA (255, 255, 255, 10));

            g.setGradientFill (capG);
            g.fillRoundedRectangle (cap, rr - 3.2f);

            addMicroBrushed (g, cap.reduced (2.0f), on ? 0.045f : 0.032f, b.getButtonText().hashCode());

            g.setColour (Colour::fromRGBA (255, 255, 255, on ? 18 : 12));
            g.drawLine (cap.getX() + 14.0f, cap.getY() + 2.0f,
                        cap.getRight() - 14.0f, cap.getY() + 2.0f, 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 185));
            g.drawRoundedRectangle (cap.reduced (0.9f), rr - 3.8f, 1.2f);

            if (on && ! down)
            {
                auto strip = cap.reduced (10.0f, 6.5f);
                strip = strip.withHeight (strip.getHeight() * 0.26f).translated (0.0f, strip.getHeight() * 1.30f);
                g.setColour (gldHi.withAlpha (0.09f));
                g.fillRoundedRectangle (strip, strip.getHeight() * 0.5f);
            }
        }

        if (on)
        {
            auto glow = cap.expanded (2.0f).getIntersection (outer.reduced (1.0f));
            g.setColour (gldHi.withAlpha (0.10f));
            g.drawRoundedRectangle (glow, rr - 1.0f, 2.0f);

            g.setColour (gld.withAlpha (0.040f));
            g.fillRoundedRectangle (glow.reduced (1.4f), rr - 2.2f);
        }

        // indicator inset (left)
        {
            const float dotD = clampf (r.getHeight() * 0.26f, 6.0f, 9.0f);
            auto dot = Rectangle<float> (0, 0, dotD, dotD);
            dot.setCentre (Point<float> (outer.getX() + dotD * 1.05f,
                                         outer.getCentreY() + pressY * 0.30f));

            g.setColour (Colour::fromRGBA (0, 0, 0, 175));
            g.fillEllipse (dot.expanded (1.7f).translated (0.4f, 0.8f));

            ColourGradient dg (Colour::fromRGB (20, 20, 20), dot.getCentreX(), dot.getY(),
                               Colour::fromRGB (6,  6,  6),  dot.getCentreX(), dot.getBottom(), false);
            g.setGradientFill (dg);
            g.fillEllipse (dot);

            g.setColour (Colour::fromRGBA (255, 255, 255, 10));
            g.drawEllipse (dot, 1.0f);

            if (on)
            {
                g.setColour (gldHi.withAlpha (0.42f));
                g.fillEllipse (dot.reduced (1.6f));
                g.setColour (gld.withAlpha (0.13f));
                g.fillEllipse (dot.expanded (2.0f));
            }
        }

        if (highlighted && ! down)
        {
            g.setColour (gld.withAlpha (0.030f));
            g.fillRoundedRectangle (outer, rr);
        }
    }

    // Header pill
    inline void drawHeaderPillButton (juce::Graphics& g,
                                      juce::Button& b,
                                      juce::Rectangle<float> r,
                                      bool highlighted,
                                      bool down)
    {
        using namespace juce;

        r = r.reduced (1.0f);
        if (r.getWidth() < 8.0f || r.getHeight() < 8.0f) return;

        const auto gld   = gold();
        const auto gldHi = goldHi();

        const float rr = clampf (r.getHeight() * 0.35f, 6.0f, 11.0f);

        const bool on = b.getToggleState();
        const float pressY = (down ? 0.80f : 0.0f) + (on ? 0.22f : 0.0f);
        auto cap = r.reduced (1.6f).translated (0.0f, pressY);

        g.setImageResamplingQuality (Graphics::highResamplingQuality);

        g.setColour (Colour::fromRGBA (0, 0, 0, 135));
        g.fillRoundedRectangle (r.expanded (1.1f), rr + 1.1f);

        {
            ColourGradient bezelG (midDark().brighter (0.04f), r.getCentreX(), r.getY(),
                                   rimLo().withAlpha (0.98f),  r.getCentreX(), r.getBottom(), false);
            bezelG.addColour (0.55, midDark().darker (0.12f));

            g.setGradientFill (bezelG);
            g.fillRoundedRectangle (r, rr);

            g.setColour (Colour::fromRGBA (255, 255, 255, 12));
            g.drawRoundedRectangle (r.reduced (0.4f), rr, 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 195));
            g.drawRoundedRectangle (r.reduced (1.0f), rr - 0.6f, 1.1f);
        }

        {
            auto top = bgDark().brighter (0.015f);
            auto bot = bgDark().darker (0.36f);

            if (on)
            {
                top = top.interpolatedWith (gldHi, 0.10f);
                bot = bot.interpolatedWith (gld,   0.06f);
            }

            if (down)
            {
                top = top.darker (0.10f);
                bot = bot.darker (0.12f);
            }

            ColourGradient capG (top, cap.getCentreX(), cap.getY(),
                                 bot, cap.getCentreX(), cap.getBottom(), false);

            if (highlighted && ! down)
                capG.addColour (0.25, juce::Colour::fromRGBA (255, 255, 255, 8));

            g.setGradientFill (capG);
            g.fillRoundedRectangle (cap, rr - 1.8f);

            g.setColour (Colour::fromRGBA (255, 255, 255, down ? 6 : 12));
            g.drawLine (cap.getX() + 8.0f, cap.getY() + 2.0f,
                        cap.getRight() - 8.0f, cap.getY() + 2.0f, 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 175));
            g.drawRoundedRectangle (cap.reduced (0.7f), rr - 2.2f, 1.1f);

            if (on && ! down)
            {
                g.setColour (gldHi.withAlpha (0.10f));
                g.drawRoundedRectangle (cap.expanded (1.2f), rr - 1.0f, 1.6f);
            }
        }

        if (highlighted && ! down)
        {
            g.setColour (gld.withAlpha (0.030f));
            g.fillRoundedRectangle (r, rr);
        }
    }
}

//==============================================================================
// Ceramic texture (visible)
static void drawCeramicTexture (juce::Graphics& g,
                                juce::Rectangle<float> face,
                                juce::Point<float> centre,
                                float angleRadians,
                                float strength01,
                                int seed)
{
    using namespace juce;

    strength01 = jlimit (0.0f, 1.0f, strength01);

    Graphics::ScopedSaveState ss (g);

    Path clip;
    clip.addEllipse (face);
    g.reduceClipRegion (clip);

    g.addTransform (AffineTransform::rotation (angleRadians, centre.x, centre.y));

    const float rMax = face.getWidth() * 0.5f;
    if (rMax < 8.0f || strength01 <= 0.001f)
        return;

    const int s = seed ^ 0x6d2b79f5;
    Random rng ((int64) s);

    const int dots = jlimit (120, 520, (int) (rMax * 7.0f));
    for (int i = 0; i < dots; ++i)
    {
        const float u = rng.nextFloat();
        const float v = rng.nextFloat();
        const float rr = std::sqrt (u) * rMax * 0.98f;
        const float tt = v * MathConstants<float>::twoPi;

        const float x = centre.x + std::cos (tt) * rr;
        const float y = centre.y + std::sin (tt) * rr;

        const float sz = jmap (rng.nextFloat(), 0.7f, 2.2f) * jmax (1.0f, rMax * 0.012f);
        const bool bright = (rng.nextFloat() > 0.50f);

        const float a = (bright ? (0.028f + 0.055f * rng.nextFloat())
                                : (0.018f + 0.045f * rng.nextFloat())) * strength01;

        g.setColour ((bright ? Colours::white : Colours::black).withAlpha (a));
        g.fillEllipse (x - sz * 0.5f, y - sz * 0.5f, sz, sz);
    }

    const int pores = jlimit (18, 60, (int) (rMax * 0.8f));
    for (int i = 0; i < pores; ++i)
    {
        const float u = rng.nextFloat();
        const float v = rng.nextFloat();
        const float rr = std::sqrt (u) * rMax * 0.90f;
        const float tt = v * MathConstants<float>::twoPi;

        const float x = centre.x + std::cos (tt) * rr;
        const float y = centre.y + std::sin (tt) * rr;

        const float sz = jmap (rng.nextFloat(), 2.8f, 6.5f) * jmax (1.0f, rMax * 0.018f);

        g.setColour (Colours::black.withAlpha (0.028f * strength01));
        g.fillEllipse (x - sz * 0.5f, y - sz * 0.5f, sz, sz);

        g.setColour (Colours::white.withAlpha (0.018f * strength01));
        g.fillEllipse (x - sz * 0.22f, y - sz * 0.22f, sz * 0.44f, sz * 0.44f);
    }
}

//==============================================================================
// Popup bubble styling
juce::Font AuricLookAndFeel::getSliderPopupFont (juce::Slider&)
{
    return AuricHelpers::makeFont (13.0f, juce::Font::plain);
}

int AuricLookAndFeel::getSliderPopupPlacement (juce::Slider&)
{
    return juce::BubbleComponent::above;
}

void AuricLookAndFeel::drawBubble (juce::Graphics& g,
                                  juce::BubbleComponent&,
                                  const juce::Point<float>& tip,
                                  const juce::Rectangle<float>& body)
{
    using namespace juce;

    auto r = body.toFloat().reduced (0.6f);

    // shadow
    g.setColour (Colour::fromRGBA (0,0,0,160));
    g.fillRoundedRectangle (r.translated (0.0f, 1.2f).expanded (0.8f), 10.0f);

    ColourGradient bg (Colour::fromRGB (26,26,26), r.getCentreX(), r.getY(),
                       Colour::fromRGB (8,8,8),   r.getCentreX(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 10.0f);

    g.setColour (Colour::fromRGBA (0,0,0,190));
    g.drawRoundedRectangle (r.reduced (0.8f), 9.0f, 1.2f);

    g.setColour (hexRGB (0xB08A3A).withAlpha (0.55f));
    g.drawRoundedRectangle (r, 10.0f, 1.0f);

    // tip triangle
    Path p;
    const float w = 12.0f, h = 8.0f;
    const float baseY = r.getBottom();

    p.startNewSubPath (tip);
    p.lineTo (tip.x - w * 0.5f, baseY);
    p.lineTo (tip.x + w * 0.5f, baseY);
    p.closeSubPath();

    g.setColour (Colour::fromRGB (14,14,14));
    g.fillPath (p);

    g.setColour (hexRGB (0xB08A3A).withAlpha (0.45f));
    g.strokePath (p, PathStrokeType (1.0f));
}

//==============================================================================
// drawRotarySlider (knob design updated)
void AuricLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float sliderPosProportional,
                                        float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    using namespace juce;

    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto centre = bounds.getCentre();
    const float diam  = jmin (bounds.getWidth(), bounds.getHeight());

    const auto nm = slider.getName().toLowerCase();
    const bool nameSaysBig = (nm.contains ("input") || nm.contains ("release"));
    const bool sizeSaysBig = (diam >= 160.0f);
    const bool isBigKnob   = nameSaysBig || sizeSaysBig;
    const bool capOnly     = ! isBigKnob;

    const float angle = rotaryStartAngle
                      + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    const float outerSpace = clampf (diam * (capOnly ? 0.075f : 0.14f), 4.0f, 26.0f);
    float radius = diam * 0.5f - outerSpace;
    radius = jmax (2.0f, radius);

    const auto knob = Rectangle<float> (centre.x - radius, centre.y - radius,
                                        radius * 2.0f, radius * 2.0f);

    // Panel ticks (draw last)
auto drawPanelTicks = [&]()
{
    const int   numTicks  = (diam >= 160.0f) ? 16 : 12;
    const float tickThick = clampf (radius * (capOnly ? 0.020f : 0.027f), 0.85f, 2.8f);
    const float tickLen   = clampf (radius * (capOnly ? 0.080f : 0.100f), 2.8f, 11.0f);

    // >>> lebih jauh dari knob
    const float tickGap   = clampf (radius * (capOnly ? 0.150f : 0.165f), 6.0f, 22.0f);

    const float r2 = radius + tickGap;
    const float r1 = r2 + tickLen;

    // >>> seragam: top/bottom ga “beda”
    const float baseAlpha = 0.22f;   // utama (naik turun dikit banget)
    const float varAlpha  = 0.04f;   // kecilin pengaruh topness

    for (int i = 0; i < numTicks; ++i)
    {
        const float t01 = (numTicks <= 1) ? 0.0f : (float) i / (float) (numTicks - 1);
        const float a   = jmap (t01, rotaryStartAngle, rotaryEndAngle);

        auto p1 = polar (centre, a, r1);
        auto p2 = polar (centre, a, r2);

        // topness dikasih kecil banget biar hampir rata
        const float topness = clampf ((-std::sin (a) * 0.6f) + 0.4f, 0.0f, 1.0f);

        juce::Colour tickCol = gold().interpolatedWith (goldHi(), 0.35f); // fix tint (stabil)
        tickCol = tickCol.withAlpha (baseAlpha + varAlpha * topness);

        // engraved shadow (tetap)
        g.setColour (juce::Colour::fromRGBA (0, 0, 0, capOnly ? 95 : 120));
        g.drawLine (p1.x + 0.35f, p1.y + 0.50f,
                    p2.x + 0.35f, p2.y + 0.50f,
                    tickThick + 0.30f);

        // main tick
        g.setColour (tickCol);
        g.drawLine (p1.x, p1.y, p2.x, p2.y, tickThick);

        // micro highlight DIHAPUS biar ga keliatan dobel
    }
};



if (! capOnly)
{
    using namespace juce;

    // ---- proportions (reduce donut, increase top-face pop)
    const float ringOuterGrow = radius * 0.145f;
    const float ringInnerGrow = radius * 0.105f;

    const float bodyGrow = radius * 0.060f;
    const float bodyDrop = radius * 0.065f;

    // face sits higher (more 3D)
    auto face = knob.reduced (radius * 0.070f)
                    .translated (0.0f, -radius * 0.055f);

    // 0) Deep cast shadow (makes knob sit on panel)
    {
        auto sh = knob.expanded (radius * 0.20f).translated (0.0f, radius * 0.12f);
        g.setColour (Colour::fromRGBA (0, 0, 0, 150));
        g.fillEllipse (sh);
    }

    // 1) HALO ring (subtle, not donut)
    {
        auto outer = knob.expanded (ringOuterGrow);
        auto inner = knob.expanded (ringInnerGrow);

        Path ring;
        ring.addEllipse (outer);
        ring.addEllipse (inner);
        ring.setUsingNonZeroWinding (false);

        ColourGradient hg (Colour::fromRGBA (0, 0, 0, 16),
                           centre.x, centre.y + radius * 0.65f,
                           Colour::fromRGBA (0, 0, 0, 0),
                           centre.x, centre.y - radius * 0.45f,
                           false);
        g.setGradientFill (hg);
        g.fillPath (ring);

        // outer hairline (clean edge cue)
        g.setColour (Colour::fromRGBA (255, 255, 255, 7));
        g.drawEllipse (outer.reduced (0.8f), 1.0f);

        // inner edge separation
        g.setColour (Colour::fromRGBA (0, 0, 0, 120));
        g.drawEllipse (inner.expanded (0.8f), 1.0f);
    }

    // 2) BODY / skirt (depth)
    auto body = knob.expanded (bodyGrow).translated (0.0f, bodyDrop);

    {
        ColourGradient bodyGrad (midDark().darker (0.07f),
                                 centre.x, body.getY(),
                                 bgDark().darker (0.66f),
                                 centre.x, body.getBottom(),
                                 false);
        bodyGrad.addColour (0.55, midDark().darker (0.30f));

        g.setGradientFill (bodyGrad);
        g.fillEllipse (body);

        // inner cavity shadow
        g.setColour (Colour::fromRGBA (0, 0, 0, 80));
        g.fillEllipse (body.reduced (radius * 0.12f).translated (0.0f, radius * 0.14f));

        // skirt separation highlight (subtle lift)
        g.setColour (Colour::fromRGBA (255, 255, 255, 7));
        g.drawEllipse (body.reduced (radius * 0.060f), 1.0f);

        // outer edge definition
        g.setColour (Colour::fromRGBA (0, 0, 0, 195));
        g.drawEllipse (body.reduced (radius * 0.092f),
                       clampf (radius * 0.023f, 1.1f, 2.9f));

        // tiny rim highlight
        g.setColour (Colour::fromRGBA (255, 255, 255, 9));
        g.drawEllipse (body.reduced (radius * 0.028f), 1.0f);
    }

    // 3) FACE (top cap) — raised, premium cues (NO CENTER DOT)
    {
        // face drop shadow (pop-out)
        g.setColour (Colour::fromRGBA (0, 0, 0, 130));
        g.fillEllipse (face.translated (0.0f, radius * 0.032f).expanded (radius * 0.032f));

        // base face gradient
        ColourGradient faceGrad (midDark().brighter (0.020f),
                                 centre.x, face.getY(),
                                 bgDark().darker (0.38f),
                                 centre.x, face.getBottom(),
                                 false);
        faceGrad.addColour (0.55, midDark().darker (0.10f));
        g.setGradientFill (faceGrad);
        g.fillEllipse (face);

        // premium radial highlight (top-left slightly brighter)
        {
            auto inner = face.reduced (radius * 0.07f);
            ColourGradient rg (Colour::fromRGBA (255, 255, 255, 20),
                               inner.getX() + inner.getWidth() * 0.28f,
                               inner.getY() + inner.getHeight() * 0.22f,
                               Colour::fromRGBA (0, 0, 0, 0),
                               inner.getCentreX(),
                               inner.getCentreY(),
                               true);
            rg.addColour (1.0, Colour::fromRGBA (0, 0, 0, 0));
            g.setGradientFill (rg);
            g.fillEllipse (inner);
        }

        // matte shading (bottom heavier)
        ColourGradient matteV (Colour::fromRGBA (0, 0, 0, 62),
                               centre.x, centre.y + radius * 0.62f,
                               Colour::fromRGBA (0, 0, 0, 0),
                               centre.x, centre.y - radius * 0.50f,
                               false);
        g.setGradientFill (matteV);
        g.fillEllipse (face.reduced (radius * 0.090f));

        // texture: softer so it doesn't look dusty
        drawCeramicTexture (g,
                            face.reduced (radius * 0.11f),
                            centre,
                            angle,
                            0.32f,
                            slider.getName().hashCode());

        // soft centre bloom (NO DOT)
        {
            auto inner = face.reduced (radius * 0.20f);

            ColourGradient cg (Colour::fromRGBA (255, 255, 255, 10),
                               inner.getCentreX(), inner.getCentreY(),
                               Colour::fromRGBA (0, 0, 0, 0),
                               inner.getCentreX(), inner.getCentreY(),
                               true);
            cg.addColour (1.0, Colour::fromRGBA (0, 0, 0, 0));

            g.setGradientFill (cg);
            g.fillEllipse (inner);
        }

        // stepped rings (depth)
        g.setColour (Colour::fromRGBA (0, 0, 0, 175));
        g.drawEllipse (face.reduced (radius * 0.032f),
                       clampf (radius * 0.050f, 1.2f, 4.6f));

        g.setColour (Colour::fromRGBA (255, 255, 255, 10));
        g.drawEllipse (face.reduced (radius * 0.012f),
                       clampf (radius * 0.015f, 0.9f, 1.8f));

        // rim highlight (helps "pop" around edge)
        g.setColour (Colour::fromRGBA (255, 255, 255, 8));
        g.drawEllipse (face.reduced (radius * 0.020f), 1.0f);

        // lens ring (adds premium "cap" feel without dot)
        g.setColour (Colour::fromRGBA (255, 255, 255, 6));
        g.drawEllipse (face.reduced (radius * 0.095f), 1.0f);
        g.setColour (Colour::fromRGBA (0, 0, 0, 55));
        g.drawEllipse (face.reduced (radius * 0.105f), 1.2f);

        // specular arc (stronger spark)
        {
            Path arc;
            auto rr = face.reduced (radius * 0.040f);
            arc.addCentredArc (rr.getCentreX(), rr.getCentreY(),
                               rr.getWidth() * 0.5f, rr.getHeight() * 0.5f,
                               0.0f,
                               MathConstants<float>::pi * 1.12f,
                               MathConstants<float>::pi * 1.62f,
                               true);

            g.setColour (Colour::fromRGBA (255, 255, 255, 52));
            g.strokePath (arc, PathStrokeType (clampf (radius * 0.020f, 1.4f, 2.8f),
                                               PathStrokeType::curved,
                                               PathStrokeType::rounded));
        }

        // subtle inner ring (keeps depth without dot)
        g.setColour (Colour::fromRGBA (0, 0, 0, 70));
        g.drawEllipse (face.reduced (radius * 0.18f), 1.2f);

        g.setColour (Colour::fromRGBA (255, 255, 255, 5));
        g.drawEllipse (face.reduced (radius * 0.175f), 1.0f);
    }

    // 4) INDICATOR sticker (embedded pill) — slimmer + shorter + moved inward
    {
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::rotation (angle, centre.x, centre.y));

        const float faceR  = face.getWidth() * 0.5f;

        const float stripW = clampf (faceR * 0.070f, 4.6f, 9.2f);
        const float stripH = clampf (faceR * 0.235f, 10.0f, 26.0f);
        const float stripR = faceR * 0.69f;

        Rectangle<float> strip (0, 0, stripW, stripH);
        strip.setCentre (polar (centre, -MathConstants<float>::halfPi, stripR));

        const float cr = stripW * 0.45f;

        // outer shadow
        g.setColour (Colour::fromRGBA (0, 0, 0, 74));
        g.fillRoundedRectangle (strip.translated (0.50f, 0.70f), cr);

        // main sticker (slightly softer)
        g.setColour (needleCol().withAlpha (0.38f));
        g.fillRoundedRectangle (strip, cr);

        // embedded inner shadow
        {
            auto sh = strip;
            sh = sh.withHeight (sh.getHeight() * 0.38f)
                   .translated (0.0f, strip.getHeight() * 0.42f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 24));
            g.fillRoundedRectangle (sh, cr);
        }

        // subtle edge
        g.setColour (Colour::fromRGBA (0, 0, 0, 16));
        g.drawRoundedRectangle (strip.reduced (0.55f), cr, 1.0f);

        // tiny highlight line
        g.setColour (Colour::fromRGBA (255, 255, 255, 6));
        g.drawLine (strip.getX() + 0.6f, strip.getY() + 1.0f,
                    strip.getRight() - 0.6f, strip.getY() + 1.0f, 1.0f);
    }

    drawPanelTicks();
    return;
}


    // ============================================================
    // SMALL KNOB (TOP-FACE ONLY) — no body, but more “hardware” bevel
    {
        const float faceR = radius * 0.99f;
        auto face = Rectangle<float> (centre.x - faceR, centre.y - faceR,
                                      faceR * 2.0f, faceR * 2.0f);

        // tiny cast shadow (still sits on panel)
        g.setColour (Colour::fromRGBA (0, 0, 0, 130));
        g.fillEllipse (face.translated (0.0f, faceR * 0.070f).expanded (faceR * 0.10f));

        // outer bevel ring (still top-face)
        auto bevel = face.expanded (faceR * 0.03f).translated (0.0f, -faceR * 0.01f);
        {
            ColourGradient bevelG (midDark().brighter (0.05f),
                                   centre.x, bevel.getY(),
                                   bgDark().darker (0.55f),
                                   centre.x, bevel.getBottom(),
                                   false);
            bevelG.addColour (0.55, midDark().darker (0.12f));
            g.setGradientFill (bevelG);
            g.fillEllipse (bevel);

            g.setColour (Colour::fromRGBA (0,0,0,185));
            g.drawEllipse (bevel.reduced (faceR * 0.020f), clampf (faceR * 0.020f, 0.9f, 2.2f));

            g.setColour (Colour::fromRGBA (255,255,255,9));
            g.drawEllipse (bevel.reduced (faceR * 0.006f), clampf (faceR * 0.010f, 0.7f, 1.4f));
        }

        // face gradient + texture
        {
            ColourGradient faceGrad (midDark().brighter (0.010f),
                                     centre.x, face.getY(),
                                     bgDark().darker (0.32f),
                                     centre.x, face.getBottom(),
                                     false);
            faceGrad.addColour (0.55, midDark().darker (0.12f));
            g.setGradientFill (faceGrad);
            g.fillEllipse (face);

            ColourGradient matteV (Colour::fromRGBA (0, 0, 0, 52),
                                   centre.x, centre.y + faceR * 0.64f,
                                   Colour::fromRGBA (0, 0, 0, 0),
                                   centre.x, centre.y - faceR * 0.36f,
                                   false);
            g.setGradientFill (matteV);
            g.fillEllipse (face.reduced (faceR * 0.10f));

            drawCeramicTexture (g,
                                face.reduced (faceR * 0.12f),
                                centre,
                                angle,
                                0.75f,
                                slider.getName().hashCode());

            g.setColour (Colour::fromRGBA (0, 0, 0, 175));
            g.drawEllipse (face.reduced (faceR * 0.028f),
                           clampf (faceR * 0.032f, 0.9f, 2.4f));
        }

        // indicator sticker
        {
            Graphics::ScopedSaveState ss (g);
            g.addTransform (AffineTransform::rotation (angle, centre.x, centre.y));

            const float stripW = clampf (faceR * 0.17f, 6.0f, 16.0f);
            const float stripH = clampf (faceR * 0.34f, 12.0f, 30.0f);
            const float stripR = faceR * 0.71f;

            Rectangle<float> strip (0, 0, stripW, stripH);
            strip.setCentre (polar (centre, -MathConstants<float>::halfPi, stripR));

            g.setColour (Colour::fromRGBA (0, 0, 0, 90));
            g.fillRoundedRectangle (strip.translated (0.5f, 0.7f), stripW * 0.32f);

            g.setColour (needleCol().withAlpha (0.60f));
            g.fillRoundedRectangle (strip, stripW * 0.32f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 18));
            g.drawRoundedRectangle (strip.reduced (0.5f), stripW * 0.32f, 1.0f);
        }

        drawPanelTicks();
    }
}

//==============================================================================
// Buttons/ComboBox/Labels (sama seperti gaya kamu)
void AuricLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                        bool highlighted, bool down)
{
    using namespace juce;

    if (isHardwareButton (button))
    {
        drawHardwareBarButton (g, button, button.getLocalBounds().toFloat(), highlighted, down);

        auto r = button.getLocalBounds().toFloat();
        r.removeFromLeft ((float) button.getHeight() * 0.38f);

        const bool  on     = button.getToggleState();
        const float pressY = (down ? 1.1f : 0.0f) + (on ? 0.45f : 0.0f);

        g.setFont (AuricHelpers::makeFont ((float) jlimit (12, 16, button.getHeight() - 12),
                                           Font::plain));

        g.setColour (Colour::fromRGBA (0, 0, 0, 165));
        g.drawText (button.getButtonText(),
                    r.translated (0.8f, 1.05f + pressY).toNearestInt(),
                    Justification::centred, false);

        auto tc = (on ? goldHi().withAlpha (0.92f) : gold().withAlpha (0.84f));
        g.setColour (tc);
        g.drawText (button.getButtonText(),
                    r.translated (0.0f, pressY).toNearestInt(),
                    Justification::centred, false);

        if (highlighted && ! down)
        {
            g.setColour (Colour::fromRGBA (255, 255, 255, 10));
            g.drawText (button.getButtonText(),
                        r.translated (-0.25f, -0.25f + pressY).toNearestInt(),
                        Justification::centred, false);
        }
        return;
    }

    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto gld = gold();

    ColourGradient bg (midDark(), r.getCentreX(), r.getY(),
                       bgDark(),  r.getCentreX(), r.getBottom(), false);
    if (down)
        bg = ColourGradient (Colour::fromRGB (16, 16, 16), r.getCentreX(), r.getY(),
                             Colour::fromRGB ( 8,  8,  8), r.getCentreX(), r.getBottom(), false);

    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 6.0f);

    g.setColour (Colour::fromRGBA (0, 0, 0, 200));
    g.drawRoundedRectangle (r.reduced (0.6f), 6.0f, 1.0f);

    g.setColour (Colour::fromRGBA (255, 255, 255, 14));
    g.drawLine (r.getX() + 8.0f, r.getY() + 2.0f, r.getRight() - 8.0f, r.getY() + 2.0f, 1.0f);

    if (button.getToggleState())
    {
        g.setColour (gld.withAlpha (0.12f));
        g.fillRoundedRectangle (r.reduced (2.0f), 5.0f);

        g.setColour (gld.withAlpha (0.26f));
        g.drawRoundedRectangle (r.reduced (1.6f), 5.0f, 1.0f);
    }

    if (highlighted)
    {
        g.setColour (gld.withAlpha (0.05f));
        g.fillRoundedRectangle (r, 6.0f);
    }

    g.setColour (gld.withAlpha (0.90f));
    g.setFont (AuricHelpers::makeFont (13.0f, Font::plain));
    g.drawText (button.getButtonText(), button.getLocalBounds(), Justification::centred, false);
}

juce::Font AuricLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return AuricHelpers::makeFont ((float) juce::jlimit (12, 15, buttonHeight - 12), juce::Font::plain);
}

void AuricLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&, bool highlighted, bool down)
{
    using namespace juce;

    if (isHardwareButton (button))
    {
        drawHardwareBarButton (g, button, button.getLocalBounds().toFloat(), highlighted, down);
        return;
    }

    if (isHeaderButton (button))
    {
        // (re-use helper from namespace)
        // NOTE: helper drawHeaderPillButton ada di anonymous namespace atas.
        // Kita panggil via lambda trick: declare again? -> cara simpel: copy call below:
        // Karena helpernya ada di namespace anonymous yang sama file, ini valid:
        extern void __dummy(); // no-op to silence some compilers
        // langsung panggil:
        // drawHeaderPillButton(...)
        // tapi helpernya anonymous scope -> bisa dipanggil langsung:
        drawHeaderPillButton (g, button, button.getLocalBounds().toFloat(), highlighted, down);
        return;
    }

    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto gld = gold();

    ColourGradient bg (Colour::fromRGB (32, 32, 32), r.getCentreX(), r.getY(),
                       Colour::fromRGB (10, 10, 10), r.getCentreX(), r.getBottom(), false);

    if (down)
        bg = ColourGradient (Colour::fromRGB (16, 16, 16), r.getCentreX(), r.getY(),
                             Colour::fromRGB ( 8,  8,  8), r.getCentreX(), r.getBottom(), false);

    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 6.0f);

    g.setColour (Colour::fromRGBA (0, 0, 0, 205));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    g.setColour (Colour::fromRGBA (255, 255, 255, down ? 8 : 14));
    g.drawLine (r.getX() + 7.0f, r.getY() + 2.0f, r.getRight() - 7.0f, r.getY() + 2.0f, 1.0f);

    if (highlighted)
    {
        g.setColour (gld.withAlpha (0.05f));
        g.fillRoundedRectangle (r, 6.0f);
    }
}

void AuricLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                       bool, bool)
{
    using namespace juce;

    const auto gld = gold();
    g.setColour (gld.withAlpha (0.90f));
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.drawText (button.getButtonText(), button.getLocalBounds(),
                Justification::centred, false);
}

void AuricLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                     int, int, int, int, juce::ComboBox& box)
{
    using namespace juce;

    auto r = Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.8f);
    const auto gld = gold();

    const bool hdr = isHeaderPresetBox (box);
    const float rr = hdr ? 7.5f : 5.0f;

    if (hdr)
    {
        g.setColour (Colour::fromRGBA (0, 0, 0, 145));
        g.fillRoundedRectangle (r.expanded (1.1f), rr + 1.1f);

        ColourGradient bezelG (midDark().brighter (0.04f), r.getCentreX(), r.getY(),
                               rimLo().withAlpha (0.98f),  r.getCentreX(), r.getBottom(), false);
        bezelG.addColour (0.55, midDark().darker (0.12f));

        g.setGradientFill (bezelG);
        g.fillRoundedRectangle (r, rr);

        g.setColour (Colour::fromRGBA (255, 255, 255, 12));
        g.drawRoundedRectangle (r.reduced (0.4f), rr, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 195));
        g.drawRoundedRectangle (r.reduced (1.0f), rr - 0.6f, 1.1f);

        auto field = r.reduced (2.0f);

        ColourGradient fg (bgDark().brighter (0.01f), field.getCentreX(), field.getY(),
                           Colour::fromRGB (0, 0, 0),    field.getCentreX(), field.getBottom(), false);
        g.setGradientFill (fg);
        g.fillRoundedRectangle (field, rr - 1.6f);

        g.setColour (Colour::fromRGBA (255, 255, 255, 9));
        g.drawRoundedRectangle (field.reduced (0.3f), rr - 1.8f, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 220));
        g.drawRoundedRectangle (field.reduced (1.0f), rr - 2.4f, 1.1f);

        auto arrowArea = field.removeFromRight (22.0f);
        g.setColour (Colour::fromRGBA (0, 0, 0, 155));
        g.drawLine (arrowArea.getX(), r.getY() + 4.0f, arrowArea.getX(), r.getBottom() - 4.0f, 1.0f);

        auto a = arrowArea.reduced (6.0f);
        Path p;
        p.startNewSubPath (a.getX(), a.getY() + 3.0f);
        p.lineTo (a.getCentreX(), a.getBottom() - 2.5f);
        p.lineTo (a.getRight(), a.getY() + 3.0f);

        g.setColour (gld.withAlpha (0.65f));
        g.strokePath (p, PathStrokeType (2.0f));
    }
    else
    {
        ColourGradient grad (Colour::fromRGB (22, 22, 22), r.getCentreX(), r.getY(),
                             Colour::fromRGB (9,  9,  9),  r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 5.0f);

        g.setColour (Colour::fromRGBA (255, 255, 255, 12));
        g.drawRoundedRectangle (r, 5.0f, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 185));
        g.drawRoundedRectangle (r.reduced (1.2f), 5.0f, 1.0f);

        auto arrowR = r.removeFromRight (18.0f).reduced (5.0f);
        Path p;
        p.startNewSubPath (arrowR.getX(), arrowR.getY() + 4.0f);
        p.lineTo (arrowR.getCentreX(), arrowR.getBottom() - 3.0f);
        p.lineTo (arrowR.getRight(), arrowR.getY() + 4.0f);

        g.setColour (gld.withAlpha (0.62f));
        g.strokePath (p, PathStrokeType (2.0f));
    }

    box.setColour (ComboBox::textColourId,  gld.withAlpha (0.88f));
    box.setColour (ComboBox::arrowColourId, gld.withAlpha (0.68f));
}

void AuricLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    if (isHeaderPresetBox (box))
        label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    else
        label.setBounds (1, 1, box.getWidth() - 20, box.getHeight() - 2);

    label.setFont (AuricHelpers::makeFont (13.0f));
}

void AuricLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    using namespace juce;

    auto r = label.getLocalBounds();
    g.setFont (label.getFont());

    const auto textCol = label.findColour (Label::textColourId).withAlpha (0.92f);

    g.setColour (Colour::fromRGBA (0, 0, 0, 130));
    g.drawText (label.getText(), r.translated (0, 1), label.getJustificationType(), true);

    g.setColour (textCol);
    g.drawText (label.getText(), r, label.getJustificationType(), true);
}
