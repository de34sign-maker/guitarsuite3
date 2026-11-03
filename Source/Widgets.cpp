#include "Widgets.h"
#include "RogerThatAssets.h"

namespace rt
{

// ---- Fonts -------------------------------------------------------------------------------------
static juce::Typeface::Ptr typefaceFor(int which)
{
    static juce::Typeface::Ptr cache[3];
    if (cache[which] == nullptr)
    {
        switch (which)
        {
            case 0: cache[0] = juce::Typeface::createSystemTypefaceFor(RogerThatAssets::InterMedium_ttf,         RogerThatAssets::InterMedium_ttfSize); break;
            case 1: cache[1] = juce::Typeface::createSystemTypefaceFor(RogerThatAssets::InterSemiBold_ttf,       RogerThatAssets::InterSemiBold_ttfSize); break;
            default: cache[2] = juce::Typeface::createSystemTypefaceFor(RogerThatAssets::JetBrainsMonoMedium_ttf, RogerThatAssets::JetBrainsMonoMedium_ttfSize); break;
        }
    }
    return cache[which];
}

juce::Font Fonts::inter(float px, float kerning, bool semibold)
{
    return juce::Font(juce::FontOptions(typefaceFor(semibold ? 1 : 0)).withPointHeight(px).withKerningFactor(kerning));
}
juce::Font Fonts::mono(float px, float kerning)
{
    return juce::Font(juce::FontOptions(typefaceFor(2)).withPointHeight(px).withKerningFactor(kerning));
}

// ---- Shadows -----------------------------------------------------------------------------------
void drawDiscShadow(juce::Graphics& g, juce::Rectangle<float> disc)
{
    juce::Path p; p.addEllipse(disc);
    juce::DropShadow(juce::Colours::black.withAlpha(0.12f), 8, { 0, 4 }).drawForPath(g, p);
    juce::DropShadow(juce::Colours::black.withAlpha(0.06f), 1, { 0, 1 }).drawForPath(g, p);
}

void drawPanelShadow(juce::Graphics& g, juce::Rectangle<float> rect, float cornerRadius)
{
    juce::Path p; p.addRoundedRectangle(rect, cornerRadius);
    juce::DropShadow(juce::Colours::black.withAlpha(0.08f), 10, { 0, 2 }).drawForPath(g, p);
    juce::DropShadow(juce::Colours::black.withAlpha(0.04f), 1, { 0, 1 }).drawForPath(g, p);
}

// ---- Knob --------------------------------------------------------------------------------------
Knob::Knob()
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    setRange(0.0, 100.0, 0.1);
    setDoubleClickReturnValue(false, 0.0);
    setMouseDragSensitivity(220);
    setScrollWheelEnabled(true);

    value.setJustificationType(juce::Justification::centred);
    value.setFont(Fonts::mono(19.f));
    value.setColour(juce::Label::textColourId, theme::ink);
    value.setColour(juce::Label::textWhenEditingColourId, theme::ink);
    value.setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
    value.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    value.setColour(juce::TextEditor::textColourId, theme::ink);
    value.setColour(juce::TextEditor::highlightColourId, theme::accent.withAlpha(0.25f));
    value.setColour(juce::TextEditor::highlightedTextColourId, theme::ink);
    value.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    value.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    value.setColour(juce::CaretComponent::caretColourId, theme::accent);
    value.setInterceptsMouseClicks(false, true);   // drags go to the knob; the editor (child) still works
    value.setEditable(false, false, false);
    value.onEditorShow = [this]
    {
        if (auto* ed = value.getCurrentTextEditor())
        {
            ed->setJustification(juce::Justification::centred);
            ed->setInputRestrictions(3, "0123456789");
            ed->applyFontToAllText(Fonts::mono(19.f));
            ed->selectAll();
        }
    };
    value.onTextChange = [this]
    {
        const auto v = juce::jlimit(0.0, 100.0, value.getText().retainCharacters("0123456789").getDoubleValue());
        setValue(v, juce::sendNotificationSync);
        updateText();
    };
    addAndMakeVisible(value);
    updateText();
}

void Knob::resized()
{
    value.setBounds(getWidth() / 2 - 30, getHeight() / 2 - 15, 60, 30);
}

void Knob::valueChanged() { updateText(); }

void Knob::updateText()
{
    if (! value.isBeingEdited())
        value.setText(juce::String(juce::roundToInt(getValue())), juce::dontSendNotification);
}

void Knob::mouseDoubleClick(const juce::MouseEvent&)
{
    value.showEditor();
}

void Knob::paint(juce::Graphics& g)
{
    const float cx = getWidth() * 0.5f, cy = getHeight() * 0.5f;
    const float discR = 36.f, ringR = 41.f;
    const auto disc = juce::Rectangle<float>(cx - discR, cy - discR, discR * 2, discR * 2);

    drawDiscShadow(g, disc);
    g.setColour(theme::disc);
    g.fillEllipse(disc);

    const float a0 = juce::degreesToRadians(-135.f), a1 = juce::degreesToRadians(135.f);
    const float prop = (float) valueToProportionOfLength(getValue());
    const float a = a0 + (a1 - a0) * prop;

    juce::Path track; track.addCentredArc(cx, cy, ringR, ringR, 0.f, a0, a1, true);
    g.setColour(theme::track);
    g.strokePath(track, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (prop > 0.005f)
    {
        juce::Path arc; arc.addCentredArc(cx, cy, ringR, ringR, 0.f, a0, a, true);
        g.setColour(theme::accent);
        g.strokePath(arc, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float dx = cx + ringR * std::sin(a), dy = cy - ringR * std::cos(a);
    g.setColour(theme::accent);
    g.fillEllipse(dx - 4.f, dy - 4.f, 8.f, 8.f);
}

// ---- UnderlineToggle ---------------------------------------------------------------------------
UnderlineToggle::UnderlineToggle(const juce::String& text) : juce::Button(text)
{
    setClickingTogglesState(true);
}

void UnderlineToggle::paintButton(juce::Graphics& g, bool highlighted, bool)
{
    const bool on = getToggleState();
    auto font = Fonts::inter(11.f, 0.14f);
    g.setFont(font);
    g.setColour(on ? theme::ink : (highlighted ? theme::muted : theme::faint));
    const auto text = getButtonText();
    const int textW = juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, text));
    const int lineW = textW + 20;
    auto r = getLocalBounds();
    g.drawText(text, r.withHeight(r.getHeight() - 4), juce::Justification::centred);
    g.setColour(on ? theme::accent : theme::track);
    g.fillRect(r.getCentreX() - lineW / 2, r.getBottom() - 4, lineW, 2);
}

// ---- PillToggle --------------------------------------------------------------------------------
PillToggle::PillToggle(const juce::String& text) : juce::Button(text)
{
    setClickingTogglesState(true);
}

void PillToggle::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    const bool on = getToggleState();
    auto pill = getLocalBounds().toFloat().reduced(6.f);
    if (down) pill.translate(0.f, 1.f);

    juce::Path p; p.addRoundedRectangle(pill, pill.getHeight() * 0.5f);
    if (! down)
    {
        juce::DropShadow(juce::Colours::black.withAlpha(0.12f), 6, { 0, 3 }).drawForPath(g, p);
        juce::DropShadow(juce::Colours::black.withAlpha(0.06f), 1, { 0, 1 }).drawForPath(g, p);
    }
    g.setColour(highlighted ? theme::disc.brighter(0.03f) : theme::disc);
    g.fillPath(p);

    auto font = Fonts::inter(11.f, 0.14f);
    g.setFont(font);
    const auto text = getButtonText();
    const float textW = juce::GlyphArrangement::getStringWidth(font, text);
    const float total = 6.f + 8.f + textW;
    const float x0 = pill.getCentreX() - total * 0.5f;

    g.setColour(on ? theme::accent : theme::offDot);
    g.fillEllipse(x0, pill.getCentreY() - 3.f, 6.f, 6.f);
    g.setColour(on ? theme::ink : theme::faint);
    g.drawText(text, juce::Rectangle<float>(x0 + 14.f, pill.getY(), textW + 4.f, pill.getHeight()), juce::Justification::centredLeft);
}

// ---- Brand mark --------------------------------------------------------------------------------
void drawBrandMark(juce::Graphics& g, juce::Rectangle<float> box, juce::Colour colour)
{
    // a dot in the lower-left with two "broadcast" arcs opening to the upper-right
    const float s = box.getWidth();
    const float cx = box.getX() + s * 0.30f, cy = box.getY() + s * 0.70f;
    const float dotR = s * 0.15f;
    g.setColour(colour);
    g.fillEllipse(cx - dotR, cy - dotR, dotR * 2.f, dotR * 2.f);

    juce::Path arcs;
    for (float r : { s * 0.42f, s * 0.68f })
        arcs.addCentredArc(cx, cy, r, r, 0.f, 0.f, juce::MathConstants<float>::halfPi, true);
    g.strokePath(arcs, juce::PathStrokeType(s * 0.12f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ---- BrandButton -------------------------------------------------------------------------------
BrandButton::BrandButton() : juce::Button("vst34")
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setTooltip("About RogerThat");
}

void BrandButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    if (down) r.translate(0.f, 1.f);

    auto font = Fonts::mono(9.f, 0.18f);
    g.setFont(font);
    const float textW = juce::GlyphArrangement::getStringWidth(font, "vst34");
    const float markS = 12.f, gap = 6.f;
    const float x0 = r.getRight() - (markS + gap + textW);
    const float cy = r.getCentreY();

    drawBrandMark(g, { x0, cy - markS * 0.5f, markS, markS }, highlighted ? theme::accent : theme::accent.withAlpha(0.6f));
    g.setColour(highlighted ? theme::muted : theme::faint);
    g.drawText("vst34", juce::Rectangle<float>(x0 + markS + gap, r.getY(), textW + 2.f, r.getHeight()), juce::Justification::centredLeft);
}

// ---- LinkPill ----------------------------------------------------------------------------------
LinkPill::LinkPill(const juce::String& text, const juce::String& u) : juce::Button(text), url(u)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    onClick = [this] { juce::URL(url).launchInDefaultBrowser(); };
}

void LinkPill::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto pill = getLocalBounds().toFloat().reduced(4.f);
    if (down) pill.translate(0.f, 1.f);

    juce::Path p; p.addRoundedRectangle(pill, pill.getHeight() * 0.5f);
    if (! down)
    {
        juce::DropShadow(juce::Colours::black.withAlpha(0.10f), 5, { 0, 2 }).drawForPath(g, p);
        juce::DropShadow(juce::Colours::black.withAlpha(0.05f), 1, { 0, 1 }).drawForPath(g, p);
    }
    g.setColour(highlighted ? theme::disc.brighter(0.03f) : theme::disc);
    g.fillPath(p);

    auto font = Fonts::inter(11.f, 0.06f, true);
    g.setFont(font);
    const auto text = getButtonText();
    const float textW = juce::GlyphArrangement::getStringWidth(font, text);
    const float arrowS = 7.f, gap = 7.f;
    const float x0 = pill.getCentreX() - (textW + gap + arrowS) * 0.5f;

    g.setColour(highlighted ? theme::ink : theme::muted);
    g.drawText(text, juce::Rectangle<float>(x0, pill.getY(), textW + 2.f, pill.getHeight()), juce::Justification::centredLeft);

    // small "open outside" arrow: diagonal line with a corner head, pointing up-right
    const float ax = x0 + textW + gap, ay = pill.getCentreY() + arrowS * 0.5f - 0.5f;
    juce::Path a;
    a.startNewSubPath(ax, ay);                 a.lineTo(ax + arrowS, ay - arrowS);
    a.startNewSubPath(ax + arrowS * 0.35f, ay - arrowS); a.lineTo(ax + arrowS, ay - arrowS); a.lineTo(ax + arrowS, ay - arrowS * 0.35f);
    g.setColour(theme::accent);
    g.strokePath(a, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ---- AboutPanel --------------------------------------------------------------------------------
AboutPanel::AboutPanel()
{
    setWantsKeyboardFocus(true);
    for (auto* b : { &spotify, &telegram, &instagram, &email }) addAndMakeVisible(b);
    setVisible(false);
}

void AboutPanel::open()
{
    setVisible(true);
    toFront(true);
    grabKeyboardFocus();
}

void AboutPanel::close() { setVisible(false); }

void AboutPanel::resized()
{
    const int cw = 404, ch = 236;
    card = getLocalBounds().withSizeKeepingCentre(cw, ch);

    // link pills row, bottom part of the card
    const int pillH = 36, pillW = 86, gap = 6;
    int x = card.getX() + 24 - 4;            // −4: pill has 4 px of shadow spill on each side
    const int y = card.getBottom() - 24 - 18 - 12 - pillH + 4;
    for (auto* b : { &spotify, &telegram, &instagram, &email })
    {
        b->setBounds(x, y, pillW, pillH);
        x += pillW + gap - 8;
    }
}

void AboutPanel::paint(juce::Graphics& g)
{
    g.fillAll(theme::ink.withAlpha(0.18f));

    auto c = card.toFloat();
    juce::Path p; p.addRoundedRectangle(c, 14.f);
    juce::DropShadow(juce::Colours::black.withAlpha(0.18f), 24, { 0, 10 }).drawForPath(g, p);
    g.setColour(theme::panel);
    g.fillPath(p);

    const float pad = 24.f;
    float x = c.getX() + pad, y = c.getY() + pad;

    // header: mark + name
    drawBrandMark(g, { x, y, 22.f, 22.f }, theme::accent);
    g.setFont(Fonts::inter(15.f, 0.f, true));
    g.setColour(theme::ink);
    g.drawText("RogerThat", juce::Rectangle<float>(x + 32.f, y - 1.f, 200.f, 24.f), juce::Justification::centredLeft);
    g.setFont(Fonts::mono(10.f, 0.18f));
    g.setColour(theme::faint);
    g.drawText("vst34  ·  v" JucePlugin_VersionString, juce::Rectangle<float>(x + 32.f, y + 18.f, 200.f, 14.f), juce::Justification::centredLeft);

    // close cross, top-right
    const auto cr = juce::Rectangle<float>(c.getRight() - 40.f, c.getY() + 16.f, 24.f, 24.f);
    const bool overClose = cr.contains(getMouseXYRelative().toFloat());
    g.setColour(overClose ? theme::ink : theme::faint);
    const float k = 4.5f; const auto cc = cr.getCentre();
    g.drawLine(cc.x - k, cc.y - k, cc.x + k, cc.y + k, 1.5f);
    g.drawLine(cc.x - k, cc.y + k, cc.x + k, cc.y - k, 1.5f);

    // two columns: creator / artist
    y += 56.f;
    auto caption = Fonts::inter(9.5f, 0.14f);
    auto value   = Fonts::inter(14.f, 0.f, true);
    const float colW = (c.getWidth() - pad * 2.f) * 0.5f;

    g.setFont(caption); g.setColour(theme::faint);
    g.drawText("CREATED BY", juce::Rectangle<float>(x, y, colW, 14.f), juce::Justification::centredLeft);
    g.drawText("ARTIST",     juce::Rectangle<float>(x + colW, y, colW, 14.f), juce::Justification::centredLeft);
    g.setFont(value); g.setColour(theme::ink);
    g.drawText("Yevhenii Bondar", juce::Rectangle<float>(x, y + 16.f, colW, 22.f), juce::Justification::centredLeft);
    g.drawText(juce::String::fromUTF8("\xd0\xa0\x33\xd0\x99\xd0\x9c\x34\xd0\x9d"), // Р3ЙМ4Н
               juce::Rectangle<float>(x + colW, y + 16.f, colW, 22.f), juce::Justification::centredLeft);

    // links caption (pills are child components placed in resized())
    y += 54.f;
    g.setFont(caption); g.setColour(theme::faint);
    g.drawText("FIND ME", juce::Rectangle<float>(x, y, colW, 14.f), juce::Justification::centredLeft);

    // email in plain text so it can be read / retyped
    g.setFont(Fonts::mono(9.5f, 0.06f)); g.setColour(theme::legend);
    g.drawText("iamyevheniibondar@gmail.com", juce::Rectangle<float>(x, c.getBottom() - pad - 14.f, c.getWidth() - pad * 2.f, 14.f), juce::Justification::centredLeft);
}

void AboutPanel::mouseDown(const juce::MouseEvent& e)
{
    const auto closeBox = juce::Rectangle<int>(card.getRight() - 44, card.getY() + 12, 32, 32);
    if (! card.contains(e.getPosition()) || closeBox.contains(e.getPosition()))
        close();
}

bool AboutPanel::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey) { close(); return true; }
    return false;
}

// ---- ScopeView ---------------------------------------------------------------------------------
void ScopeView::paint(juce::Graphics& g)
{
    const int w = getWidth(), h = getHeight();
    const float mid = h * 0.5f;
    const float H = mid * 0.92f;          // full-scale half height
    const float gain = 2.0f;              // −6 dBFS peak fills the display

    g.fillAll(theme::panel);
    g.setColour(theme::centerLine);
    g.fillRect(0.f, mid - 0.5f, (float) w, 1.f);

    const int cols = juce::jmin(w, ScopeHistory::size);
    const int wi = hist.writeIndex.load(std::memory_order_acquire);

    juce::Path dry, fx;
    // input keeps a hairline in silence; the FX layer disappears completely so the centre line stays grey
    auto heightOf = [&](float peak, float minH) { return juce::jmax(minH, juce::jmin(1.f, peak * gain) * H); };

    for (int pass = 0; pass < 2; ++pass)
    {
        auto& path = pass == 0 ? dry : fx;
        auto& arr  = pass == 0 ? hist.in : hist.fx;
        const float minH = pass == 0 ? 0.6f : 0.f;
        // top edge left→right
        for (int i = 0; i < cols; ++i)
        {
            const int idx = (wi - cols + i) & (ScopeHistory::size - 1);
            const float y = mid - heightOf(arr[(size_t) idx].load(std::memory_order_relaxed), minH);
            if (i == 0) path.startNewSubPath((float) (w - cols), y); else path.lineTo((float) (w - cols + i), y);
        }
        // bottom edge right→left
        for (int i = cols - 1; i >= 0; --i)
        {
            const int idx = (wi - cols + i) & (ScopeHistory::size - 1);
            path.lineTo((float) (w - cols + i), mid + heightOf(arr[(size_t) idx].load(std::memory_order_relaxed), minH));
        }
        path.closeSubPath();
    }

    g.setColour(theme::dryFill);
    g.fillPath(dry);
    g.setColour(theme::accent.withAlpha(0.92f));
    g.fillPath(fx);

    // legend
    auto font = Fonts::inter(10.f, 0.12f);
    g.setFont(font);
    g.setColour(theme::offDot);
    g.fillRect(16.f, 17.f, 10.f, 3.f);
    g.setColour(theme::legend);
    g.drawText("IN", 32, 11, 30, 14, juce::Justification::centredLeft);
    g.setColour(theme::accent);
    g.fillRect(62.f, 17.f, 10.f, 3.f);
    g.setColour(theme::legend);
    g.drawText("FX", 78, 11, 30, 14, juce::Justification::centredLeft);
}

} // namespace rt
