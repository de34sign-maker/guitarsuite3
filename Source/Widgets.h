#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace rt
{

// ---- Theme (mockup "D": light grey, soft shadows, pink accent) --------------------------------
namespace theme
{
    const juce::Colour bg         { 0xffECEDEF };
    const juce::Colour panel      { 0xffF7F8F9 };
    const juce::Colour disc       { 0xffF6F7F8 };
    const juce::Colour track      { 0xffD8DBDF };
    const juce::Colour accent     { 0xffF2416B };
    const juce::Colour ink        { 0xff1D1F22 };
    const juce::Colour muted      { 0xff5C6168 };
    const juce::Colour faint      { 0xff9AA0A8 };
    const juce::Colour legend     { 0xff6B7078 };
    const juce::Colour dryFill    { 0xffD5D8DC };
    const juce::Colour centerLine { 0xffE3E5E8 };
    const juce::Colour offDot     { 0xffC3C7CD };
}

// ---- Embedded fonts ----------------------------------------------------------------------------
struct Fonts
{
    static juce::Font inter(float px, float kerning = 0.f, bool semibold = false);
    static juce::Font mono(float px, float kerning = 0.f);
    static juce::Typeface::Ptr interTypeface(bool semibold = false);
};

// Soft drop shadow helpers (drawn by the component that owns the surrounding space)
void drawDiscShadow(juce::Graphics& g, juce::Rectangle<float> disc);
void drawPanelShadow(juce::Graphics& g, juce::Rectangle<float> rect, float cornerRadius);

// ---- Rotary knob with the value inside; double-click the number to type it --------------------
class Knob : public juce::Slider
{
public:
    static constexpr int sizePx = 100;   // component size (disc + ring + shadow spill)

    Knob();
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void valueChanged() override;

private:
    void updateText();
    juce::Label value;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};

// ---- Block toggle drawn as a caption with an accent underline (COMP / FUZZ) ---------------------
class UnderlineToggle : public juce::Button
{
public:
    explicit UnderlineToggle(const juce::String& text);
    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
};

// ---- Block toggle drawn as a raised pill with a dot (FILTER / BOX) ----------------------------
class PillToggle : public juce::Button
{
public:
    static constexpr int widthPx = 84, heightPx = 56;   // includes shadow spill (pill is 72x44)
    explicit PillToggle(const juce::String& text);
    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
};

// ---- Small "open outside" arrow (↗), the plugin's "this is clickable / this is a link" glyph ----
void drawArrowOut(juce::Graphics& g, juce::Rectangle<float> box, juce::Colour colour, float thickness = 1.5f);

// ---- Bottom-right "↗ vst34" badge; click opens the About panel ---------------------------------
class BrandButton : public juce::Button
{
public:
    static constexpr int widthPx = 84, heightPx = 22;
    BrandButton();
    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
};

// ---- One link row inside the About panel: name · address · ↗ (opens a URL) --------------------
class LinkRow : public juce::Button
{
public:
    LinkRow(const juce::String& name, const juce::String& shownAddress, const juce::String& url, bool lastRow);
    void paintButton(juce::Graphics&, bool highlighted, bool down) override;
private:
    juce::String shown, url;
    bool last;
};

// ---- About overlay: who made the plugin and where to find the artist --------------------------
class AboutPanel : public juce::Component
{
public:
    AboutPanel();
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;     // click outside the card closes
    void mouseMove(const juce::MouseEvent&) override { repaint(); }   // hover on the close cross
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
    bool keyPressed(const juce::KeyPress&) override;      // Esc closes
    void open();
    void close();

private:
    juce::Rectangle<int> card;
    juce::Image icon;
    LinkRow spotify   { "Spotify",   "open.spotify.com/artist/4WJKwDv6cDbc1hDyL0xnBg", "https://open.spotify.com/artist/4WJKwDv6cDbc1hDyL0xnBg", false };
    LinkRow telegram  { "Telegram",  "t.me/callmerayman",                               "https://t.me/callmerayman", false };
    LinkRow instagram { "Instagram", "instagram.com/callme34_",                         "https://www.instagram.com/callme34_/", false };
    LinkRow email     { "Email",     "iamyevheniibondar@gmail.com",                     "mailto:iamyevheniibondar@gmail.com", true };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutPanel)
};

// ---- Scrolling waveform: input (grey) and what the plugin adds (accent) -----------------------
class ScopeView : public juce::Component
{
public:
    explicit ScopeView(const ScopeHistory& history) : hist(history) {}
    void paint(juce::Graphics&) override;
private:
    const ScopeHistory& hist;
};

} // namespace rt
