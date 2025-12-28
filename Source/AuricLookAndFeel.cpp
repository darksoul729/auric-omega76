#include "AuricLookAndFeel.h"
#include "AuricHelpers.h"
#include <cmath>

//==============================================================================
AuricLookAndFeel::AuricLookAndFeel() {}

//==============================================================================
// Local palette + helpers (MINIMAL, penting saja)
namespace
{
    inline juce::Colour hexRGB (int rgb)
    {
        return juce::Colour::fromRGB ((juce::uint8) ((rgb >> 16) & 0xFF),
                                      (juce::uint8) ((rgb >>  8) & 0xFF),
                                      (juce::uint8) ((rgb      ) & 0xFF));
    }

    // ====== panel palette (match reference: warm black + soft gold) ======
    inline juce::Colour bgDark()   { return hexRGB (0x171712); }
    inline juce::Colour midDark()  { return hexRGB (0x22221B); }
    inline juce::Colour rimHi()    { return hexRGB (0x3A3A32); }
    inline juce::Colour rimLo()    { return hexRGB (0x070706); }

    inline juce::Colour gold()     { return hexRGB (0xD6BE87); }
    inline juce::Colour goldHi()   { return hexRGB (0xF3E4B2); }

    inline juce::Colour needleCol(){ return juce::Colour::fromRGB (226, 195, 129); } // gold needle

    inline float clampf (float v, float lo, float hi) { return juce::jlimit (lo, hi, v); }

    inline juce::Point<float> polar (juce::Point<float> c, float ang, float r)
    {
        return { c.x + std::cos (ang) * r,
                 c.y + std::sin (ang) * r };
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
        return (t == "save" || t == "load" || t == "del" || t == "s" || t == "m" || t == "l" || t == "i");
    }

    inline bool isHeaderPresetBox (const juce::ComboBox& c)
    {
        const auto id = c.getComponentID();
        return id == "hdr_preset_box" || id == "hdr_quality_box";
    }

    static juce::Font withTracking (juce::Font f, float trackingPx)
    {
        const float denom  = juce::jmax (1.0f, f.getHeight());
        const float factor = trackingPx / denom;
       #if defined (JUCE_MAJOR_VERSION) && JUCE_MAJOR_VERSION >= 7
        return f.withExtraKerningFactor (factor);
       #else
        f.setExtraKerningFactor (factor);
        return f;
       #endif
    }

    static inline void embossText (juce::Graphics& g,
                                   const juce::String& text,
                                   juce::Rectangle<int> bounds,
                                   juce::Font font,
                                   juce::Colour main,
                                   juce::Justification just = juce::Justification::centred)
    {
        using namespace juce;

        g.setFont (font);

        g.setColour (Colour::fromRGBA (0, 0, 0, 135));
        g.drawText (text, bounds.translated (1, 1), just, false);

        g.setColour (Colour::fromRGBA (255, 255, 255, 18));
        g.drawText (text, bounds.translated (-1, -1), just, false);

        g.setColour (main);
        g.drawText (text, bounds, just, false);
    }

    static inline void addMicroBrushed (juce::Graphics& g, juce::Rectangle<float> rr, float alpha, int seed)
    {
        using namespace juce;

        if (alpha <= 0.0001f || rr.getWidth() < 8.0f || rr.getHeight() < 6.0f)
            return;

        Graphics::ScopedSaveState ss (g);

        Path clip;
        clip.addRoundedRectangle (rr, rr.getHeight() * 0.35f);
        g.reduceClipRegion (clip);

        Random rng ((int64) (seed ^ 0x51C0FFEE));
        g.setColour (Colour::fromRGBA (255, 255, 255,
                                       (uint8) jlimit (0, 255, (int) (alpha * 255.0f))));

        const int lines = jlimit (8, 18, (int) (rr.getHeight() * 0.75f));
        for (int i = 0; i < lines; ++i)
        {
            const float y   = rr.getY() + (i + 0.5f) * (rr.getHeight() / (float) lines);
            const float wob = (rng.nextFloat() - 0.5f) * 0.9f;
            g.drawLine (rr.getX() + 6.0f, y + wob, rr.getRight() - 6.0f, y + wob, 1.0f);
        }
    }

static void drawAuricDialTicks (juce::Graphics& g,
                                juce::Point<float> centre,
                                float rInner,
                                float startAng,
                                float endAng,
                                bool big)
{
    using namespace juce;

    // Dense + short like reference
    const int   numTicks   = big ? 25 : 23;   // <-- kunci: bikin rapat
    const int   majorEvery = big ? 5  : 5;

    const float lenMinor   = big ? 9.5f : 7.5f;
    const float lenMajor   = big ? 14.5f : 11.5f;

    const float thickMin   = big ? 1.25f : 1.05f;
    const float thickMaj   = big ? 1.55f : 1.25f;

    // Very subtle engraved
    const float shx = big ? 0.40f : 0.32f;
    const float shy = big ? 0.55f : 0.45f;

    // Subtle warm brass (jangan terlalu gold ngejreng)
    const auto cMin = gold().darker(0.25f).withAlpha (0.38f);
    const auto cMaj = gold().withAlpha (0.50f);

    for (int i = 0; i < numTicks; ++i)
    {
        const float t01 = (numTicks <= 1) ? 0.0f : (float) i / (float) (numTicks - 1);
        const float a   = jmap (t01, startAng, endAng);

        const bool  major = (i % majorEvery) == 0;
        const float len   = major ? lenMajor : lenMinor;
        const float th    = major ? thickMaj : thickMin;

        const auto p0 = polar (centre, a, rInner);
        const auto p1 = polar (centre, a, rInner + len);

        Path p;
        p.startNewSubPath (p1);
        p.lineTo (p0);

        // engraved shadow (super halus)
        {
            Path s = p;
            s.applyTransform (AffineTransform::translation (shx, shy));
            g.setColour (Colour::fromRGBA (0, 0, 0, big ? 120 : 105));
            g.strokePath (s, PathStrokeType (th + 0.22f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // main tick
        g.setColour (major ? cMaj : cMin);
        g.strokePath (p, PathStrokeType (th, PathStrokeType::curved, PathStrokeType::rounded));

        // micro highlight (tipis banget)
        {
            Path h = p;
            h.applyTransform (AffineTransform::translation (-shx * 0.65f, -shy * 0.65f));
            g.setColour (Colour::fromRGBA (255, 255, 255, big ? 10 : 8));
            g.strokePath (h, PathStrokeType (th * 0.28f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }
}


    // Ceramic/satin speckle (very subtle, avoid “noisy”)
    static void drawCeramicTexture (juce::Graphics& g,
                                    juce::Rectangle<float> face,
                                    juce::Point<float> centre,
                                    float angleRadians,
                                    float strength01,
                                    int seed)
    {
        using namespace juce;

        strength01 = jlimit (0.0f, 1.0f, strength01);
        if (strength01 <= 0.001f) return;

        Graphics::ScopedSaveState ss (g);

        Path clip;
        clip.addEllipse (face);
        g.reduceClipRegion (clip);

        g.addTransform (AffineTransform::rotation (angleRadians * 0.25f, centre.x, centre.y));

        const float rMax = face.getWidth() * 0.5f;
        if (rMax < 8.0f) return;

        Random rng ((int64) (seed ^ 0x6d2b79f5));

        const int dots = jlimit (80, 220, (int) (rMax * 4.6f));
        for (int i = 0; i < dots; ++i)
        {
            const float u  = rng.nextFloat();
            const float v  = rng.nextFloat();
            const float rr = std::sqrt (u) * rMax * 0.98f;
            const float tt = v * MathConstants<float>::twoPi;

            const float x = centre.x + std::cos (tt) * rr;
            const float y = centre.y + std::sin (tt) * rr;

            const float sz = jmap (rng.nextFloat(), 0.7f, 1.8f) * jmax (1.0f, rMax * 0.010f);
            const bool  br = (rng.nextFloat() > 0.52f);

            const float a = (br ? (0.018f + 0.034f * rng.nextFloat())
                                : (0.014f + 0.030f * rng.nextFloat())) * strength01;

            g.setColour ((br ? Colours::white : Colours::black).withAlpha (a));
            g.fillEllipse (x - sz * 0.5f, y - sz * 0.5f, sz, sz);
        }
    }

    // Shadow that DOESN'T become a “black disk”
    static void drawDirectionalShadowEllipse (juce::Graphics& g,
                                              juce::Rectangle<float> e,
                                              float blurGrow,
                                              juce::Point<float> offset,
                                              float alpha)
    {
        using namespace juce;

        alpha = jlimit (0.0f, 1.0f, alpha);
        if (alpha <= 0.001f) return;

        Graphics::ScopedSaveState ss (g);

        e = e.translated (offset.x, offset.y).expanded (blurGrow);

        Path clip;
        clip.addEllipse (e);
        g.reduceClipRegion (clip);

        ColourGradient cg (Colours::black.withAlpha (0.0f),
                           e.getCentreX(), e.getY(),
                           Colours::black.withAlpha (alpha),
                           e.getCentreX(), e.getBottom(),
                           false);

        cg.addColour (0.58, Colours::black.withAlpha (alpha * 0.58f));
        cg.addColour (0.85, Colours::black.withAlpha (alpha));

        g.setGradientFill (cg);
        g.fillEllipse (e);
    }

    // ===== ring notches (inner cut) like reference =====
    static void drawRingNotches (juce::Graphics& g,
                                 juce::Point<float> c,
                                 float rOuter,
                                 float rInner,
                                 int count,
                                 float startAng,
                                 float endAng,
                                 float alpha)
    {
        using namespace juce;

        alpha = jlimit (0.0f, 1.0f, alpha);
        if (alpha <= 0.001f) return;

        const float range = endAng - startAng;
        if (range <= 0.001f) return;

        const float step = range / (float) count;

        // engraved dark
        g.setColour (Colours::black.withAlpha (alpha * 0.70f));
        for (int i = 0; i < count; ++i)
        {
            const float a = startAng + (i + 0.5f) * step;
            auto p1 = polar (c, a, rInner);
            auto p2 = polar (c, a, rOuter);

            g.drawLine (p1.x, p1.y, p2.x, p2.y, 1.0f);
        }

        // tiny highlight top-left
        g.setColour (Colours::white.withAlpha (alpha * 0.13f));
        for (int i = 0; i < count; ++i)
        {
            const float a = startAng + (i + 0.5f) * step;
            auto p1 = polar (c, a, rInner - 0.35f);
            auto p2 = polar (c, a, rOuter - 0.35f);

            g.drawLine (p1.x, p1.y, p2.x, p2.y, 1.0f);
        }
    }

    // =========================
    // Hardware bar button (SC HPF / PWR) - REALISTIC HARDWARE STYLE
    // Matched with overall dark panel aesthetic
    // =========================
static void drawHardwareBarButton (juce::Graphics& g,
                                   juce::Button& b,
                                   juce::Rectangle<float> r,
                                   bool highlighted,
                                   bool down)
{
    using namespace juce;

    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    // Jangan gambar full-width: bikin tombolnya kecil di tengah seperti ref
    r = r.reduced (2.0f);
    if (r.getWidth() < 20.0f || r.getHeight() < 12.0f) return;

    const bool on = b.getToggleState();
    const auto gld   = gold();
    const auto gldHi = goldHi();

    // =========================
    // SIZE MATCH (gambar kanan)
    // =========================
    // lebar tombol ~60-70% dari slot, tinggi ~70%
    const float btnW = jlimit (70.0f, r.getWidth(),  r.getWidth()  * 0.64f);
    const float btnH = jlimit (30.0f, r.getHeight(), r.getHeight() * 0.99f);

    auto btn = Rectangle<float> (0, 0, btnW, btnH).withCentre (r.getCentre());
    btn = btn.reduced (0.5f);

    const float rr = clampf (btn.getHeight() * 0.18f, 4.0f, 7.0f);
    const float pressY = down ? 1.0f : 0.0f;

    btn = btn.translated (0.0f, pressY);

    // Layer thickness
    const float bezel = clampf (btn.getHeight() * 0.10f, 1.6f, 2.6f);
    auto outer = btn;
    auto well  = btn.reduced (bezel);
    auto face  = well.reduced (2.0f);

    // =========================
    // 1) SHADOW (subtle)
    // =========================
    {
        g.setColour (Colour::fromRGBA (0, 0, 0, 120));
        g.fillRoundedRectangle (outer.translated (0.0f, 1.6f), rr);

        g.setColour (Colour::fromRGBA (0, 0, 0, 70));
        g.fillRoundedRectangle (outer.expanded (1.2f).translated (0.0f, 2.0f), rr + 1.2f);
    }

    // =========================
    // 2) OUTER FRAME (thin, machined)
    // =========================
    {
        ColourGradient bezelG (Colour::fromRGB (34, 33, 29), outer.getCentreX(), outer.getY(),
                               Colour::fromRGB (11, 10, 9),  outer.getCentreX(), outer.getBottom(), false);
        bezelG.addColour (0.55, Colour::fromRGB (18, 17, 15));
        g.setGradientFill (bezelG);
        g.fillRoundedRectangle (outer, rr);

        // top micro highlight (tipis seperti ref)
        g.setColour (Colour::fromRGBA (255, 255, 255, 10));
        g.drawLine (outer.getX() + rr, outer.getY() + 0.6f,
                    outer.getRight() - rr, outer.getY() + 0.6f, 1.0f);

        // thin rims
        g.setColour (Colour::fromRGBA (0, 0, 0, 210));
        g.drawRoundedRectangle (outer.reduced (0.5f), rr - 0.2f, 1.2f);

        g.setColour (Colour::fromRGBA (255, 255, 255, 6));
        g.drawRoundedRectangle (outer.reduced (1.25f), rr - 0.6f, 0.9f);
    }

    // =========================
    // 3) WELL (recess)
    // =========================
    {
        ColourGradient wellG (Colour::fromRGB (8, 8, 7),  well.getCentreX(), well.getY(),
                              Colour::fromRGB (3, 3, 3),  well.getCentreX(), well.getBottom(), false);
        wellG.addColour (0.55, Colour::fromRGB (5, 5, 5));

        g.setGradientFill (wellG);
        g.fillRoundedRectangle (well, rr - 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 235));
        g.drawRoundedRectangle (well.reduced (0.2f).translated (0.0f, 0.55f), rr - 1.2f, 1.15f);

        g.setColour (Colour::fromRGBA (255, 255, 255, 7));
        g.drawRoundedRectangle (well.reduced (0.8f).translated (0.0f, -0.25f), rr - 1.6f, 0.9f);
    }

    // =========================
    // 4) FACE (matte insert)
    // =========================
    {
        auto top = midDark().darker (0.44f);
        auto bot = bgDark().darker (0.72f);

        if (on)
        {
            top = top.brighter (0.08f).interpolatedWith (gld, 0.018f);
            bot = bot.brighter (0.05f).interpolatedWith (gld, 0.010f);
        }

        if (down)
        {
            top = top.darker (0.10f);
            bot = bot.darker (0.10f);
        }

        ColourGradient faceG (top, face.getCentreX(), face.getY(),
                              bot, face.getCentreX(), face.getBottom(), false);
        faceG.addColour (0.55, top.interpolatedWith (bot, 0.55f));

        g.setGradientFill (faceG);
        g.fillRoundedRectangle (face, rr - 2.0f);

        // tiny sheen line (ref feel)
        if (! down)
        {
            g.setColour (Colour::fromRGBA (255, 255, 255, on ? 9 : 6));
            g.drawLine (face.getX() + 8.0f, face.getY() + 1.5f,
                        face.getRight() - 8.0f, face.getY() + 1.5f, 1.0f);
        }

        g.setColour (Colour::fromRGBA (0, 0, 0, 200));
        g.drawRoundedRectangle (face.reduced (0.35f), rr - 2.1f, 1.05f);

        // hover super subtle
        if (highlighted && ! down)
        {
            g.setColour (gld.withAlpha (0.010f));
            g.fillRoundedRectangle (face, rr - 2.0f);
        }
    }

    // =========================
    // 5) TEXT (gold, centered)
    // =========================
    {
        auto tr = face.reduced (6.0f, 1.0f);

        auto f = AuricHelpers::makeFont ((float) jlimit (12, 16, (int) (face.getHeight() - 6.0f)),
                                         Font::plain);
        f = withTracking (f, 1.0f);

        g.setFont (f);

        // engraved shadow
        g.setColour (Colour::fromRGBA (0, 0, 0, 135));
        g.drawText (b.getButtonText(),
                    tr.translated (0.45f, 0.70f).toNearestInt(),
                    Justification::centred, false);

        // main
        g.setColour ((on ? gldHi : gld).withAlpha (on ? 0.92f : 0.82f));
        g.drawText (b.getButtonText(),
                    tr.toNearestInt(),
                    Justification::centred, false);
    }
}

    // Header pill (Save/Load/Del/i) - HARDWARE STYLE
    static void drawHeaderPillButton (juce::Graphics& g,
                                      juce::Button& b,
                                      juce::Rectangle<float> r,
                                      bool highlighted,
                                      bool down)
    {
        using namespace juce;

        r = r.reduced (1.5f);
        if (r.getWidth() < 8.0f || r.getHeight() < 8.0f) return;

        const float rr = clampf (r.getHeight() * 0.25f, 4.0f, 7.0f);
        const bool  on = b.getToggleState();

        const float pressY = (down ? 1.2f : 0.0f) + (on ? 0.5f : 0.0f);

        g.setImageResamplingQuality (Graphics::highResamplingQuality);

        // ========== OUTER SHADOW ==========
        g.setColour (Colour::fromRGBA (0, 0, 0, 90));
        g.fillRoundedRectangle (r.expanded (2.0f).translated (0.0f, 1.5f), rr + 2.0f);
        
        g.setColour (Colour::fromRGBA (0, 0, 0, 140));
        g.fillRoundedRectangle (r.expanded (1.0f).translated (0.0f, 0.8f), rr + 1.0f);

        // ========== METAL BEZEL ==========
        {
            ColourGradient bezelG (Colour::fromRGB (38, 36, 32), r.getCentreX(), r.getY(),
                                   Colour::fromRGB (12, 11, 9), r.getCentreX(), r.getBottom(), false);
            bezelG.addColour (0.25, Colour::fromRGB (45, 43, 38));
            bezelG.addColour (0.50, Colour::fromRGB (28, 26, 23));
            bezelG.addColour (0.80, Colour::fromRGB (14, 13, 11));

            g.setGradientFill (bezelG);
            g.fillRoundedRectangle (r, rr);

            // Top highlight
            g.setColour (Colour::fromRGBA (255, 255, 255, 14));
            g.drawLine (r.getX() + rr, r.getY() + 0.5f,
                        r.getRight() - rr, r.getY() + 0.5f, 0.8f);

            // Outer edge
            g.setColour (Colour::fromRGBA (0, 0, 0, 200));
            g.drawRoundedRectangle (r.reduced (0.3f), rr, 1.2f);
        }

        // ========== RECESSED WELL ==========
        auto well = r.reduced (1.8f);
        {
            ColourGradient wellG (Colour::fromRGB (2, 2, 2), well.getCentreX(), well.getY(),
                                  Colour::fromRGB (6, 6, 5), well.getCentreX(), well.getBottom(), false);
            g.setGradientFill (wellG);
            g.fillRoundedRectangle (well, rr - 1.2f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 230));
            g.drawRoundedRectangle (well.reduced (0.2f), rr - 1.4f, 1.2f);
        }

        // ========== BUTTON CAP ==========
        auto cap = well.reduced (1.5f).translated (0.0f, pressY);
        {
            auto capTop = midDark().brighter (0.04f);
            auto capBot = bgDark().darker (0.45f);

            if (on)
            {
                capTop = capTop.interpolatedWith (goldHi(), 0.06f);
                capBot = capBot.interpolatedWith (gold(), 0.03f);
            }

            if (down)
            {
                capTop = capTop.darker (0.15f);
                capBot = capBot.darker (0.12f);
            }

            ColourGradient capG (capTop, cap.getCentreX(), cap.getY(),
                                 capBot, cap.getCentreX(), cap.getBottom(), false);
            capG.addColour (0.20, capTop.brighter (0.03f));
            capG.addColour (0.55, capTop.interpolatedWith (capBot, 0.5f));

            g.setGradientFill (capG);
            g.fillRoundedRectangle (cap, rr - 2.5f);

            // Subtle brushed texture
            {
                Graphics::ScopedSaveState ss (g);
                Path clip;
                clip.addRoundedRectangle (cap.reduced (0.5f), rr - 3.0f);
                g.reduceClipRegion (clip);

                Random rng (b.getButtonText().hashCode() ^ 0x1D1201);
                const int lines = (int) (cap.getHeight() * 0.4f);
                
                for (int i = 0; i < lines; ++i)
                {
                    const float y = cap.getY() + (i + 0.5f) * (cap.getHeight() / (float) lines);
                    g.setColour (Colours::white.withAlpha (rng.nextFloat() > 0.5f ? 0.012f : 0.006f));
                    g.drawLine (cap.getX() + 2.0f, y, cap.getRight() - 2.0f, y, 0.5f);
                }
            }

            // Top highlight
            if (! down)
            {
                g.setColour (Colour::fromRGBA (255, 255, 255, on ? 10 : 8));
                g.drawLine (cap.getX() + 5.0f, cap.getY() + 1.5f,
                            cap.getRight() - 5.0f, cap.getY() + 1.5f, 0.8f);
            }

            // Cap edge
            g.setColour (Colour::fromRGBA (0, 0, 0, down ? 200 : 160));
            g.drawRoundedRectangle (cap.reduced (0.3f), rr - 2.8f, 1.0f);
        }

        // ========== HOVER ==========
        if (highlighted && ! down)
        {
            g.setColour (gold().withAlpha (0.02f));
            g.fillRoundedRectangle (cap, rr - 2.5f);
        }

        // ========== TEXT ==========
        {
            const float pressTxt = (down ? 1.0f : 0.0f) + (on ? 0.4f : 0.0f);

            auto f = AuricHelpers::makeFont ((float) jlimit (10, 12, b.getHeight() - 8), Font::plain);
            f = withTracking (f, 0.8f);

            // Shadow
            g.setColour (Colour::fromRGBA (0, 0, 0, 130));
            g.setFont (f);
            g.drawText (b.getButtonText(),
                        b.getLocalBounds().translated (0, (int)(pressTxt + 0.6f)),
                        Justification::centred, false);

            // Main text
            g.setColour (on ? goldHi().withAlpha (0.88f) : gold().withAlpha (0.75f));
            g.drawText (b.getButtonText(),
                        b.getLocalBounds().translated (0, (int)pressTxt),
                        Justification::centred, false);
        }
    }
}

//==============================================================================
// Popup bubble styling (keep)
juce::Font AuricLookAndFeel::getSliderPopupFont (juce::Slider&)
{
    return AuricHelpers::makeFont (13.0f * uiScale, juce::Font::plain);
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

    g.setColour (Colour::fromRGBA (0,0,0,160));
    g.fillRoundedRectangle (r.translated (0.0f, 1.2f).expanded (0.8f), 10.0f);

    ColourGradient bg (Colour::fromRGB (26,26,26), r.getCentreX(), r.getY(),
                       Colour::fromRGB (8,8,8),   r.getCentreX(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 10.0f);

    g.setColour (Colour::fromRGBA (0,0,0,190));
    g.drawRoundedRectangle (r.reduced (0.8f), 9.0f, 1.2f);

    g.setColour (gold().withAlpha (0.55f));
    g.drawRoundedRectangle (r, 10.0f, 1.0f);

    Path p;
    const float w = 12.0f, h = 8.0f;
    const float baseY = r.getBottom();

    p.startNewSubPath (tip);
    p.lineTo (tip.x - w * 0.5f, baseY);
    p.lineTo (tip.x + w * 0.5f, baseY);
    p.closeSubPath();

    g.setColour (Colour::fromRGB (14,14,14));
    g.fillPath (p);

    g.setColour (gold().withAlpha (0.45f));
    g.strokePath (p, PathStrokeType (1.0f));
}

//==============================================================================
// AuricLookAndFeel::drawRotarySlider  (FULL)
// - Big knob + Small knob (same family)
// - Ring-only cast shadow (no black disk)
// - Ticks closer to knob
// - Small knob pointer shorter (matches ref)
//==============================================================================

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
    const bool nameSaysBig = (nm.contains ("input")  || nm.contains ("release")
                           || nm.contains ("output") || nm.contains ("attack"));
    const bool sizeSaysBig = (diam >= 150.0f);
    const bool isBigKnob   = nameSaysBig || sizeSaysBig;

    const bool isHover = slider.isMouseOverOrDragging();

    const float angle = rotaryStartAngle
                      + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Size control (jangan bikin knob kegedean / kekecilan)
    const float pad = clampf (diam * (isBigKnob ? 0.115f : 0.105f), 8.0f, 26.0f);
    float radius = (diam * 0.5f - pad);
    radius = jmax (2.0f, radius);

    const auto outer = Rectangle<float> (centre.x - radius, centre.y - radius,
                                         radius * 2.0f, radius * 2.0f);

    // ============================================================
    // TICKS: rapat + pendek + DEKET knob (seperti ref)
    // ============================================================
    const float tickInnerR = radius + (isBigKnob ? radius * 0.050f : radius * 0.055f);
    drawAuricDialTicks (g, centre, tickInnerR, rotaryStartAngle, rotaryEndAngle, isBigKnob);

    // ============================================================
    // BIG KNOB
    // ============================================================
    if (isBigKnob)
    {
        // =========================
        // 0) CAST SHADOW (RING-ONLY)
        // =========================
        {
            const float dropY   = radius * 0.10f;
            const float growOut = radius * 0.18f;   // spread outward
            const float growIn  = radius * 0.02f;   // tight inner edge

            auto outerSh = outer.expanded (growOut).translated (0.0f, dropY);
            auto innerSh = outer.expanded (growIn ).translated (0.0f, dropY);

            Path ring;
            ring.addEllipse (outerSh);
            ring.addEllipse (innerSh);
            ring.setUsingNonZeroWinding (false);

            ColourGradient sg (Colours::black.withAlpha (0.34f),
                               innerSh.getCentreX(), innerSh.getCentreY(),
                               Colours::black.withAlpha (0.00f),
                               outerSh.getCentreX(), outerSh.getCentreY() + radius * 0.20f,
                               true);

            sg.addColour (0.55, Colours::black.withAlpha (0.16f));
            g.setGradientFill (sg);
            g.fillPath (ring);

            // subtle contact shadow (smaller = no halo disk)
            auto contact = outer.expanded (radius * 0.028f).translated (0.0f, radius * 0.11f);
            g.setColour (Colours::black.withAlpha (0.12f));
            g.fillEllipse (contact);
        }

        // =========================
        // 1) PROPORTIONS
        // =========================
        const float skirtThick  = radius * 0.255f;  // beefy body ring
        const float collarThick = radius * 0.060f;  // thin collar
        const float faceInset   = radius * 0.050f;  // recess amount

        auto skirtOuter = outer;
        auto skirtInner = outer.reduced (skirtThick);

        auto collarOuter = skirtInner.expanded (radius * 0.004f);
        auto collarInner = skirtInner.reduced (collarThick);

        auto face = collarInner.reduced (faceInset).translated (0.0f, -radius * 0.02f);

        // =========================
        // 2) OUTER SKIRT
        // =========================
        {
            Path ring;
            ring.addEllipse (skirtOuter);
            ring.addEllipse (skirtInner);
            ring.setUsingNonZeroWinding (false);

            ColourGradient rg (midDark().brighter (0.04f), skirtOuter.getCentreX(), skirtOuter.getY(),
                               bgDark().darker (0.95f),    skirtOuter.getCentreX(), skirtOuter.getBottom(),
                               false);
            rg.addColour (0.40, midDark().darker (0.06f));
            rg.addColour (0.78, bgDark().darker (0.72f));

            g.setGradientFill (rg);
            g.fillPath (ring);

            // crisp lip
            g.setColour (Colour::fromRGBA (255, 255, 255, 10));
            g.drawEllipse (skirtOuter.reduced (radius * 0.010f), 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 215));
            g.drawEllipse (skirtOuter.reduced (radius * 0.020f), 2.2f);

            // inner shadow at skirtInner
            g.setColour (Colour::fromRGBA (0, 0, 0, 150));
            g.drawEllipse (skirtInner.expanded (radius * 0.006f), 1.6f);
        }

        // =========================
        // 3) COLLAR RING
        // =========================
        {
            Path ring;
            ring.addEllipse (collarOuter);
            ring.addEllipse (collarInner);
            ring.setUsingNonZeroWinding (false);

            ColourGradient rg (bgDark().brighter (0.020f), collarOuter.getCentreX(), collarOuter.getY(),
                               bgDark().darker (0.82f),   collarOuter.getCentreX(), collarOuter.getBottom(),
                               false);
            rg.addColour (0.55, midDark().darker (0.10f));
            rg.addColour (0.86, bgDark().darker (0.90f));

            g.setGradientFill (rg);
            g.fillPath (ring);

            g.setColour (Colour::fromRGBA (255, 255, 255, 7));
            g.drawEllipse (collarOuter.reduced (radius * 0.010f), 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 160));
            g.drawEllipse (collarInner.expanded (radius * 0.004f), 1.0f);
        }

        // =========================
        // 4) FACE (recessed satin)
        // =========================
        {
            // inner cavity shadow (tight)
            g.setColour (Colour::fromRGBA (0, 0, 0, 170));
            g.fillEllipse (face.translated (0.0f, radius * 0.055f).expanded (radius * 0.020f));

            ColourGradient fg (midDark().brighter (0.11f),
                               centre.x - radius * 0.40f, centre.y - radius * 0.48f,
                               bgDark().darker (0.90f),
                               centre.x + radius * 0.18f, centre.y + radius * 0.22f,
                               true);
            fg.addColour (0.58, midDark().darker (0.14f));
            fg.addColour (0.86, bgDark().darker (0.80f));

            g.setGradientFill (fg);
            g.fillEllipse (face);

            // diagonal sheen band (very subtle)
            {
                Graphics::ScopedSaveState ss (g);
                Path clip; clip.addEllipse (face);
                g.reduceClipRegion (clip);

                g.addTransform (AffineTransform::rotation (-0.55f, centre.x, centre.y));
                auto band = face.expanded (radius * 0.32f).translated (-radius * 0.10f, -radius * 0.26f);

                ColourGradient sheen (Colour::fromRGBA (255,255,255,0),
                                      band.getCentreX(), band.getY(),
                                      Colour::fromRGBA (255,255,255,0),
                                      band.getCentreX(), band.getBottom(), false);
                sheen.addColour (0.40, Colour::fromRGBA (255,255,255,9));
                sheen.addColour (0.60, Colour::fromRGBA (255,255,255,0));

                g.setGradientFill (sheen);
                g.fillRect (band);
            }

            // face edge lines
            g.setColour (Colour::fromRGBA (255, 255, 255, 9));
            g.drawEllipse (face.reduced (radius * 0.020f), 1.4f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 180));
            g.drawEllipse (face.reduced (radius * 0.080f), 1.6f);

            // groove ring
            {
                auto groove = face.reduced (radius * 0.135f);
                g.setColour (Colour::fromRGBA (0, 0, 0, 70));
                g.drawEllipse (groove, 1.1f);
                g.setColour (Colour::fromRGBA (255, 255, 255, 6));
                g.drawEllipse (groove.translated (-0.35f, -0.35f), 1.0f);
            }

            // micro texture
            drawCeramicTexture (g,
                                face.reduced (radius * 0.15f),
                                centre,
                                angle,
                                isHover ? 0.050f : 0.038f,
                                slider.getName().hashCode());
        }

        // =========================
        // 5) POINTER (shorter, cleaner)
        // =========================
        {
            const float faceR = face.getWidth() * 0.5f;
            const float inR   = faceR * 0.33f;
            const float outR  = faceR * 0.88f; // doesn’t touch rim
            const float th    = clampf (radius * 0.014f, 1.0f, 2.0f);

            auto p1 = polar (centre, angle, inR);
            auto p2 = polar (centre, angle, outR);

            g.setColour (Colour::fromRGBA (0, 0, 0, 120));
            g.drawLine (p1.x + 0.50f, p1.y + 0.62f,
                        p2.x + 0.50f, p2.y + 0.62f,
                        th + 0.60f);

            g.setColour (needleCol().withAlpha (isHover ? 0.98f : 0.86f));
            g.drawLine (p1.x, p1.y, p2.x, p2.y, th);
        }

        return;
    }

    // ============================================================
    // SMALL KNOB (same family)
    // ============================================================
    {
        // =========================
        // CAST SHADOW (RING-ONLY)
        // =========================
        {
            const float dropY   = radius * 0.09f;
            const float growOut = radius * 0.17f;
            const float growIn  = radius * 0.02f;

            auto outerSh = outer.expanded (growOut).translated (0.0f, dropY);
            auto innerSh = outer.expanded (growIn ).translated (0.0f, dropY);

            Path ring;
            ring.addEllipse (outerSh);
            ring.addEllipse (innerSh);
            ring.setUsingNonZeroWinding (false);

            ColourGradient sg (Colours::black.withAlpha (0.30f),
                               innerSh.getCentreX(), innerSh.getCentreY(),
                               Colours::black.withAlpha (0.00f),
                               outerSh.getCentreX(), outerSh.getCentreY() + radius * 0.18f,
                               true);

            sg.addColour (0.55, Colours::black.withAlpha (0.14f));
            g.setGradientFill (sg);
            g.fillPath (ring);

            auto contact = outer.expanded (radius * 0.025f).translated (0.0f, radius * 0.105f);
            g.setColour (Colours::black.withAlpha (0.10f));
            g.fillEllipse (contact);
        }

        const float ringThick = radius * 0.17f;
        auto ringOuter = outer;
        auto ringInner = outer.reduced (ringThick);
        auto face      = ringInner.reduced (radius * 0.06f);

        // ring
        {
            Path ring;
            ring.addEllipse (ringOuter);
            ring.addEllipse (ringInner);
            ring.setUsingNonZeroWinding (false);

            ColourGradient rg (midDark().brighter (0.05f), ringOuter.getCentreX(), ringOuter.getY(),
                               bgDark().darker (0.90f),    ringOuter.getCentreX(), ringOuter.getBottom(), false);
            rg.addColour (0.48, midDark().darker (0.10f));
            rg.addColour (0.82, bgDark().darker (0.72f));

            g.setGradientFill (rg);
            g.fillPath (ring);

            g.setColour (Colour::fromRGBA (255, 255, 255, 9));
            g.drawEllipse (ringOuter.reduced (radius * 0.012f), 1.0f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 195));
            g.drawEllipse (ringOuter.reduced (radius * 0.020f), 1.6f);
        }

        // face
        {
            g.setColour (Colour::fromRGBA (0, 0, 0, 145));
            g.fillEllipse (face.translated (0.0f, radius * 0.045f).expanded (radius * 0.018f));

            ColourGradient fg (midDark().brighter (0.10f),
                               centre.x - radius * 0.42f, centre.y - radius * 0.50f,
                               bgDark().darker (0.86f),
                               centre.x + radius * 0.18f, centre.y + radius * 0.18f,
                               true);
            fg.addColour (0.62, midDark().darker (0.15f));
            fg.addColour (0.86, bgDark().darker (0.76f));

            g.setGradientFill (fg);
            g.fillEllipse (face);

            g.setColour (Colour::fromRGBA (255, 255, 255, 9));
            g.drawEllipse (face.reduced (radius * 0.020f), 1.1f);

            g.setColour (Colour::fromRGBA (0, 0, 0, 165));
            g.drawEllipse (face.reduced (radius * 0.075f), 1.4f);

            drawCeramicTexture (g,
                                face.reduced (radius * 0.15f),
                                centre,
                                angle,
                                isHover ? 0.048f : 0.030f,
                                slider.getName().hashCode());
        }

        // pointer (SHORTER than before)
        {
            const float faceR = face.getWidth() * 0.5f;
            const float inR   = faceR * 0.30f;
            const float outR  = faceR * 0.86f; // <- shorter (matches ref)
            const float th    = clampf (radius * 0.013f, 0.9f, 1.8f);

            auto p1 = polar (centre, angle, inR);
            auto p2 = polar (centre, angle, outR);

            g.setColour (Colour::fromRGBA (0, 0, 0, 110));
            g.drawLine (p1.x + 0.45f, p1.y + 0.58f,
                        p2.x + 0.45f, p2.y + 0.58f,
                        th + 0.55f);

            g.setColour (needleCol().withAlpha (isHover ? 0.98f : 0.86f));
            g.drawLine (p1.x, p1.y, p2.x, p2.y, th);
        }
    }
}


//==============================================================================
// Buttons / ComboBox / Labels (tetap minimal & hardware)
void AuricLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                        bool highlighted, bool down)
{
    using namespace juce;

    if (isHardwareButton (button))
    {
        drawHardwareBarButton (g, button, button.getLocalBounds().toFloat(), highlighted, down);
        return;
    }

    // fallback minimal
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    ColourGradient bg (midDark(), r.getCentreX(), r.getY(),
                       bgDark(),  r.getCentreX(), r.getBottom(), false);

    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 6.0f);

    g.setColour (Colour::fromRGBA (0, 0, 0, 200));
    g.drawRoundedRectangle (r.reduced (0.6f), 6.0f, 1.0f);

    g.setColour (gold().withAlpha (0.90f));
    g.setFont (AuricHelpers::makeFont (13.0f * uiScale, Font::plain));
    g.drawText (button.getButtonText(), button.getLocalBounds(), Justification::centred, false);
}

juce::Font AuricLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    const float base = (float) juce::jlimit (11, 16, buttonHeight - 12);
    return AuricHelpers::makeFont (base * uiScale, juce::Font::plain);
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
        drawHeaderPillButton (g, button, button.getLocalBounds().toFloat(), highlighted, down);
        return;
    }

    // default small matte button
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);

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
        g.setColour (gold().withAlpha (0.05f));
        g.fillRoundedRectangle (r, 6.0f);
    }
}

void AuricLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                       bool, bool)
{
    using namespace juce;

    auto f = getTextButtonFont (button, button.getHeight());
    f = withTracking (f, 0.7f);

    g.setColour (gold().withAlpha (0.90f));
    g.setFont (f);
    g.drawText (button.getButtonText(), button.getLocalBounds(),
                Justification::centred, false);
}

void AuricLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int, int, int, int, juce::ComboBox& box)
{
    using namespace juce;

    auto r = Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
    const auto gld = gold();
    const auto gldHi = goldHi();

    const bool hdr = isHeaderPresetBox (box);
    const float rr = hdr ? 5.0f : 4.0f;

    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    if (hdr)
    {
        // ========== HARDWARE STYLE DROPDOWN ==========
        
        // Outer shadow
        g.setColour (Colour::fromRGBA (0, 0, 0, 85));
        g.fillRoundedRectangle (r.expanded (2.0f).translated (0.0f, 1.5f), rr + 2.0f);
        
        g.setColour (Colour::fromRGBA (0, 0, 0, 130));
        g.fillRoundedRectangle (r.expanded (1.0f).translated (0.0f, 0.8f), rr + 1.0f);

        // Metal bezel
        {
            ColourGradient bezelG (Colour::fromRGB (36, 34, 30), r.getCentreX(), r.getY(),
                                   Colour::fromRGB (10, 9, 8), r.getCentreX(), r.getBottom(), false);
            bezelG.addColour (0.20, Colour::fromRGB (42, 40, 35));
            bezelG.addColour (0.45, Colour::fromRGB (26, 24, 21));
            bezelG.addColour (0.75, Colour::fromRGB (14, 13, 11));

            g.setGradientFill (bezelG);
            g.fillRoundedRectangle (r, rr);

            // Top highlight
            g.setColour (Colour::fromRGBA (255, 255, 255, 12));
            g.drawLine (r.getX() + rr, r.getY() + 0.5f,
                        r.getRight() - rr, r.getY() + 0.5f, 0.8f);

            // Outer edge
            g.setColour (Colour::fromRGBA (0, 0, 0, 190));
            g.drawRoundedRectangle (r.reduced (0.3f), rr, 1.1f);
        }

        // Recessed field
        auto field = r.reduced (2.0f);
        {
            ColourGradient fieldG (Colour::fromRGB (2, 2, 2), field.getCentreX(), field.getY(),
                                   Colour::fromRGB (5, 5, 4), field.getCentreX(), field.getBottom(), false);
            g.setGradientFill (fieldG);
            g.fillRoundedRectangle (field, rr - 1.2f);

            // Inner shadow
            g.setColour (Colour::fromRGBA (0, 0, 0, 220));
            g.drawRoundedRectangle (field.reduced (0.2f), rr - 1.4f, 1.2f);

            // Subtle bottom highlight
            g.setColour (Colour::fromRGBA (255, 255, 255, 4));
            g.drawLine (field.getX() + rr, field.getBottom() - 1.0f,
                        field.getRight() - rr, field.getBottom() - 1.0f, 0.6f);
        }

        // Arrow button area (right side)
        auto arrowArea = Rectangle<float> (field.getRight() - 24.0f, field.getY(),
                                           24.0f, field.getHeight());
        
        // Separator line
        g.setColour (Colour::fromRGBA (0, 0, 0, 180));
        g.drawLine (arrowArea.getX(), field.getY() + 3.0f,
                    arrowArea.getX(), field.getBottom() - 3.0f, 1.0f);
        
        g.setColour (Colour::fromRGBA (255, 255, 255, 6));
        g.drawLine (arrowArea.getX() + 1.0f, field.getY() + 3.0f,
                    arrowArea.getX() + 1.0f, field.getBottom() - 3.0f, 0.6f);

        // Arrow
        auto a = arrowArea.reduced (7.0f, 8.0f);
        Path p;
        p.startNewSubPath (a.getX(), a.getCentreY() - 2.0f);
        p.lineTo (a.getCentreX(), a.getCentreY() + 3.0f);
        p.lineTo (a.getRight(), a.getCentreY() - 2.0f);

        // Arrow shadow
        g.setColour (Colour::fromRGBA (0, 0, 0, 120));
        g.strokePath (p, PathStrokeType (2.2f), AffineTransform::translation (0.5f, 0.5f));

        // Arrow main
        g.setColour (isButtonDown ? gldHi.withAlpha (0.80f) : gld.withAlpha (0.60f));
        g.strokePath (p, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    else
    {
        // Non-header combo (simpler style)
        g.setColour (Colour::fromRGBA (0, 0, 0, 100));
        g.fillRoundedRectangle (r.expanded (1.0f).translated (0.0f, 0.8f), rr + 1.0f);

        ColourGradient grad (Colour::fromRGB (28, 26, 23), r.getCentreX(), r.getY(),
                             Colour::fromRGB (10, 9, 8),  r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, rr);

        g.setColour (Colour::fromRGBA (255, 255, 255, 8));
        g.drawLine (r.getX() + rr, r.getY() + 0.5f,
                    r.getRight() - rr, r.getY() + 0.5f, 0.6f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 170));
        g.drawRoundedRectangle (r.reduced (0.3f), rr, 1.0f);

        auto arrowR = r.removeFromRight (16.0f).reduced (4.0f, 6.0f);
        Path p;
        p.startNewSubPath (arrowR.getX(), arrowR.getCentreY() - 1.5f);
        p.lineTo (arrowR.getCentreX(), arrowR.getCentreY() + 2.5f);
        p.lineTo (arrowR.getRight(), arrowR.getCentreY() - 1.5f);

        g.setColour (gld.withAlpha (0.55f));
        g.strokePath (p, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    box.setColour (ComboBox::textColourId,  gld.withAlpha (0.85f));
    box.setColour (ComboBox::arrowColourId, gld.withAlpha (0.65f));
}

void AuricLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    if (isHeaderPresetBox (box))
        label.setBounds (10, 1, box.getWidth() - 36, box.getHeight() - 2);
    else
        label.setBounds (6, 1, box.getWidth() - 22, box.getHeight() - 2);

    label.setFont (withTracking (AuricHelpers::makeFont (12.0f * uiScale), 0.7f));
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

void AuricLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    using namespace juce;

    auto r = Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);

    ColourGradient bg (bgDark().brighter (0.02f), r.getCentreX(), r.getY(),
                       Colours::black,           r.getCentreX(), r.getBottom(), false);
    bg.addColour (0.55, midDark());
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 6.0f);

    g.setColour (Colour::fromRGBA (255, 255, 255, 12));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    g.setColour (Colour::fromRGBA (0, 0, 0, 190));
    g.drawRoundedRectangle (r.reduced (1.0f), 5.0f, 1.2f);
}

void AuricLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool hasSubMenu, const juce::String& text,
                                         const juce::String& shortcutKeyText,
                                         const juce::Drawable* icon,
                                         const juce::Colour* textColourToUse)
{
    using namespace juce;

    if (isSeparator)
    {
        g.setColour (Colour::fromRGBA (255, 255, 255, 10));
        g.drawLine ((float) area.getX() + 6.0f, (float) area.getCentreY(),
                    (float) area.getRight() - 6.0f, (float) area.getCentreY(), 1.0f);
        return;
    }

    auto r = area.toFloat().reduced (2.0f, 1.0f);

    if (isHighlighted)
    {
        g.setColour (goldHi().withAlpha (0.10f));
        g.fillRoundedRectangle (r, 4.0f);
    }

    if (icon != nullptr)
        icon->drawWithin (g, r.removeFromLeft (20.0f), RectanglePlacement::centred, 0.85f);

    if (isTicked)
    {
        g.setColour (goldHi().withAlpha (0.85f));
        g.drawRect (r.removeFromLeft (18.0f).reduced (3.0f), 1.0f);
    }

    g.setColour (textColourToUse != nullptr ? *textColourToUse
                                            : gold().withAlpha (isActive ? 0.92f : 0.45f));
    g.setFont (withTracking (AuricHelpers::makeFont (13.0f * uiScale), 0.7f));
    g.drawText (text, r, Justification::centredLeft, false);

    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (gold().withAlpha (0.55f));
        g.setFont (withTracking (AuricHelpers::makeFont (11.0f * uiScale), 0.6f));
        g.drawText (shortcutKeyText, r, Justification::centredRight, false);
    }

    if (hasSubMenu)
    {
        auto a = r.removeFromRight (12.0f).reduced (2.0f);
        Path p;
        p.startNewSubPath (a.getX(), a.getY());
        p.lineTo (a.getRight(), a.getCentreY());
        p.lineTo (a.getX(), a.getBottom());

        g.setColour (gold().withAlpha (0.70f));
        g.strokePath (p, PathStrokeType (1.4f));
    }
}
