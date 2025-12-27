#include "SegmentedSwitch.h"
#include "AuricHelpers.h"

#include <cmath>

//==============================================================================
// Local theme (matte hardware)
namespace SegTheme
{
    static inline juce::Colour goldText() { return juce::Colour::fromRGB (200, 170, 110); }
    static inline juce::Colour goldHi()   { return juce::Colour::fromRGB (220, 190, 130); }

    static inline juce::Colour bg0()      { return juce::Colour::fromRGB (10, 10, 10); }
    static inline juce::Colour bg1()      { return juce::Colour::fromRGB (18, 18, 18); }
    static inline juce::Colour rimHi()    { return juce::Colour::fromRGB (60, 60, 60); }
    static inline juce::Colour rimLo()    { return juce::Colour::fromRGB (5, 5, 5); }
}

static inline float clampf (float v, float lo, float hi) { return juce::jlimit (lo, hi, v); }

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

// Path segmen: only outer corners rounded (tanpa overload 12 arg)
static juce::Path makeSegmentPath (juce::Rectangle<float> r, float cr,
                                  bool roundLeft, bool roundRight)
{
    using namespace juce;

    Path p;
    cr = jmin (cr, r.getWidth() * 0.5f, r.getHeight() * 0.5f);

    const float x  = r.getX();
    const float y  = r.getY();
    const float xr = r.getRight();
    const float yb = r.getBottom();

    p.startNewSubPath (x + (roundLeft ? cr : 0.0f), y);

    // top -> right
    p.lineTo (xr - (roundRight ? cr : 0.0f), y);
    if (roundRight) p.quadraticTo (xr, y, xr, y + cr);
    else            p.lineTo (xr, y);

    // right -> bottom
    p.lineTo (xr, yb - (roundRight ? cr : 0.0f));
    if (roundRight) p.quadraticTo (xr, yb, xr - cr, yb);
    else            p.lineTo (xr, yb);

    // bottom -> left
    p.lineTo (x + (roundLeft ? cr : 0.0f), yb);
    if (roundLeft)  p.quadraticTo (x, yb, x, yb - cr);
    else            p.lineTo (x, yb);

    // left -> top
    p.lineTo (x, y + (roundLeft ? cr : 0.0f));
    if (roundLeft)  p.quadraticTo (x, y, x + cr, y);
    else            p.lineTo (x, y);

    p.closeSubPath();
    return p;
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
}

void SegmentedSwitch3::mouseExit (const juce::MouseEvent&)
{
    if (hovered != -1) { hovered = -1; repaint(); }
}

void SegmentedSwitch3::mouseDown (const juce::MouseEvent& e)
{
    pressed = hitTestIndex (e.getPosition());
    repaint();
}

void SegmentedSwitch3::mouseDrag (const juce::MouseEvent& e)
{
    const int p = hitTestIndex (e.getPosition());
    if (p != pressed) { pressed = p; repaint(); }
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

void SegmentedSwitch3::paint (juce::Graphics& g)
{
    using namespace juce;

    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    auto bounds = getLocalBounds().toFloat().reduced (0.6f);
    if (bounds.getHeight() < 10.0f) return;

    const bool hasLegend = drawLeftLegend && (leftText.isNotEmpty() || leftSymbol.isNotEmpty());
    const bool hasSub =
        subLabels[0].isNotEmpty() || subLabels[1].isNotEmpty() || subLabels[2].isNotEmpty();

    int legendW = 0;
    if (hasLegend)
        legendW = (legendWidthOverride > 0) ? legendWidthOverride
                                            : jmin (120, (int) (bounds.getWidth() * 0.30f));

    const float rrOuter = clampf (bounds.getHeight() * 0.26f, 8.0f, 14.0f);
    const float rrInner = rrOuter - 2.2f;

    //==================== OUTER CAPSULE ====================
    {
        g.setColour (Colour::fromRGBA (0, 0, 0, 160));
        g.fillRoundedRectangle (bounds.expanded (1.4f), rrOuter + 1.2f);

        ColourGradient bezel (SegTheme::rimHi().withAlpha (0.95f), bounds.getCentreX(), bounds.getY(),
                              SegTheme::rimLo().withAlpha (0.98f), bounds.getCentreX(), bounds.getBottom(), false);
        bezel.addColour (0.55, SegTheme::bg1().withAlpha (0.95f));

        g.setGradientFill (bezel);
        g.fillRoundedRectangle (bounds, rrOuter);

        g.setColour (Colour::fromRGBA (255, 255, 255, 12));
        g.drawRoundedRectangle (bounds.reduced (0.6f), rrOuter, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 210));
        g.drawRoundedRectangle (bounds.reduced (1.8f), rrOuter - 0.8f, 1.6f);
    }

    auto inner = bounds.reduced (2.6f);

    //==================== INNER WELL ====================
    {
        ColourGradient well (SegTheme::bg1(), inner.getCentreX(), inner.getY(),
                             SegTheme::bg0(), inner.getCentreX(), inner.getBottom(), false);
        well.addColour (0.55, Colour::fromRGB (12, 12, 12));

        g.setGradientFill (well);
        g.fillRoundedRectangle (inner, rrInner);

        g.setColour (Colour::fromRGBA (255, 255, 255, 8));
        g.drawRoundedRectangle (inner.reduced (0.6f), rrInner - 0.2f, 1.0f);

        g.setColour (Colour::fromRGBA (0, 0, 0, 220));
        g.drawRoundedRectangle (inner.reduced (1.2f), rrInner - 0.8f, 1.1f);
    }

    //==================== CLIP to inner capsule ====================
    Path clip;
    clip.addRoundedRectangle (inner, rrInner);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (clip);

    // ✅ FIX: hitung subH pakai inner height, bukan bounds height
    const float HIn = inner.getHeight();

    const float subH = (hasSub && HIn >= 30.0f)
                         ? jlimit (10.0f, 16.0f, HIn * 0.32f)
                         : 0.0f;

    const float btnH = jmax (10.0f, HIn - subH);

    // Areas
    auto btnArea = inner.withHeight (btnH);
    auto subArea = inner.withTrimmedTop (btnH);

    // Sub area padding biar teks nggak nabrak rounding bawah
    if (subH > 0.5f)
        subArea = subArea.reduced (2.0f, 1.0f);

    auto legendArea = btnArea.removeFromLeft ((float) legendW);
    auto segArea    = btnArea;

    //==================== LEFT LEGEND ====================
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
                    AuricHelpers::makeFont (fontSize, Font::plain),
                    gold.withAlpha (0.90f), Justification::centredLeft);

        // divider
        g.setColour (Colour::fromRGBA (0, 0, 0, 150));
        g.drawLine (segArea.getX(), segArea.getY() + 4.0f,
                    segArea.getX(), segArea.getBottom() - 4.0f, 1.2f);
    }

    //==================== SEGMENTS ====================
    auto segInner = segArea.reduced (4.0f, 3.0f);
    const float segW = segInner.getWidth() / 3.0f;

    const auto gold = SegTheme::goldText();
    const float fontSize = jlimit (12.0f, 18.0f, segInner.getHeight() * 0.62f);
    const float cr = jlimit (4.0f, 10.0f, segInner.getHeight() * 0.26f);

    for (int i = 0; i < 3; ++i)
    {
        auto cell = segInner.withX (segInner.getX() + segW * (float) i).withWidth (segW);

        const bool isSel   = (i == selected);
        const bool isHover = (i == hovered);
        const bool isDown  = (i == pressed);

        auto face = cell.reduced (2.0f, 2.0f);
        if (isDown && ! isSel) face = face.translated (0.0f, 1.0f);

        const bool roundL = (i == 0);
        const bool roundR = (i == 2);

        auto path = makeSegmentPath (face, cr, roundL, roundR);

        if (isSel)
        {
            ColourGradient plate (Colour::fromRGB (30, 30, 30), face.getCentreX(), face.getY(),
                                  Colour::fromRGB (12, 12, 12), face.getCentreX(), face.getBottom(), false);
            plate.addColour (0.55, Colour::fromRGB (18, 18, 18));

            g.setGradientFill (plate);
            g.fillPath (path);

            g.setColour (Colour::fromRGBA (255, 255, 255, 16));
            g.drawLine (face.getX() + 6.0f, face.getY() + 1.0f,
                        face.getRight() - 6.0f, face.getY() + 1.0f, 1.0f);

            g.setColour (SegTheme::goldHi().withAlpha (0.22f));
            g.strokePath (path, PathStrokeType (1.2f));

            g.setColour (Colour::fromRGBA (0, 0, 0, 165));
            g.strokePath (path, PathStrokeType (1.0f));
        }
        else
        {
            ColourGradient plate (Colour::fromRGB (22, 22, 22), face.getCentreX(), face.getY(),
                                  Colour::fromRGB (9, 9, 9),   face.getCentreX(), face.getBottom(), false);

            if (isDown)
                plate = ColourGradient (Colour::fromRGB (18, 18, 18), face.getCentreX(), face.getY(),
                                        Colour::fromRGB (7, 7, 7),     face.getCentreX(), face.getBottom(), false);

            g.setGradientFill (plate);
            g.fillPath (path);

            g.setColour (Colour::fromRGBA (0, 0, 0, 190));
            g.strokePath (path, PathStrokeType (1.2f));

            g.setColour (Colour::fromRGBA (255, 255, 255, 10));
            g.strokePath (path, PathStrokeType (1.0f), AffineTransform::translation (0.0f, -0.2f));

            if (isHover && ! isDown)
            {
                g.setColour (gold.withAlpha (0.030f));
                g.fillPath (path);
            }
        }

        if (i > 0)
        {
            const float x = cell.getX();
            g.setColour (Colour::fromRGBA (0, 0, 0, 140));
            g.drawLine (x, cell.getY() + 4.0f, x, cell.getBottom() - 4.0f, 1.0f);
        }

        auto textCol = gold.withAlpha (isSel ? 0.95f : 0.78f);
        if (isDown && ! isSel) textCol = textCol.withAlpha (0.70f);

        auto tb = cell.toNearestInt();
        tb.reduce (0, 1); // ✅ biar gak ke-clip atas/bawah
        if (isSel || (isDown && ! isSel)) tb = tb.translated (0, 1);

        embossText (g, labels[i], tb,
                    AuricHelpers::makeFont (fontSize, Font::plain),
                    textCol, Justification::centred);
    }

    //==================== SUB LABELS ====================
    if (subH > 0.5f)
    {
        const float subFont = jlimit (9.5f, 12.5f, subArea.getHeight() * 0.78f);
        auto f = AuricHelpers::makeFont (subFont, Font::plain);

        const float segW2 = segArea.getWidth() / 3.0f;

        for (int i = 0; i < 3; ++i)
        {
            auto cell = Rectangle<int> ((int) std::round (segArea.getX() + segW2 * i),
                                        (int) std::round (subArea.getY()),
                                        (int) std::round (segW2),
                                        (int) std::round (subArea.getHeight()));

            cell.reduce (0, 1); // ✅ safety padding

            g.setFont (f);
            g.setColour (Colour::fromRGBA (0, 0, 0, 120));
            g.drawText (subLabels[i], cell.translated (1, 1), Justification::centred, false);

            g.setColour (SegTheme::goldText().withAlpha (0.82f));
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
