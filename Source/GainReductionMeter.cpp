//==============================================================================
// GainReductionMeter.cpp  (AURIC Ω76) — Responsive GR meter (S/M/L) (NO AuricTheme)
//  - All pixels: scaled by component size (s)
//  - Fonts, strokes, offsets: scaled
//  - No AuricTheme dependency
//==============================================================================

#include "GainReductionMeter.h"
#include "AuricHelpers.h"

#include <cmath>
#include <functional>

//==============================================================================
// Local helpers + palette (NO AuricTheme)
namespace
{
    using namespace juce;

    static Font withTracking (Font f, float trackingPx)
    {
        const float denom  = jmax (1.0f, f.getHeight());
        const float factor = trackingPx / denom;
       #if defined (JUCE_MAJOR_VERSION) && JUCE_MAJOR_VERSION >= 7
        return f.withExtraKerningFactor (factor);
       #else
        f.setExtraKerningFactor (factor);
        return f;
       #endif
    }

    static void clipRounded (Graphics& g, Rectangle<float> area, float cr, const std::function<void()>& fn)
    {
        Graphics::ScopedSaveState ss (g);
        Path p; p.addRoundedRectangle (area, cr);
        g.reduceClipRegion (p);
        fn();
    }

    // very subtle paper/backlight grain (keep it LOW, so it doesn't look "dirty")
    static void addSoftGrain (Graphics& g, Rectangle<float> area, float cr, float strength01, int seed)
    {
        strength01 = jlimit (0.0f, 1.0f, strength01);
        if (strength01 <= 0.001f) return;

        Random rng ((int64) seed);

        clipRounded (g, area, cr, [&]
        {
            const int dots = jlimit (80, 220, (int) (area.getWidth() * 0.55f));
            for (int i = 0; i < dots; ++i)
            {
                const float x = area.getX() + rng.nextFloat() * area.getWidth();
                const float y = area.getY() + rng.nextFloat() * area.getHeight();

                const bool bright = rng.nextFloat() > 0.62f;
                const float a = (bright ? (0.010f + 0.020f * rng.nextFloat())
                                        : (0.008f + 0.018f * rng.nextFloat())) * strength01;

                const float sz = jmap (rng.nextFloat(), 0.6f, 1.35f);
                g.setColour ((bright ? Colours::white : Colours::black).withAlpha (a));
                g.fillEllipse (x - sz * 0.5f, y - sz * 0.5f, sz, sz);
            }
        });
    }

    namespace GRPal
    {
        // amber screen
        static inline Colour amberHi()  { return Colour::fromRGB (255, 240, 62); }
        static inline Colour amberMid() { return Colour::fromRGB (201, 121, 41); }
        static inline Colour amberLow() { return Colour::fromRGB (110,  68, 30); }

        // bezel
        static inline Colour bezelHi()  { return Colour::fromRGB (36, 36, 34); }
        static inline Colour bezelMid() { return Colour::fromRGB (18, 18, 18); }
        static inline Colour bezelLo()  { return Colour::fromRGB ( 8,  8,  8); }

        // ink
        static inline Colour inkDark()  { return Colour::fromRGBA (18, 12, 10, 210); }
        static inline Colour inkDim()   { return Colour::fromRGBA (18, 12, 10, 150); }

        // red zone
        static inline Colour redInk()   { return Colour::fromRGBA (150, 35, 25, 160); }
        static inline Colour redDim()   { return Colour::fromRGBA (150, 35, 25, 110); }
    }
}

//==============================================================================

GainReductionMeterComponent::GainReductionMeterComponent()
{
    setOpaque (false);
}

void GainReductionMeterComponent::setGainReductionDb (float db)
{
    const float gr = std::abs (db);
    targetDb = juce::jlimit (0.0f, maxDb, gr);

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
    const float ms    = moreGR ? attackCoeffMs : releaseCoeffMs;

    const float tau = juce::jmax (0.001f, ms * 0.001f);
    const float a   = std::exp (-dt / tau);
    cur = tgt + (cur - tgt) * a;

    currentDb = cur;
    repaint();
}

//==============================================================================
// Paint — Responsive GR meter (ellipse arc, pivot bottom-ish center)
//==============================================================================
void GainReductionMeterComponent::paint (juce::Graphics& g)
{
    using namespace juce;

    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    auto full = getLocalBounds().toFloat();
    if (full.getWidth() < 80.0f || full.getHeight() < 50.0f)
        return;

    // ============================================================
    // RESPONSIVE SCALE (anchor = "M" size)
    // Adjust refW/refH once if your meterHousing "M" differs.
    // ============================================================
    constexpr float refW = 430.0f;
    constexpr float refH = 250.0f;

    const float sx = full.getWidth()  / refW;
    const float sy = full.getHeight() / refH;
    const float s  = jlimit (0.80f, 1.25f, std::min (sx, sy));

    auto S = [s] (float v) { return v * s; };

    full = full.reduced (S(1.0f));

    // =========================
    // OUTER BEZEL (hardware)
    // =========================
    const float cornerR = jlimit (S(10.0f), S(18.0f), full.getHeight() * 0.12f);

    // soft cast shadow
    {
        auto sh = full.expanded (S(3.0f)).translated (0.0f, S(2.0f));
        ColourGradient sg (Colours::black.withAlpha (0.00f), sh.getCentreX(), sh.getY(),
                           Colours::black.withAlpha (0.40f), sh.getCentreX(), sh.getBottom(), false);
        sg.addColour (0.60, Colours::black.withAlpha (0.18f));
        g.setGradientFill (sg);
        g.fillRoundedRectangle (sh, cornerR + S(2.0f));
    }

    // bezel body
    {
        ColourGradient bg (GRPal::bezelHi(), full.getCentreX(), full.getY(),
                           GRPal::bezelLo(), full.getCentreX(), full.getBottom(), false);
        bg.addColour (0.52, GRPal::bezelMid());
        g.setGradientFill (bg);
        g.fillRoundedRectangle (full, cornerR);

        g.setColour (Colour::fromRGBA (255, 255, 255, 10));
        g.drawRoundedRectangle (full.reduced (S(0.8f)), cornerR - S(0.8f), S(1.0f));

        g.setColour (Colour::fromRGBA (0, 0, 0, 210));
        g.drawRoundedRectangle (full.reduced (S(1.6f)), cornerR - S(1.4f), S(1.3f));
    }

    // =========================
    // INNER SCREEN WELL
    // =========================
    auto screen = full.reduced (S(4.2f));
    const float screenR = jmax (S(5.0f), cornerR - S(5.0f));

    {
        ColourGradient well (Colour::fromRGB (12, 12, 12), screen.getCentreX(), screen.getY(),
                             Colour::fromRGB (0,  0,  0),  screen.getCentreX(), screen.getBottom(), false);
        well.addColour (0.55, Colour::fromRGB (7, 7, 7));
        g.setGradientFill (well);
        g.fillRoundedRectangle (screen, screenR);

        g.setColour (Colour::fromRGBA (255, 255, 255, 8));
        g.drawRoundedRectangle (screen.reduced (S(0.6f)), screenR - S(0.6f), S(1.0f));

        g.setColour (Colour::fromRGBA (0, 0, 0, 220));
        g.drawRoundedRectangle (screen.reduced (S(1.4f)), screenR - S(1.2f), S(1.2f));
    }

    // =========================
    // AMBER PLATE (usable area)
    // =========================
    auto plate = screen.reduced (S(2.0f));
    const float plateR = jmax (S(5.0f), screenR - S(1.6f));

    {
        ColourGradient cg (GRPal::amberHi(), plate.getCentreX(), plate.getY(),
                           GRPal::amberLow(), plate.getCentreX(), plate.getBottom(), false);
        cg.addColour (0.56, GRPal::amberMid());
        g.setGradientFill (cg);
        g.fillRoundedRectangle (plate, plateR);

        // subtle vignette bottom + glare band
        clipRounded (g, plate, plateR, [&]
        {
            ColourGradient v (Colour::fromRGBA (0, 0, 0, 55),
                              plate.getCentreX(), plate.getBottom(),
                              Colour::fromRGBA (0, 0, 0, 0),
                              plate.getCentreX(), plate.getY(), false);
            g.setGradientFill (v);
            g.fillRect (plate);

            ColourGradient glare (Colour::fromRGBA (255, 255, 255, 18),
                                  plate.getX(), plate.getY(),
                                  Colour::fromRGBA (255, 255, 255, 0),
                                  plate.getRight(), plate.getCentreY(), false);
            g.setGradientFill (glare);
            g.fillRect (plate.withHeight (plate.getHeight() * 0.48f));
        });

        const int seed = ((int) plate.getWidth() * 193) ^ ((int) plate.getHeight() * 997) ^ 0x76A11C0F;
        addSoftGrain (g, plate, plateR, 0.22f, seed);

        g.setColour (Colour::fromRGBA (0, 0, 0, 80));
        g.drawRoundedRectangle (plate.reduced (S(0.8f)), plateR - S(0.8f), S(1.0f));
    }

    // =========================
    // CLIP to PLATE (all dial inside)
    // =========================
    clipRounded (g, plate, plateR, [&]
    {
        // dialRect (scaled padding)
        auto dialRect = plate.reduced (S(10.0f), S(8.0f));
        dialRect = dialRect.withTrimmedBottom (S(8.0f));

        // ---- Dial geometry
        // rx based on width; ry “taller” so arc fills top area better
        const float rx = dialRect.getWidth() * 0.545f;
        const float ry = rx * 0.78f;

        // pivot a bit up (but still bottom-ish)
        const Point<float> centre (dialRect.getCentreX(),
                                   dialRect.getBottom() - dialRect.getHeight() * 0.095f);

        // ROTATE/tilt arc slightly (scaled independent)
        const float rot = MathConstants<float>::pi * (-7.0f / 180.0f); // -7°
        const float startAng = (-MathConstants<float>::pi * 0.998f) + rot;
        const float endAng   = ( MathConstants<float>::pi * 0.028f) + rot;

        auto unitToAngle = [startAng, endAng] (float u01)
        {
            u01 = jlimit (0.0f, 1.0f, u01);
            return jmap (u01, startAng, endAng);
        };

        auto dbToUnit01 = [this] (float db)
        {
            const float m = juce::jmax (1.0f, maxDb);
            return jlimit (0.0f, m, db) / m;
        };

        auto polarR = [&] (float a, float sx, float sy)
        {
            return Point<float> (centre.x + std::cos (a) * (rx * sx),
                                 centre.y + std::sin (a) * (ry * sy));
        };

        // =========================
        // MAIN ARC + RED ZONE ARC
        // =========================
        {
            Path scaleArc;
            const int steps = 260;

            scaleArc.startNewSubPath (polarR (startAng, 0.99f, 0.99f));
            for (int i = 1; i <= steps; ++i)
            {
                const float t = (float) i / (float) steps;
                const float a = jmap (t, startAng, endAng);
                scaleArc.lineTo (polarR (a, 0.99f, 0.99f));
            }

            // arc shadow
            g.setColour (Colour::fromRGBA (0, 0, 0, 55));
            g.strokePath (scaleArc,
                          PathStrokeType (S(2.2f), PathStrokeType::curved, PathStrokeType::rounded),
                          AffineTransform::translation (S(0.35f), S(0.45f)));

            // arc ink
            g.setColour (GRPal::inkDim().withAlpha (0.88f));
            g.strokePath (scaleArc,
                          PathStrokeType (S(1.9f), PathStrokeType::curved, PathStrokeType::rounded));

            // red zone from 15 -> max
            const float redStartU   = (15.0f / juce::jmax (1.0f, maxDb));
            const float redStartAng = unitToAngle (redStartU);

            Path redArc;
            redArc.startNewSubPath (polarR (redStartAng, 0.99f, 0.99f));
            for (int i = 1; i <= steps; ++i)
            {
                const float t = (float) i / (float) steps;
                const float a = jmap (t, redStartAng, endAng);
                redArc.lineTo (polarR (a, 0.99f, 0.99f));
            }

            g.setColour (GRPal::redInk());
            g.strokePath (redArc,
                          PathStrokeType (S(2.0f), PathStrokeType::curved, PathStrokeType::rounded));
        }

        // =========================
        // TICKS
        // =========================
        {
            const float m = juce::jmax (1.0f, maxDb);
            const int steps = (int) std::round (m * 2.0f); // 0.5 dB
            const float outerS = 0.965f;

            for (int i = 0; i <= steps; ++i)
            {
                const float v = (float) i * 0.5f;
                const float u = dbToUnit01 (v);
                const float a = unitToAngle (u);

                const bool major = (std::fmod (v, 5.0f) < 0.001f);
                const bool mid   = (! major) && (std::fmod (v, 1.0f) < 0.001f);

                const float len = major ? 0.195f : (mid ? 0.145f : 0.105f);
                const float innerS = outerS - len;

                auto p1 = polarR (a, outerS, outerS);
                auto p2 = polarR (a, innerS, innerS);

                const bool redZone = (v >= 15.0f);

                // engraved shadow
                g.setColour (Colour::fromRGBA (0, 0, 0, major ? 70 : 45));
                g.drawLine (p1.x + S(0.35f), p1.y + S(0.35f),
                            p2.x + S(0.35f), p2.y + S(0.35f),
                            major ? S(2.0f) : (mid ? S(1.4f) : S(1.05f)));

                // main ink
                Colour ink = redZone ? (major ? GRPal::redInk() : GRPal::redDim())
                                     : GRPal::inkDark().withAlpha (major ? 0.90f : (mid ? 0.70f : 0.55f));

                g.setColour (ink);
                g.drawLine (p1.x, p1.y, p2.x, p2.y,
                            major ? S(1.75f) : (mid ? S(1.15f) : S(0.85f)));
            }
        }

        // =========================
        // NUMBERS
        // =========================
        {
            auto fNum = withTracking (AuricHelpers::makeFont (S(13.0f), Font::plain), S(0.2f));
            g.setFont (fNum);

            auto drawNum = [&] (float v, Colour col)
            {
                const float a  = unitToAngle (dbToUnit01 (v));
                const float rr = 0.68f;

                auto p = polarR (a, rr, rr);

                Rectangle<float> tr (0.0f, 0.0f, S(28.0f), S(16.0f));
                tr.setCentre (p);

                g.setColour (Colour::fromRGBA (0, 0, 0, 35));
                g.drawFittedText (String ((int) v), tr.translated (S(0.30f), S(0.30f)).toNearestInt(),
                                  Justification::centred, 1);

                g.setColour (col);
                g.drawFittedText (String ((int) v), tr.toNearestInt(),
                                  Justification::centred, 1);
            };

            drawNum (0.0f,  GRPal::inkDark().withAlpha (0.78f));
            drawNum (5.0f,  GRPal::inkDark().withAlpha (0.78f));
            drawNum (10.0f, GRPal::inkDark().withAlpha (0.78f));
            drawNum (15.0f, GRPal::redInk());
            drawNum (20.0f, GRPal::redInk());
        }

        // =========================
        // TOP CORNER LEGENDS (GR / R)
        // =========================
        {
            auto topBand = plate.withTrimmedBottom (plate.getHeight() * 0.78f);

            auto fSmall = withTracking (AuricHelpers::makeFont (S(12.0f), Font::bold), S(0.35f));
            g.setFont (fSmall);

            g.setColour (GRPal::inkDim().withAlpha (0.92f));
            g.drawText ("GR", topBand.removeFromLeft ((int) S(34.0f)).toNearestInt(),
                        Justification::centred, false);

            g.setColour (GRPal::redInk());
            g.drawText ("R", topBand.removeFromRight ((int) S(26.0f)).toNearestInt(),
                        Justification::centred, false);
        }

        // =========================
        // CENTER TEXT
        // =========================
        {
            auto fBig = withTracking (AuricHelpers::makeFont (S(22.0f), Font::bold), S(0.15f));
            g.setFont (fBig);

            auto t = plate.withTrimmedTop (plate.getHeight() * 0.42f)
                         .withTrimmedBottom (plate.getHeight() * 0.26f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 38));
            g.drawText ("GR", t.translated (S(0.45f), S(0.45f)).toNearestInt(),
                        Justification::centred, false);

            g.setColour (GRPal::inkDark().withAlpha (0.75f));
            g.drawText ("GR", t.toNearestInt(), Justification::centred, false);

            auto fSub = withTracking (AuricHelpers::makeFont (S(10.0f), Font::plain), S(0.10f));
            g.setFont (fSub);
            g.setColour (GRPal::inkDim().withAlpha (0.60f));
            g.drawText ("GAIN REDUCTION", t.translated (0, (int) S(18.0f)).toNearestInt(),
                        Justification::centredTop, false);
        }

        // =========================
        // BOTTOM STRIP LABEL
        // =========================
        {
            auto strip = plate.withTrimmedTop (plate.getHeight() * 0.84f);
            g.setColour (Colour::fromRGBA (0, 0, 0, 22));
            g.fillRect (strip);

            g.setColour (Colour::fromRGBA (255, 255, 255, 10));
            g.drawLine (strip.getX(), strip.getY() + S(1.0f),
                        strip.getRight(), strip.getY() + S(1.0f), S(1.0f));

            auto f = withTracking (AuricHelpers::makeFont (S(10.0f), Font::plain), S(0.15f));
            g.setFont (f);
            g.setColour (GRPal::inkDim().withAlpha (0.55f));
            g.drawText ("GAIN REDUCTION (dB)", strip.toNearestInt(), Justification::centred, false);
        }

        // =========================
        // NEEDLE
        // =========================
        {
            const float grDb = jlimit (0.0f, maxDb, currentDb);
            const float needleAng = unitToAngle (dbToUnit01 (grDb));

            const float needleScale = 0.925f;
            const auto needleEnd = polarR (needleAng, needleScale, needleScale);

            // pivot base
            const float ringR = jlimit (S(6.0f), S(7.5f), plate.getHeight() * 0.030f);
            auto ring = Rectangle<float> (centre.x - ringR, centre.y - ringR, ringR * 2.0f, ringR * 2.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 110));
            g.fillEllipse (ring.expanded (S(1.35f)).translated (S(0.20f), S(0.55f)));

            {
                ColourGradient ringG (Colour::fromRGB (30, 30, 30), ring.getCentreX(), ring.getY(),
                                      Colour::fromRGB (10, 10, 10), ring.getCentreX(), ring.getBottom(), false);
                ringG.addColour (0.55, Colour::fromRGB (18, 18, 18));
                g.setGradientFill (ringG);
                g.fillEllipse (ring);

                g.setColour (Colour::fromRGBA (255, 255, 255, 12));
                g.drawEllipse (ring.reduced (S(0.4f)), S(1.0f));

                g.setColour (Colour::fromRGBA (0, 0, 0, 205));
                g.drawEllipse (ring.reduced (S(0.9f)), S(1.0f));
            }

            // needle shadow
            g.setColour (Colour::fromRGBA (0, 0, 0, 115));
            g.drawLine (centre.x + S(1.0f), centre.y + S(1.0f),
                        needleEnd.x + S(1.0f), needleEnd.y + S(1.0f), S(3.0f));

            // needle main
            Path needle;
            needle.startNewSubPath (centre);
            needle.lineTo (needleEnd);

            g.setColour (Colour::fromRGBA (14, 14, 14, 235));
            g.strokePath (needle, PathStrokeType (S(2.35f), PathStrokeType::curved, PathStrokeType::rounded));

            // highlight
            const float hx = -std::sin (needleAng) * S(0.70f);
            const float hy =  std::cos (needleAng) * S(0.70f);

            g.setColour (Colour::fromRGBA (255, 255, 255, 70));
            g.drawLine (centre.x + hx, centre.y + hy,
                        needleEnd.x + hx, needleEnd.y + hy, S(0.85f));

            // cap
            const float capR = ringR * 0.62f;
            auto cap = Rectangle<float> (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

            ColourGradient capG (Colour::fromRGB (34, 34, 34), cap.getCentreX(), cap.getY(),
                                 Colour::fromRGB (12, 12, 12), cap.getCentreX(), cap.getBottom(), false);
            capG.addColour (0.55, Colour::fromRGB (22, 22, 22));
            g.setGradientFill (capG);
            g.fillEllipse (cap);

            g.setColour (Colour::fromRGBA (0, 0, 0, 165));
            g.drawEllipse (cap.reduced (S(0.8f)), S(1.0f));
        }
    });

    // =========================
    // GLASS (very subtle)
    // =========================
    {
        auto glass = screen.reduced (S(0.8f));
        const float gr = jmax (S(6.0f), screenR - S(0.8f));

        clipRounded (g, glass, gr, [&]
        {
            g.addTransform (AffineTransform::rotation (-0.45f, glass.getCentreX(), glass.getCentreY()));

            auto band = glass.expanded (glass.getWidth() * 0.35f)
                             .translated (-glass.getWidth() * 0.10f, -glass.getHeight() * 0.30f);

            ColourGradient sheen (Colour::fromRGBA (255,255,255,0),
                                  band.getCentreX(), band.getY(),
                                  Colour::fromRGBA (255,255,255,0),
                                  band.getCentreX(), band.getBottom(), false);
            sheen.addColour (0.38, Colour::fromRGBA (255,255,255,10));
            sheen.addColour (0.52, Colour::fromRGBA (255,255,255,0));
            g.setGradientFill (sheen);
            g.fillRect (band);
        });

        g.setColour (Colour::fromRGBA (255, 255, 255, 9));
        g.drawRoundedRectangle (glass, gr, S(1.0f));
    }
}
