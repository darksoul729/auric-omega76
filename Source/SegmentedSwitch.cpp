#include "SegmentedSwitch.h"
#include "AuricHelpers.h"
#include <cmath>

//==============================================================================
// Theme (match Auric panel: clean, matte, subtle gold)
namespace SegTheme
{
    static inline juce::Colour goldText() { return juce::Colour::fromRGB (226, 195, 129); }
    static inline juce::Colour goldHi()   { return juce::Colour::fromRGB (251, 247, 175); }

    static inline juce::Colour bg0()      { return juce::Colour::fromRGB (10, 10, 9);    }
    static inline juce::Colour bg1()      { return juce::Colour::fromRGB (18, 18, 16);  }

    static inline juce::Colour rimHi()    { return juce::Colour::fromRGB (52, 52, 46);  }
    static inline juce::Colour rimLo()    { return juce::Colour::fromRGB (6, 6, 5);     }

    static inline juce::Colour inkDark()  { return juce::Colour::fromRGBA (0, 0, 0, 205); }
}

static inline float clampf (float v, float lo, float hi) { return juce::jlimit (lo, hi, v); }

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

    // subtle engraved shadow only (keep clean)
    g.setColour (Colour::fromRGBA (0, 0, 0, 120));
    g.drawText (text, bounds.translated (1, 1), just, false);

    g.setColour (main);
    g.drawText (text, bounds, just, false);
}

// outer corners only
static juce::Path makeSegmentPath (juce::Rectangle<float> r, float cr, bool roundLeft, bool roundRight)
{
    using namespace juce;

    Path p;
    cr = jmin (cr, r.getWidth() * 0.5f, r.getHeight() * 0.5f);

    const float x  = r.getX();
    const float y  = r.getY();
    const float xr = r.getRight();
    const float yb = r.getBottom();

    p.startNewSubPath (x + (roundLeft ? cr : 0.0f), y);

    p.lineTo (xr - (roundRight ? cr : 0.0f), y);
    if (roundRight) p.quadraticTo (xr, y, xr, y + cr);
    else            p.lineTo (xr, y);

    p.lineTo (xr, yb - (roundRight ? cr : 0.0f));
    if (roundRight) p.quadraticTo (xr, yb, xr - cr, yb);
    else            p.lineTo (xr, yb);

    p.lineTo (x + (roundLeft ? cr : 0.0f), yb);
    if (roundLeft)  p.quadraticTo (x, yb, x, yb - cr);
    else            p.lineTo (x, yb);

    p.lineTo (x, y + (roundLeft ? cr : 0.0f));
    if (roundLeft)  p.quadraticTo (x, y, x + cr, y);
    else            p.lineTo (x, y);

    p.closeSubPath();
    return p;
}

static inline void clipRounded (juce::Graphics& g, juce::Rectangle<float> r, float rr, std::function<void()> fn)
{
    using namespace juce;
    Graphics::ScopedSaveState ss (g);
    Path p; p.addRoundedRectangle (r, rr);
    g.reduceClipRegion (p);
    fn();
}

//==============================================================================
// SegmentedSwitch3
SegmentedSwitch3::SegmentedSwitch3 (juce::String a, juce::String b, juce::String c)
{
    labels[0] = std::move (a);
    labels[1] = std::move (b);
    labels[2] = std::move (c);

    subLabels[0] = {};
    subLabels[1] = {};
    subLabels[2] = {};

    setInterceptsMouseClicks (true, true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setOpaque (false);
}

void SegmentedSwitch3::setSelectedIndex (int idx)
{
    selected = juce::jlimit (0, 2, idx);
    repaint();
}

void SegmentedSwitch3::setLabels (juce::String a, juce::String b, juce::String c)
{
    labels[0] = std::move (a);
    labels[1] = std::move (b);
    labels[2] = std::move (c);
    repaint();
}

void SegmentedSwitch3::setSubLabels (juce::String a, juce::String b, juce::String c)
{
    subLabels[0] = std::move (a);
    subLabels[1] = std::move (b);
    subLabels[2] = std::move (c);
    repaint();
}

void SegmentedSwitch3::setLeftLegend (juce::String symbol, juce::String text, bool enable)
{
    leftSymbol = std::move (symbol);
    leftText   = std::move (text);
    drawLeftLegend = enable;
    repaint();
}

void SegmentedSwitch3::setDrawLeftLegend (bool enable)
{
    drawLeftLegend = enable;
    repaint();
}

void SegmentedSwitch3::setLegendWidth (int px)
{
    legendWidthOverride = juce::jmax (0, px);
    repaint();
}

int SegmentedSwitch3::hitTestIndex (juce::Point<int> p) const
{
    auto r = getLocalBounds();

    const bool hasLegend = drawLeftLegend && (leftText.isNotEmpty() || leftSymbol.isNotEmpty());

    int legendW = 0;
    if (hasLegend)
        legendW = (legendWidthOverride > 0) ? legendWidthOverride
                                            : juce::jmin (120, r.getWidth() / 3);

    auto segArea = r.withTrimmedLeft (legendW);
    if (! segArea.contains (p)) return -1;

    const int w = segArea.getWidth() / 3;
    const int x = p.x - segArea.getX();

    if (x < w)     return 0;
    if (x < 2 * w) return 1;
    return 2;
}

void SegmentedSwitch3::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitTestIndex (e.getPosition());
    if (h != hovered) { hovered = h; repaint(); }

    const bool inside = getLocalBounds().contains (e.getPosition());
    const int info = (h >= 0 ? h : (inside ? -2 : -1));
    if (info != hoverInfo)
    {
        hoverInfo = info;
        if (onHover) onHover (hoverInfo);
    }
}

void SegmentedSwitch3::mouseEnter (const juce::MouseEvent& e) { mouseMove (e); }

void SegmentedSwitch3::mouseExit (const juce::MouseEvent&)
{
    if (hovered != -1) { hovered = -1; repaint(); }
    if (hoverInfo != -1)
    {
        hoverInfo = -1;
        if (onHover) onHover (-1);
    }
}

void SegmentedSwitch3::mouseDown (const juce::MouseEvent& e)
{
    pressed = hitTestIndex (e.getPosition());
    mouseMove (e);
    repaint();
}

void SegmentedSwitch3::mouseDrag (const juce::MouseEvent& e)
{
    const int p = hitTestIndex (e.getPosition());
    if (p != pressed) { pressed = p; repaint(); }
    mouseMove (e);
}

void SegmentedSwitch3::mouseUp (const juce::MouseEvent& e)
{
    const int idx = hitTestIndex (e.getPosition());
    pressed = -1;

    if (idx >= 0 && idx != selected)
    {
        selected = idx;
        repaint();
        if (onChange) onChange (selected);
        return;
    }

    repaint();
}

//==============================================================================
// paint (CLEAN HARDWARE)
void SegmentedSwitch3::paint (juce::Graphics& g)
{
    using namespace juce;

    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    auto bounds = getLocalBounds().toFloat().reduced (0.6f);
    if (bounds.getHeight() < 10.0f) return;

    const bool hasLegend = drawLeftLegend && (leftText.isNotEmpty() || leftSymbol.isNotEmpty());
    const bool hasSub = subLabels[0].isNotEmpty() || subLabels[1].isNotEmpty() || subLabels[2].isNotEmpty();

    int legendW = 0;
    if (hasLegend)
        legendW = (legendWidthOverride > 0) ? legendWidthOverride
                                            : jmin (120, (int) (bounds.getWidth() * 0.30f));

    // rounded like your other controls
    const float rrOuter = clampf (bounds.getHeight() * 0.26f, 8.0f, 14.0f);
    const float rrInner = rrOuter - 2.2f;

    // =========================
    // OUTER BEZEL (clean)
    // =========================
    {
        auto sh = bounds.expanded (2.0f).translated (0.0f, 1.9f);
        ColourGradient sg (Colours::black.withAlpha (0.00f), sh.getCentreX(), sh.getY(),
                           Colours::black.withAlpha (0.52f), sh.getCentreX(), sh.getBottom(), false);
        sg.addColour (0.60, Colours::black.withAlpha (0.18f));
        g.setGradientFill (sg);
        g.fillRoundedRectangle (sh, rrOuter + 1.6f);

        ColourGradient bezel (SegTheme::rimHi().withAlpha (0.98f), bounds.getCentreX(), bounds.getY(),
                              SegTheme::rimLo().withAlpha (0.99f), bounds.getCentreX(), bounds.getBottom(), false);
        bezel.addColour (0.52, SegTheme::bg1().withAlpha (0.98f));
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (bounds, rrOuter);

        // thin machined lines
        g.setColour (Colour::fromRGBA (255, 255, 255, 10));
        g.drawRoundedRectangle (bounds.reduced (0.8f), rrOuter - 0.8f, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 220));
        g.drawRoundedRectangle (bounds.reduced (1.8f), rrOuter - 1.6f, 1.35f);
    }

    // =========================
    // INNER WELL (recess)
    // =========================
    auto inner = bounds.reduced (2.8f);
    {
        ColourGradient well (SegTheme::bg1(), inner.getCentreX(), inner.getY(),
                             SegTheme::bg0(), inner.getCentreX(), inner.getBottom(), false);
        well.addColour (0.55, Colour::fromRGB (10, 10, 10));
        g.setGradientFill (well);
        g.fillRoundedRectangle (inner, rrInner);

        // recess bevel (subtle)
        g.setColour (Colour::fromRGBA (255, 255, 255, 8));
        g.drawRoundedRectangle (inner.reduced (0.9f).translated (0.0f, -0.25f), rrInner - 0.9f, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 235));
        g.drawRoundedRectangle (inner.reduced (1.35f).translated (0.0f, 0.55f), rrInner - 1.35f, 1.25f);

        // slight vignette
        clipRounded (g, inner, rrInner, [&]
        {
            ColourGradient v (Colour::fromRGBA (0,0,0,85), inner.getCentreX(), inner.getBottom(),
                              Colour::fromRGBA (0,0,0,0),  inner.getCentreX(), inner.getY(), false);
            g.setGradientFill (v);
            g.fillRect (inner);
        });
    }

    // clip to inner
    Path clip; clip.addRoundedRectangle (inner, rrInner);
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (clip);

    const float HIn = inner.getHeight();

    const float subH = (hasSub && HIn >= 30.0f) ? jlimit (10.0f, 16.0f, HIn * 0.32f) : 0.0f;
    const float btnH = jmax (10.0f, HIn - subH);

    auto btnArea = inner.withHeight (btnH);
    auto subArea = inner.withTrimmedTop (btnH);

    if (subH > 0.5f)
        subArea = subArea.reduced (2.0f, 2.0f);

    auto legendArea = btnArea.removeFromLeft ((float) legendW);
    auto segArea    = btnArea;

    // =========================
    // LEFT LEGEND (clean)
    // =========================
    if (hasLegend)
    {
        auto lr = legendArea.reduced (6.0f, 2.0f);

        const auto gold = SegTheme::goldText();
        const float symSize  = jlimit (14.0f, 22.0f, lr.getHeight() * 0.62f);
        const float fontSize = jlimit (12.0f, 18.0f, lr.getHeight() * 0.54f);

        auto symR = lr.removeFromLeft (28.0f);
        embossText (g, leftSymbol, symR.toNearestInt(),
                    AuricHelpers::makeFont (symSize, Font::plain),
                    gold.withAlpha (0.95f), Justification::centred);

        lr.removeFromLeft (2.0f);
        embossText (g, leftText, lr.toNearestInt(),
                    withTracking (AuricHelpers::makeFont (fontSize, Font::plain), 0.9f),
                    gold.withAlpha (0.90f), Justification::centredLeft);

        // machined divider seam
        const float x = segArea.getX();
        g.setColour (Colour::fromRGBA (0, 0, 0, 190));
        g.drawLine (x, segArea.getY() + 4.0f, x, segArea.getBottom() - 4.0f, 1.25f);
        g.setColour (Colour::fromRGBA (255, 255, 255, 9));
        g.drawLine (x + 1.0f, segArea.getY() + 4.0f, x + 1.0f, segArea.getBottom() - 4.0f, 1.0f);
    }

    // =========================
    // SEGMENTS (match Auric aesthetic)
    // =========================
    auto segInner = segArea.reduced (4.0f, 3.0f);
    const float segW = segInner.getWidth() / 3.0f;

    const auto gold = SegTheme::goldText();
    const float fontSize = jlimit (12.0f, 18.0f, segInner.getHeight() * 0.62f);
    const float cr = jlimit (4.0f, 10.0f, segInner.getHeight() * 0.26f);

    auto drawSelectedGlow = [&] (Rectangle<float> r, float rr, float a)
    {
        if (a <= 0.001f) return;
        clipRounded (g, r, rr, [&]
        {
            ColourGradient cg (SegTheme::goldHi().withAlpha (0.0f), r.getCentreX(), r.getCentreY(),
                               SegTheme::goldHi().withAlpha (a),   r.getCentreX(), r.getY(), false);
            cg.addColour (0.60, SegTheme::goldText().withAlpha (a * 0.45f));
            cg.addColour (1.00, Colour::fromRGBA (0,0,0,0));
            g.setGradientFill (cg);
            g.fillRect (r);
        });
    };

    for (int i = 0; i < 3; ++i)
    {
        auto cell = segInner.withX (segInner.getX() + segW * (float) i).withWidth (segW);

        const bool isSel   = (i == selected);
        const bool isHover = (i == hovered);
        const bool isDown  = (i == pressed);

        auto face = cell.reduced (2.0f, 2.0f);
        if (isDown) face = face.translated (0.0f, 1.0f);

        const bool roundL = (i == 0);
        const bool roundR = (i == 2);

        auto path = makeSegmentPath (face, cr, roundL, roundR);

        if (isSel)
        {
            // raised plate (subtle)
            g.setColour (Colour::fromRGBA (0, 0, 0, 130));
            { auto sh = path; sh.applyTransform (AffineTransform::translation (0.0f, 1.6f)); g.fillPath (sh); }

            ColourGradient plate (Colour::fromRGB (36, 36, 34), face.getCentreX(), face.getY(),
                                  Colour::fromRGB (9, 9, 9),     face.getCentreX(), face.getBottom(), false);
            plate.addColour (0.55, Colour::fromRGB (18, 18, 18));
            g.setGradientFill (plate);
            g.fillPath (path);

            // spec band (tiny)
            clipRounded (g, face, cr, [&]
            {
                auto top = face.withHeight (face.getHeight() * 0.42f);
                ColourGradient gl (Colour::fromRGBA (255,255,255,18), top.getCentreX(), top.getY(),
                                   Colour::fromRGBA (255,255,255,0),  top.getCentreX(), top.getBottom(), false);
                gl.addColour (0.45, Colour::fromRGBA (255,255,255,9));
                g.setGradientFill (gl);
                g.fillRect (top);
            });

            drawSelectedGlow (face.reduced (1.5f), cr - 1.0f, isDown ? 0.09f : 0.13f);

            // gold rim (thin)
            g.setColour (SegTheme::goldHi().withAlpha (0.18f));
            g.strokePath (path, PathStrokeType (1.0f));

            g.setColour (Colour::fromRGBA (0,0,0,170));
            g.strokePath (path, PathStrokeType (1.1f), AffineTransform::translation (0.0f, 0.55f));
        }
        else
        {
            // recessed insert
            ColourGradient insert (Colour::fromRGB (15, 15, 14), face.getCentreX(), face.getY(),
                                   Colour::fromRGB (6, 6, 6),     face.getCentreX(), face.getBottom(), false);
            insert.addColour (0.55, Colour::fromRGB (10, 10, 10));
            g.setGradientFill (insert);
            g.fillPath (path);

            // bevel lines
            g.setColour (Colour::fromRGBA (0,0,0,220));
            g.strokePath (path, PathStrokeType (1.25f), AffineTransform::translation (0.0f, 0.75f));

            g.setColour (Colour::fromRGBA (255,255,255,8));
            g.strokePath (path, PathStrokeType (1.0f), AffineTransform::translation (0.0f, -0.25f));

            if (isHover && ! isDown)
            {
                g.setColour (SegTheme::goldText().withAlpha (0.022f));
                g.fillPath (path);

                g.setColour (SegTheme::goldHi().withAlpha (0.08f));
                g.strokePath (path, PathStrokeType (1.0f));
            }
        }

        // machined seam dividers
        if (i > 0)
        {
            const float x = cell.getX();
            g.setColour (Colour::fromRGBA (0,0,0,190));
            g.drawLine (x, cell.getY() + 4.0f, x, cell.getBottom() - 4.0f, 1.25f);

            g.setColour (Colour::fromRGBA (255,255,255,9));
            g.drawLine (x + 1.0f, cell.getY() + 4.0f, x + 1.0f, cell.getBottom() - 4.0f, 1.0f);
        }

        // text colour (clean)
        auto textCol = (isSel ? SegTheme::goldHi() : gold).withAlpha (isSel ? 0.96f : 0.72f);
        if (isHover && ! isSel) textCol = textCol.withAlpha (0.84f);
        if (isDown && ! isSel)  textCol = textCol.withAlpha (0.66f);

        auto tb = cell.toNearestInt();
        tb.reduce (0, 1);
        if (isSel || (isDown && ! isSel)) tb = tb.translated (0, 1);

        embossText (g, labels[i], tb,
                    withTracking (AuricHelpers::makeFont (fontSize, Font::plain), 0.95f),
                    textCol, Justification::centred);
    }

    // =========================
    // SUB LABELS
    // =========================
    if (subH > 0.5f)
    {
        const float subFont = jlimit (9.3f, 12.2f, subArea.getHeight() * 0.78f);
        auto f = withTracking (AuricHelpers::makeFont (subFont, Font::plain), 0.85f);

        const float segW2 = segArea.getWidth() / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            auto cell = Rectangle<int> ((int) std::round (segArea.getX() + segW2 * i),
                                        (int) std::round (subArea.getY()),
                                        (int) std::round (segW2),
                                        (int) std::round (subArea.getHeight()));

            cell.reduce (0, 1);

            g.setFont (f);
            g.setColour (Colour::fromRGBA (0, 0, 0, 120));
            g.drawText (subLabels[i], cell.translated (1, 1), Justification::centred, false);

            g.setColour (SegTheme::goldText().withAlpha (0.72f));
            g.drawText (subLabels[i], cell, Justification::centred, false);
        }
    }
}

//==============================================================================
// Attachment
SegmentedSwitchAttachment::SegmentedSwitchAttachment (juce::AudioProcessorValueTreeState& state,
                                                      const juce::String& paramID,
                                                      SegmentedSwitch3& control)
    : apvts (state), id (paramID), ctrl (control)
{
    param  = apvts.getParameter (id);
    choice = dynamic_cast<juce::AudioParameterChoice*> (param);

    jassert (param  != nullptr);
    jassert (choice != nullptr);

    apvts.addParameterListener (id, this);

    ctrl.onChange = [this] (int idx) { pushParamFromControl (idx); };
    pushControlFromParam();
}

SegmentedSwitchAttachment::~SegmentedSwitchAttachment()
{
    apvts.removeParameterListener (id, this);
    ctrl.onChange = nullptr;
}

void SegmentedSwitchAttachment::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID != id) return;
    juce::MessageManager::callAsync ([this] { pushControlFromParam(); });
}

void SegmentedSwitchAttachment::pushControlFromParam()
{
    if (choice == nullptr) return;
    ctrl.setSelectedIndex (choice->getIndex());
}

void SegmentedSwitchAttachment::pushParamFromControl (int idx)
{
    if (param == nullptr || choice == nullptr) return;

    const int n = choice->choices.size();
    const float norm = (n <= 1) ? 0.0f : (float) idx / (float) (n - 1);

    param->beginChangeGesture();
    param->setValueNotifyingHost (norm);
    param->endChangeGesture();
}
