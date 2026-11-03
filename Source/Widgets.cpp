#include "Widgets.h"
#include "RogerThatAssets.h"

namespace rt
{

// ---- Fonts -------------------------------------------------------------------------------------
static juce::Typeface::Ptr typefaceFor(int which)
{
    // heap-allocated on purpose: a static Typeface::Ptr would be destroyed after JUCE has shut down
    static juce::Typeface::Ptr* cache = new juce::Typeface::Ptr[3];
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
juce::Typeface::Ptr Fonts::interTypeface(bool semibold) { return typefaceFor(semibold ? 1 : 0); }

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

// ---- Arrow ↗ ----------------------------------------------------------------------------------
void drawArrowOut(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour colour, float thickness)
{
    // diagonal from bottom-left to top-right, with an L-shaped head in the top-right corner
    const float x0 = b.getX(), y0 = b.getBottom(), x1 = b.getRight(), y1 = b.getY();
    const float head = b.getWidth() * 0.45f;
    juce::Path p;
    p.startNewSubPath(x0, y0);        p.lineTo(x1, y1);
    p.startNewSubPath(x1 - head, y1); p.lineTo(x1, y1); p.lineTo(x1, y1 + head);
    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ---- BrandButton -------------------------------------------------------------------------------
BrandButton::BrandButton() : juce::Button("vst34")
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void BrandButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    if (down) r.translate(0.f, 1.f);

    auto font = Fonts::mono(11.f, 0.16f);
    g.setFont(font);
    const float textW = juce::GlyphArrangement::getStringWidth(font, "vst34");
    const float arrowS = 10.f, gap = 6.f;
    const float x0 = r.getRight() - (arrowS + gap + textW);
    const float cy = r.getCentreY();

    drawArrowOut(g, { x0, cy - arrowS * 0.5f, arrowS, arrowS }, theme::accent, 1.7f);
    g.setColour(highlighted ? theme::ink : theme::muted);
    g.drawText("vst34", juce::Rectangle<float>(x0 + arrowS + gap, r.getY(), textW + 2.f, r.getHeight()), juce::Justification::centredLeft);
}

// ---- LinkRow -----------------------------------------------------------------------------------
LinkRow::LinkRow(const juce::String& name, const juce::String& shownAddress, const juce::String& u, bool lastRow)
    : juce::Button(name), shown(shownAddress), url(u), last(lastRow)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    onClick = [this] { juce::URL(url).launchInDefaultBrowser(); };
}

void LinkRow::paintButton(juce::Graphics& g, bool highlighted, bool)
{
    auto r = getLocalBounds().toFloat();

    g.setColour(theme::centerLine);
    g.fillRect(r.getX(), r.getY(), r.getWidth(), 1.f);
    if (last) g.fillRect(r.getX(), r.getBottom() - 1.f, r.getWidth(), 1.f);

    const float nameW = 110.f, arrowS = 9.f;
    g.setFont(Fonts::inter(13.f, 0.f, true));
    g.setColour(theme::ink);
    g.drawText(getButtonText(), juce::Rectangle<float>(r.getX(), r.getY(), nameW, r.getHeight()), juce::Justification::centredLeft);

    g.setFont(Fonts::mono(10.5f, 0.02f));
    g.setColour(highlighted ? theme::ink : theme::legend);
    g.drawText(shown, juce::Rectangle<float>(r.getX() + nameW, r.getY(), r.getWidth() - nameW - arrowS - 14.f, r.getHeight()),
               juce::Justification::centredLeft, true);

    drawArrowOut(g, { r.getRight() - arrowS, r.getCentreY() - arrowS * 0.5f, arrowS, arrowS }, theme::accent, 1.6f);
}

// ---- AboutPanel --------------------------------------------------------------------------------
AboutPanel::AboutPanel()
{
    setWantsKeyboardFocus(true);
    icon = juce::ImageCache::getFromMemory(RogerThatAssets::walkie_256_png, RogerThatAssets::walkie_256_pngSize);
    for (auto* b : { &spotify, &telegram, &instagram, &email }) addAndMakeVisible(b);

    const std::pair<PillToggle*, float> scales[] { { &scale100, 1.f }, { &scale125, 1.25f }, { &scale150, 1.5f }, { &scale200, 2.f } };
    for (auto& [b, v] : scales)
    {
        b->setRadioGroupId(34);
        const float value = v;
        b->onClick = [this, value] { if (onScaleChosen) onScaleChosen(value); };
        addAndMakeVisible(b);
    }
    scale100.setToggleState(true, juce::dontSendNotification);
    setVisible(false);
}

void AboutPanel::setScale(float s)
{
    PillToggle* which = s >= 1.9f ? &scale200 : s >= 1.4f ? &scale150 : s >= 1.2f ? &scale125 : &scale100;
    which->setToggleState(true, juce::dontSendNotification);
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
    const int cw = 480, ch = 378, pad = 28;
    card = getLocalBounds().withSizeKeepingCentre(cw, ch);

    // UI size pills: PillToggle draws its pill inside a 6 px shadow margin, so 84x44 bounds = a 72x32 pill
    const int pillW = PillToggle::widthPx, boundsH = 32 + 12;
    int x = card.getX() + pad - 6;
    const int py = card.getY() + 158 - 6;
    for (auto* b : { &scale100, &scale125, &scale150, &scale200 })
    {
        b->setBounds(x, py, pillW, boundsH);
        x += pillW - 12 + 8;
    }

    // four link rows at the bottom of the card
    const int rowH = 36;
    int y = card.getBottom() - 20 - rowH * 4;
    for (auto* b : { &spotify, &telegram, &instagram, &email })
    {
        b->setBounds(card.getX() + pad, y, cw - pad * 2, rowH);
        y += rowH;
    }
}

void AboutPanel::paint(juce::Graphics& g)
{
    g.fillAll(theme::ink.withAlpha(0.18f));

    auto c = card.toFloat();
    juce::Path p; p.addRoundedRectangle(c, 16.f);
    juce::DropShadow(juce::Colours::black.withAlpha(0.18f), 28, { 0, 12 }).drawForPath(g, p);
    g.setColour(theme::panel);
    g.fillPath(p);

    const float pad = 28.f;
    const float x = c.getX() + pad, y = c.getY() + 24.f;

    // header: purchased icon (48 px, kept in proportion) + name / "vst34 · version"
    const float iconH = 48.f;
    const float iconW = icon.isValid() ? iconH * (float) icon.getWidth() / (float) icon.getHeight() : iconH;
    if (icon.isValid())
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(icon, { x, y, iconW, iconH }, juce::RectanglePlacement::centred);
    }
    const float tx = x + iconW + 18.f;
    g.setFont(Fonts::inter(18.f, 0.f, true));
    g.setColour(theme::ink);
    g.drawText("RogerThat", juce::Rectangle<float>(tx, y + 2.f, 240.f, 22.f), juce::Justification::centredLeft);
    g.setFont(Fonts::mono(11.f, 0.14f));
    g.setColour(theme::faint);
    g.drawText(juce::String::fromUTF8("vst34 \xc2\xb7 v") + JucePlugin_VersionString,   // "vst34 · v1.5.0"
               juce::Rectangle<float>(tx, y + 30.f, 240.f, 14.f), juce::Justification::centredLeft);

    // close cross, top-right
    const auto cr = juce::Rectangle<float>(c.getRight() - pad - 20.f, c.getY() + 22.f, 24.f, 24.f);
    const bool overClose = cr.expanded(6.f).contains(getMouseXYRelative().toFloat());
    g.setColour(overClose ? theme::ink : theme::faint);
    const float k = 4.5f; const auto cc = cr.getCentre();
    g.drawLine(cc.x - k, cc.y - k, cc.x + k, cc.y + k, 1.6f);
    g.drawLine(cc.x - k, cc.y + k, cc.x + k, cc.y - k, 1.6f);

    // two columns: creator / artist
    const float y2 = c.getY() + 92.f;
    const float colW = (c.getWidth() - pad * 2.f) * 0.5f;
    auto caption = Fonts::inter(11.f, 0.14f);
    g.setFont(caption); g.setColour(theme::faint);
    g.drawText("CREATED BY", juce::Rectangle<float>(x, y2, colW, 14.f), juce::Justification::centredLeft);
    g.drawText("ARTIST",     juce::Rectangle<float>(x + colW, y2, colW, 14.f), juce::Justification::centredLeft);
    g.setFont(Fonts::inter(15.f, 0.f, true)); g.setColour(theme::ink);
    g.drawText("Yevhenii Bondar", juce::Rectangle<float>(x, y2 + 18.f, colW, 20.f), juce::Justification::centredLeft);
    g.drawText(juce::String::fromUTF8("\xd0\xa0\x33\xd0\x99\xd0\x9c\x34\xd0\x9d"), // Р3ЙМ4Н
               juce::Rectangle<float>(x + colW, y2 + 18.f, colW, 20.f), juce::Justification::centredLeft);

    // UI size caption (pills are child components placed in resized())
    g.setFont(caption); g.setColour(theme::faint);
    g.drawText("UI SIZE", juce::Rectangle<float>(x, c.getY() + 140.f, colW, 14.f), juce::Justification::centredLeft);
}

void AboutPanel::mouseDown(const juce::MouseEvent& e)
{
    const auto closeBox = juce::Rectangle<int>(card.getRight() - 28 - 26, card.getY() + 16, 36, 36);
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
