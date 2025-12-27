// GainReductionMeter.cpp
#include "GainReductionMeter.h"
#include "AuricHelpers.h"

#include <cmath>

//==============================================================================
// Ballistics
void GainReductionMeterComponent::setGainReductionDb (float db)
{
    targetDb = db;

    if (! useBallistics)
    {
        currentDb = targetDb;
        repaint();
    }
}

void GainReductionMeterComponent::setBallisticsMs (float attackMs, float releaseMs)
{
    attackCoeffMs  = juce::jmax (0.1f, attackMs);
    releaseCoeffMs = juce::jmax (0.1f, releaseMs);
}

void GainReductionMeterComponent::tick (double hz)
{
    if (! useBallistics || hz <= 1.0)
    {
        currentDb = targetDb;
        repaint();
        return;
    }

    const float dt  = (float) (1.0 / hz);
    const float tgt = targetDb;
    float cur = currentDb;

    const bool moreGR = (tgt > cur);
    const float ms = moreGR ? attackCoeffMs : releaseCoeffMs;

    const float tau = juce::jmax (0.001f, ms * 0.001f);
    const float a   = std::exp (-dt / tau);
    cur = tgt + (cur - tgt) * a;

    currentDb = cur;
    repaint();
}

//==============================================================================
// Paint (RE-DESIGN ONLY: hardware matte + glass + paper print)
// NOTE: GEOMETRY yang kamu tandai "FIXED" TIDAK diubah.
//==============================================================================
void GainReductionMeterComponent::paint (juce::Graphics& g)
{
    using namespace juce;

    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    auto r = getLocalBounds().toFloat();

    // =========================================================
    // BEZEL/GLASS TUNING (biar nempel ke ref)
    const float cornerR      = 12.0f;    // radius bezel luar
    const float bezelInset   = 18.0f;    // tebal bezel
    const float frameGap     = 4.8f;     // border antara kaca & frame
    const float glassInset   = 4.2f;     // jarak frame -> kaca
    const float barRatio     = 0.245f;   // tinggi bottom bar (ref ~ 24-26%)
    const float screwYBias   = 0.04f;    // screw naik/turun (0 = tengah bar)

    // DIAL (arc/ticks/needle) TUNING (ELLIPSE)  --- FIXED (jangan ubah)
    const float yScale       = 0.60f;
    const float pivotUpRatio = 0.025f;
    const float rxMul        = 0.560f;
    const float xNudgeMul    = 0.016f;

    const float arcStroke    = 1.9f;
    const float arcShadow    = 2.1f;

    // sweep meter --- FIXED (jangan ubah)
    const float startAng = -MathConstants<float>::pi * 0.998f;
    const float endAng   =  MathConstants<float>::pi * 0.028f;

    // =========================================================
    // Palette (match Auric matte hardware)
    auto hexRGB = [] (int rgb) -> Colour
    {
        return Colour::fromRGB ((uint8) ((rgb >> 16) & 0xFF),
                                (uint8) ((rgb >>  8) & 0xFF),
                                (uint8) ((rgb      ) & 0xFF));
    };

    const Colour bezelHi   = hexRGB (0x2A2A2A);
    const Colour bezelMid  = hexRGB (0x141414);
    const Colour bezelLo   = hexRGB (0x050505);

    const Colour frameHi   = Colour::fromRGBA (255, 255, 255, 12);
    const Colour frameLo   = Colour::fromRGBA (0, 0, 0, 210);

    // paper/amber (lebih “print matte”)
    const Colour amberHi   = Colour::fromRGB (246, 190, 116);
    const Colour amberMid  = Colour::fromRGB (210, 146, 72);
    const Colour amberLow  = Colour::fromRGB (110,  68, 30);

    // ink
    const Colour inkDark   = Colour::fromRGBA (34, 21, 11, 235);
    const Colour inkDim    = Colour::fromRGBA (34, 21, 11, 160);

    // helpers
    auto clampU8 = [] (int v) -> uint8 { return (uint8) jlimit (0, 255, v); };

    auto fillRoundedClip = [&] (Rectangle<float> area, float cr, std::function<void()> fn)
    {
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (area, cr);
        g.reduceClipRegion (clip);
        fn();
    };

    // brushed metal hairlines (deterministic)
    auto addMetalHairlines = [&] (Rectangle<float> area, float alpha01, bool horizontal, int seed)
    {
        alpha01 = jlimit (0.0f, 1.0f, alpha01);
        if (alpha01 <= 0.0001f) return;

        Random rng ((int64) seed);

        fillRoundedClip (area, cornerR, [&]
        {
            g.setColour (Colours::white.withAlpha (0.020f * alpha01));
            const float step = jlimit (2.2f, 5.6f, area.getHeight() * 0.035f);

            if (horizontal)
            {
                for (float y = area.getY(); y <= area.getBottom(); y += step)
                {
                    const float wob = (rng.nextFloat() - 0.5f) * 0.9f;
                    const float a   = 0.010f + 0.020f * rng.nextFloat();
                    g.setColour (Colours::white.withAlpha (a * alpha01));
                    g.drawLine (area.getX(), y + wob, area.getRight(), y + wob, 1.0f);
                }
            }
            else
            {
                for (float x = area.getX(); x <= area.getRight(); x += step)
                {
                    const float wob = (rng.nextFloat() - 0.5f) * 0.9f;
                    const float a   = 0.010f + 0.020f * rng.nextFloat();
                    g.setColour (Colours::white.withAlpha (a * alpha01));
                    g.drawLine (x + wob, area.getY(), x + wob, area.getBottom(), 1.0f);
                }
            }

            // tiny dark scratches
            const int scratches = 14;
            for (int i = 0; i < scratches; ++i)
            {
                const float y  = area.getY() + rng.nextFloat() * area.getHeight();
                const float x0 = area.getX() + rng.nextFloat() * area.getWidth();
                const float x1 = jlimit (area.getX(), area.getRight(), x0 + (rng.nextFloat() * 120.0f - 60.0f));

                g.setColour (Colours::black.withAlpha (0.020f * alpha01));
                g.drawLine (x0, y, x1, y + (rng.nextFloat() - 0.5f) * 1.2f, 1.0f);
            }
        });
    };

    // paper grain (deterministic, cheap)
    auto addPaperGrain = [&] (Rectangle<float> area, float strength01)
    {
        strength01 = jlimit (0.0f, 1.0f, strength01);
        if (strength01 <= 0.001f) return;

        const int seed = (int) (area.getWidth() * 131) ^ (int) (area.getHeight() * 977) ^ 0x51C0FFEE;
        Random rng ((int64) seed);

        fillRoundedClip (area, cornerR - 5.0f, [&]
        {
            const int dots = jlimit (140, 420, (int) (area.getWidth() * 0.75f));
            for (int i = 0; i < dots; ++i)
            {
                const float x = area.getX() + rng.nextFloat() * area.getWidth();
                const float y = area.getY() + rng.nextFloat() * area.getHeight();

                const bool bright = rng.nextFloat() > 0.55f;
                const float a = (bright ? (0.020f + 0.040f * rng.nextFloat())
                                        : (0.014f + 0.032f * rng.nextFloat())) * strength01;

                const float sz = jmap (rng.nextFloat(), 0.6f, 1.8f);

                g.setColour ((bright ? Colours::white : Colours::black).withAlpha (a));
                g.fillEllipse (x - sz * 0.5f, y - sz * 0.5f, sz, sz);
            }

            const int fibers = 12;
            for (int i = 0; i < fibers; ++i)
            {
                const float y  = area.getY() + rng.nextFloat() * area.getHeight();
                const float wob = (rng.nextFloat() - 0.5f) * 1.2f;

                g.setColour (Colours::white.withAlpha (0.010f * strength01));
                g.drawLine (area.getX(), y + wob, area.getRight(), y + wob, 1.0f);
            }
        });
    };

    auto drawScrew = [&] (Point<float> c, float rad)
    {
        Rectangle<float> hub (c.x - rad, c.y - rad, rad * 2.0f, rad * 2.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 175));
        g.fillEllipse (hub.expanded (2.3f));

        ColourGradient hg (Colour::fromRGB (38, 38, 38), hub.getCentreX(), hub.getY(),
                           Colour::fromRGB (10, 10, 10), hub.getCentreX(), hub.getBottom(), false);
        hg.addColour (0.55, Colour::fromRGB (18, 18, 18));
        g.setGradientFill (hg);
        g.fillEllipse (hub);

        g.setColour (Colour::fromRGBA (255, 255, 255, 18));
        g.drawEllipse (hub, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 235));
        g.drawEllipse (hub.reduced (1.2f), 1.2f);

        // slot
        g.setColour (Colour::fromRGBA (0, 0, 0, 190));
        g.drawLine (hub.getX() + rad * 0.55f, hub.getCentreY(),
                    hub.getRight() - rad * 0.55f, hub.getCentreY(), 2.3f);

        g.setColour (Colour::fromRGBA (255, 255, 255, 18));
        g.drawLine (hub.getX() + rad * 0.55f, hub.getCentreY() - 1.0f,
                    hub.getRight() - rad * 0.55f, hub.getCentreY() - 1.0f, 1.0f);
    };

    // =========================================================
    // 0) clear base (transparent safe)
    g.fillAll (Colours::transparentBlack);

    // =========================================================
    // 1) OUTER BEZEL (matte metal, clearer depth)
    auto outer = r.reduced (1.0f);

    {
        // cast shadow behind bezel
        g.setColour (Colour::fromRGBA (0, 0, 0, 210));
        g.fillRoundedRectangle (outer.expanded (2.3f).translated (0.0f, 1.2f), cornerR + 2.0f);

        // main bezel gradient
        ColourGradient bz (bezelHi, outer.getCentreX(), outer.getY(),
                           bezelLo, outer.getCentreX(), outer.getBottom(), false);
        bz.addColour (0.22, bezelMid);
        bz.addColour (0.56, Colour::fromRGB (10, 10, 10));
        g.setGradientFill (bz);
        g.fillRoundedRectangle (outer, cornerR);

        // inner chamfer (inset ring)
        auto chamfer = outer.reduced (2.4f);
        ColourGradient ch (Colour::fromRGBA (255, 255, 255, 10), chamfer.getCentreX(), chamfer.getY(),
                           Colour::fromRGBA (0, 0, 0, 120),      chamfer.getCentreX(), chamfer.getBottom(), false);
        g.setGradientFill (ch);
        g.drawRoundedRectangle (chamfer, cornerR - 1.2f, 2.2f);

        // top matte sheen band (very subtle)
        auto topLip = outer.reduced (3.0f).withHeight (outer.getHeight() * 0.20f);
        ColourGradient hl (Colour::fromRGBA (255, 255, 255, 16),
                           topLip.getCentreX(), topLip.getY(),
                           Colour::fromRGBA (255, 255, 255, 0),
                           topLip.getCentreX(), topLip.getBottom(), false);
        g.setGradientFill (hl);
        g.fillRoundedRectangle (topLip, cornerR - 2.0f);

        // hairlines
        g.setColour (frameHi);
        g.drawRoundedRectangle (outer.reduced (1.2f), cornerR, 1.0f);

        g.setColour (frameLo);
        g.drawRoundedRectangle (outer.reduced (3.8f), cornerR - 1.0f, 2.0f);

        // subtle brushed feel (deterministic)
        addMetalHairlines (outer.reduced (3.0f), 0.90f, true,
                           (int) (outer.getWidth() * 73) ^ (int) (outer.getHeight() * 29) ^ 0xA761);
    }

    // =========================================================
    // 2) BOTTOM BAR (matte panel) + screw
    const float barH = outer.getHeight() * barRatio;
    auto bar     = outer.withTop (outer.getBottom() - barH);
    auto topArea = outer.withBottom (bar.getY());

    {
        auto barInner = bar.reduced (3.0f, 2.5f);

        ColourGradient bg (Colour::fromRGB (18, 18, 18), barInner.getCentreX(), barInner.getY(),
                           Colour::fromRGB (6,  6,  6),  barInner.getCentreX(), barInner.getBottom(), false);
        bg.addColour (0.52, Colour::fromRGB (11, 11, 11));
        g.setGradientFill (bg);
        g.fillRoundedRectangle (barInner, cornerR - 3.0f);

        // seam line (engraved)
        g.setColour (Colour::fromRGBA (255, 255, 255, 9));
        g.drawLine (outer.getX() + 10.0f, bar.getY() + 1.0f,
                    outer.getRight() - 10.0f, bar.getY() + 1.0f, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 175));
        g.drawLine (outer.getX() + 10.0f, bar.getY() + 2.4f,
                    outer.getRight() - 10.0f, bar.getY() + 2.4f, 2.2f);

        // micro dust vignette on bar
        fillRoundedClip (barInner, cornerR - 3.0f, [&]
        {
            ColourGradient v (Colour::fromRGBA (0, 0, 0, 55), barInner.getCentreX(), barInner.getBottom(),
                              Colour::fromRGBA (0, 0, 0, 0),  barInner.getCentreX(), barInner.getY(),
                              false);
            g.setGradientFill (v);
            g.fillRect (barInner);
        });

        // screw
        const float screwR = 10.0f;
        Point<float> screwC (outer.getCentreX(),
                             bar.getCentreY() + barH * screwYBias);
        drawScrew (screwC, screwR);
    }

    // =========================================================
    // 3) WINDOW FRAME (inset)
    auto windowOuter = topArea.reduced (bezelInset, bezelInset * 0.78f);
    windowOuter = windowOuter.withTrimmedBottom (frameGap * 0.6f);

    {
        g.setColour (Colour::fromRGBA (0, 0, 0, 160));
        g.fillRoundedRectangle (windowOuter.expanded (2.4f), cornerR - 2.0f);

        ColourGradient fr (Colour::fromRGB (16, 16, 16), windowOuter.getCentreX(), windowOuter.getY(),
                           Colour::fromRGB (5,  5,  5),  windowOuter.getCentreX(), windowOuter.getBottom(), false);
        fr.addColour (0.52, Colour::fromRGB (9, 9, 9));
        g.setGradientFill (fr);
        g.fillRoundedRectangle (windowOuter, cornerR - 3.0f);

        // tight rim lines
        g.setColour (Colour::fromRGBA (255, 255, 255, 10));
        g.drawRoundedRectangle (windowOuter.reduced (0.8f), cornerR - 3.0f, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 200));
        g.drawRoundedRectangle (windowOuter.reduced (2.4f), cornerR - 3.5f, 2.0f);

        // a little brushed inside frame
        addMetalHairlines (windowOuter.reduced (1.6f), 0.55f, true,
                           (int) (windowOuter.getWidth() * 41) ^ (int) (windowOuter.getHeight() * 97) ^ 0xBEE3);
    }

    auto frameInner = windowOuter.reduced (frameGap);
    auto glassRect  = frameInner.reduced (glassInset);

    // =========================================================
    // 4) GLASS + PAPER (less glossy, more “dial behind glass”)
    {
        // base paper fill
        ColourGradient cg (amberHi,  glassRect.getCentreX(), glassRect.getY(),
                           amberLow, glassRect.getCentreX(), glassRect.getBottom(), false);
        cg.addColour (0.55, amberMid);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (glassRect, cornerR - 5.0f);

        // paper vignette + inner edge
        fillRoundedClip (glassRect, cornerR - 5.0f, [&]
        {
            ColourGradient v (Colour::fromRGBA (0, 0, 0, 40), glassRect.getCentreX(), glassRect.getBottom(),
                              Colour::fromRGBA (0, 0, 0, 0),  glassRect.getCentreX(), glassRect.getY(),
                              false);
            g.setGradientFill (v);
            g.fillRect (glassRect);

            ColourGradient v2 (Colour::fromRGBA (0, 0, 0, 26), glassRect.getX(), glassRect.getCentreY(),
                               Colour::fromRGBA (0, 0, 0, 0),  glassRect.getCentreX(), glassRect.getCentreY(),
                               false);
            g.setGradientFill (v2);
            g.fillRect (glassRect);

            ColourGradient v3 (Colour::fromRGBA (0, 0, 0, 22), glassRect.getRight(), glassRect.getCentreY(),
                               Colour::fromRGBA (0, 0, 0, 0),  glassRect.getCentreX(), glassRect.getCentreY(),
                               false);
            g.setGradientFill (v3);
            g.fillRect (glassRect);
        });

        // minimal glass highlight (smaller)
        fillRoundedClip (glassRect, cornerR - 5.0f, [&]
        {
            auto sweep = glassRect.reduced (8.0f)
                                  .withTrimmedBottom (glassRect.getHeight() * 0.64f)
                                  .translated (-glassRect.getWidth() * 0.04f, 0.0f);

            ColourGradient sh (Colour::fromRGBA (255, 255, 255, 12),
                               sweep.getX(), sweep.getY(),
                               Colour::fromRGBA (255, 255, 255, 0),
                               sweep.getRight(), sweep.getBottom(), false);
            g.setGradientFill (sh);
            g.fillRoundedRectangle (sweep, cornerR - 6.0f);

            Path streak;
            auto sr = glassRect.reduced (12.0f);
            streak.startNewSubPath (sr.getX(), sr.getY() + sr.getHeight() * 0.18f);
            streak.lineTo (sr.getRight(), sr.getY() + sr.getHeight() * 0.06f);

            g.setColour (Colour::fromRGBA (255, 255, 255, 10));
            g.strokePath (streak, PathStrokeType (1.1f));
        });

        // paper grain
        addPaperGrain (glassRect, 0.90f);

        // inner edge line
        g.setColour (Colour::fromRGBA (0, 0, 0, 95));
        g.drawRoundedRectangle (glassRect.reduced (0.9f), cornerR - 5.0f, 1.2f);
    }

    // =========================================================
    // 5) CLIP: dial harus di dalam kaca
    {
        Path glassClip;
        glassClip.addRoundedRectangle (glassRect, cornerR - 5.0f);

        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (glassClip);

        // =====================================================
        // DIAL GEOMETRY (ELLIPSE) --- FIXED (jangan ubah)
        auto dialRect = glassRect.reduced (12.0f, 10.0f)
                                .withTrimmedRight (10.0f)
                                .withTrimmedBottom (10.0f);

        const float xNudge = dialRect.getWidth() * xNudgeMul;
        const float rx = dialRect.getWidth()  * rxMul;
        const float ry = rx * yScale;

        const float pivotUp = dialRect.getHeight() * pivotUpRatio;

        const Point<float> centre (dialRect.getCentreX() + xNudge,
                                   dialRect.getBottom() - pivotUp);

        auto unitToAngle = [startAng, endAng] (float u01)
        {
            u01 = jlimit (0.0f, 1.0f, u01);
            return jmap (u01, startAng, endAng);
        };

        auto dbToUnit01 = [] (float db)
        {
            return jlimit (0.0f, 20.0f, db) / 20.0f;
        };

        auto polarR = [&] (float a, float scale)
        {
            return Point<float> (centre.x + std::cos (a) * (rx * scale),
                                 centre.y + std::sin (a) * (ry * scale));
        };

        const PathStrokeType shadowStroke (arcShadow, PathStrokeType::curved, PathStrokeType::rounded);
        const PathStrokeType mainStroke   (arcStroke,  PathStrokeType::curved, PathStrokeType::rounded);

        // =====================================================
        // ARC + RED ZONE  --- FIXED geometry (jangan ubah)
        {
            scaleArc.clear();

            const int steps = 220;

            scaleArc.startNewSubPath (polarR (startAng, 0.985f));
            for (int i = 1; i <= steps; ++i)
            {
                const float t = (float) i / (float) steps;
                const float a = jmap (t, startAng, endAng);
                scaleArc.lineTo (polarR (a, 0.985f));
            }

            // arc shadow + main arc (printed)
            g.setColour (Colour::fromRGBA (0, 0, 0, 40));
            g.strokePath (scaleArc, shadowStroke, AffineTransform::translation (0.35f, 0.35f));

            g.setColour (inkDim.withAlpha (0.90f));
            g.strokePath (scaleArc, mainStroke);

            // red zone (last 25%) --- FIXED geometry
            const float redStartAng = jmap (0.75f, startAng, endAng);

            Path redArc;
            redArc.startNewSubPath (polarR (redStartAng, 1.0f));
            for (int i = 1; i <= steps; ++i)
            {
                const float t = (float) i / (float) steps;
                const float a = jmap (t, redStartAng, endAng);
                redArc.lineTo (polarR (a, 1.0f));
            }

            g.setColour (Colour::fromRGBA (150, 35, 25, 125));
            g.strokePath (redArc, mainStroke);
        }

        // =====================================================
        // TICKS  --- FIXED geometry (jangan ubah)
        {
            const int totalTicks = 11; // 0..10

            auto isMajorIndex = [] (int i)
            {
                return (i == 0 || i == 1 || i == 3 || i == 5 || i == 7 || i == 10);
            };

            for (int i = 0; i < totalTicks; ++i)
            {
                const float u = (float) i / (float) (totalTicks - 1);
                const float a = unitToAngle (u);
                const bool major = isMajorIndex (i);

                const float outerS = 0.955f; // FIXED
                const float innerS = outerS - (major ? 0.16f : 0.11f);

                auto p1 = polarR (a, outerS);
                auto p2 = polarR (a, innerS);

                g.setColour (Colour::fromRGBA (0, 0, 0, major ? 70 : 46));
                g.drawLine (p1.x + 0.45f, p1.y + 0.45f, p2.x + 0.45f, p2.y + 0.45f,
                            major ? 2.1f : 1.3f);

                g.setColour (inkDark.withAlpha (major ? 0.95f : 0.72f));
                g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 1.9f : 1.1f);
            }
        }

        // =====================================================
        // NUMBERS + small labels (printed)
        {
            g.setFont (AuricHelpers::makeFont (15.0f, Font::bold));

            auto drawNum = [&] (int value, float u)
            {
                const float a  = unitToAngle (u);
                const float rr = 0.70f; // FIXED
                auto p = polarR (a, rr);

                Rectangle<float> tr (0, 0, 26, 18);
                tr.setCentre (p);

                if (value == 10)
                    tr = tr.translated (-7.0f, -3.0f);

                g.setColour (Colour::fromRGBA (0, 0, 0, 38));
                g.drawFittedText (String (value), tr.translated (0.35f, 0.35f).toNearestInt(),
                                  Justification::centred, 1);

                g.setColour (inkDark.withAlpha (0.96f));
                g.drawFittedText (String (value), tr.toNearestInt(),
                                  Justification::centred, 1);

                g.setColour (Colour::fromRGBA (255, 255, 255, 8));
                g.drawFittedText (String (value), tr.translated (-0.25f, -0.25f).toNearestInt(),
                                  Justification::centred, 1);
            };

            drawNum (0,  0.0f);
            drawNum (1,  0.1f);
            drawNum (3,  0.3f);
            drawNum (5,  0.5f);
            drawNum (7,  0.7f);
            drawNum (10, 1.0f);

            auto topBand = glassRect.withTrimmedBottom (glassRect.getHeight() * 0.55f);

            g.setFont (AuricHelpers::makeFont (13.0f, Font::bold));
            g.setColour (inkDim.withAlpha (0.95f));
            g.drawText ("GR", topBand.removeFromLeft (40).toNearestInt(),
                        Justification::centred, false);

            g.setColour (Colour::fromRGBA (150, 35, 25, 145));
            g.drawText ("R", topBand.removeFromRight (30).toNearestInt(),
                        Justification::centred, false);
        }

        // =====================================================
        // Big "GR" (printed)
        {
            g.setFont (AuricHelpers::makeFont (26.0f, Font::bold));

            auto t = glassRect.withTrimmedTop (glassRect.getHeight() * 0.36f)
                              .withTrimmedBottom (glassRect.getHeight() * 0.18f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 48));
            g.drawText ("GR", t.translated (0.55f, 0.55f).toNearestInt(),
                        Justification::centred, false);

            g.setColour (inkDark.withAlpha (0.95f));
            g.drawText ("GR", t.toNearestInt(), Justification::centred, false);

            g.setColour (Colour::fromRGBA (255, 255, 255, 8));
            g.drawText ("GR", t.translated (-0.25f, -0.25f).toNearestInt(),
                        Justification::centred, false);
        }

        // =====================================================
        // NEEDLE (matte metal, geometry FIXED)
        const float grDb = jlimit (0.0f, 20.0f, currentDb);
        const float needleAng = unitToAngle (dbToUnit01 (grDb));

        const float needleScale = 0.92f; // FIXED
        const auto needleEnd = polarR (needleAng, needleScale);

        // shadow
        g.setColour (Colour::fromRGBA (0, 0, 0, 115));
        g.drawLine (centre.x + 1.0f, centre.y + 1.0f,
                    needleEnd.x + 1.0f, needleEnd.y + 1.0f, 3.2f);

        // main needle
        {
            Path needle;
            needle.startNewSubPath (centre);
            needle.lineTo (needleEnd);

            g.setColour (Colour::fromRGBA (14, 14, 14, 245));
            g.strokePath (needle, PathStrokeType (2.45f, PathStrokeType::curved, PathStrokeType::rounded));

            g.setColour (Colour::fromRGBA (255, 255, 255, 30));
            g.strokePath (needle, PathStrokeType (1.00f, PathStrokeType::curved, PathStrokeType::rounded),
                          AffineTransform::translation (-0.55f, -0.55f));
        }

        // tip bead
        {
            Rectangle<float> tip (needleEnd.x - 2.2f, needleEnd.y - 2.2f, 4.4f, 4.4f);
            g.setColour (Colour::fromRGBA (0, 0, 0, 110));
            g.fillEllipse (tip.translated (0.6f, 0.6f));

            g.setColour (Colour::fromRGBA (18, 18, 18, 240));
            g.fillEllipse (tip);

            g.setColour (Colour::fromRGBA (255, 255, 255, 24));
            g.drawEllipse (tip, 0.8f);
        }

        // pivot cap (more metallic, still matte)
        {
            Rectangle<float> hub (centre.x - 10.0f, centre.y - 10.0f, 20.0f, 20.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 125));
            g.fillEllipse (hub.expanded (1.8f).translated (0.6f, 0.8f));

            ColourGradient hubGrad (Colour::fromRGB (38, 38, 38), hub.getCentreX(), hub.getY(),
                                    Colour::fromRGB (9,  9,  9),  hub.getCentreX(), hub.getBottom(), false);
            hubGrad.addColour (0.55, Colour::fromRGB (18, 18, 18));
            g.setGradientFill (hubGrad);
            g.fillEllipse (hub);

            g.setColour (Colour::fromRGBA (255, 255, 255, 18));
            g.drawEllipse (hub, 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 230));
            g.drawEllipse (hub.reduced (1.2f), 1.0f);

            auto spec = hub.reduced (5.0f).translated (-2.0f, -2.0f);
            g.setColour (Colour::fromRGBA (255, 255, 255, 10));
            g.fillEllipse (spec);
        }
    }
}
