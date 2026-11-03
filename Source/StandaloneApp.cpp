// Custom standalone application for RogerThat.
//
// Replaces JUCE's default StandaloneFilterWindow so the app matches the plugin's look:
//   • native macOS / Windows title bar (traffic lights on the left, rounded corners, no black frame)
//   • a 44 px strip under it: accent-coloured "input is muted" notice with a white SETTINGS pill,
//     or a plain strip with the SETTINGS pill and the current audio device
//   • a light "Audio Settings" dialog in the plugin's grey / white / accent palette
//
// Compiled into the shared code; only the Standalone target references juce_CreateApplication(),
// so VST3 / AU never pull this in (JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1 is set in CMake).

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "PluginProcessor.h"
#include "Widgets.h"

using namespace juce;

namespace rt
{

// ---- Look and feel for stock JUCE widgets (settings dialog): plugin palette + Inter ---------------
class LightLookAndFeel : public LookAndFeel_V4
{
public:
    LightLookAndFeel()
        : LookAndFeel_V4(ColourScheme(theme::bg,       // windowBackground
                                     theme::panel,    // widgetBackground
                                     theme::panel,    // menuBackground
                                     theme::track,    // outline
                                     theme::ink,      // defaultText
                                     theme::accent,   // defaultFill
                                     Colours::white,  // highlightedText
                                     theme::accent,   // highlightedFill
                                     theme::ink))     // menuText
    {
        setColour(ListBox::backgroundColourId, theme::panel);
        setColour(ListBox::outlineColourId, theme::track);
        setColour(ComboBox::backgroundColourId, theme::panel);
        setColour(ComboBox::outlineColourId, theme::track);
        setColour(ComboBox::arrowColourId, theme::muted);
        setColour(ComboBox::textColourId, theme::ink);
        setColour(PopupMenu::backgroundColourId, theme::panel);
        setColour(PopupMenu::highlightedBackgroundColourId, theme::accent);
        setColour(PopupMenu::highlightedTextColourId, Colours::white);
        setColour(PopupMenu::textColourId, theme::ink);
        setColour(Label::textColourId, theme::ink);
        setColour(ToggleButton::textColourId, theme::ink);
        setColour(ToggleButton::tickColourId, theme::accent);
        setColour(ToggleButton::tickDisabledColourId, theme::track);
        setColour(TextButton::buttonColourId, theme::panel);
        setColour(TextButton::textColourOffId, theme::ink);
        setColour(TextButton::textColourOnId, theme::ink);
        setColour(ScrollBar::thumbColourId, theme::track);
        setColour(AlertWindow::backgroundColourId, theme::bg);
        setColour(AlertWindow::textColourId, theme::ink);
        setColour(DialogWindow::backgroundColourId, theme::bg);
        setColour(ResizableWindow::backgroundColourId, theme::bg);
    }

    Typeface::Ptr getTypefaceForFont(const Font& f) override
    {
        return Fonts::interTypeface(f.isBold());
    }

    Font getComboBoxFont(ComboBox&) override            { return Fonts::inter(13.f); }

    // never squeeze combo-box text horizontally (JUCE's default condenses it to 70 %); clip with "..." instead
    void positionComboBoxText(ComboBox& box, Label& label) override
    {
        LookAndFeel_V4::positionComboBoxText(box, label);
        label.setMinimumHorizontalScale(1.0f);
    }
    Font getPopupMenuFont() override                    { return Fonts::inter(13.f); }
    Font getLabelFont(Label&) override                  { return Fonts::inter(13.f); }
    Font getTextButtonFont(TextButton&, int) override   { return Fonts::inter(12.f, 0.04f, true); }
    Font getAlertWindowMessageFont() override           { return Fonts::inter(13.f); }
    Font getAlertWindowTitleFont() override             { return Fonts::inter(15.f, 0.f, true); }
};

// ---- Gear glyph for the SETTINGS pill ---------------------------------------------------------
static void drawGear(Graphics& g, Rectangle<float> box, Colour colour)
{
    const auto c = box.getCentre();
    const float r = box.getWidth() * 0.5f;
    Path p;
    p.addEllipse(c.x - r * 0.28f, c.y - r * 0.28f, r * 0.56f, r * 0.56f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = MathConstants<float>::pi * 0.25f * (float) i;
        p.startNewSubPath(c.x + std::sin(a) * r * 0.62f, c.y - std::cos(a) * r * 0.62f);
        p.lineTo(c.x + std::sin(a) * r, c.y - std::cos(a) * r);
    }
    g.setColour(colour);
    g.strokePath(p, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));
}

// ---- SETTINGS pill: white on the accent strip, raised disc on the plain strip -------------------
class SettingsButton : public Button
{
public:
    SettingsButton() : Button("SETTINGS") { setMouseCursor(MouseCursor::PointingHandCursor); }
    void setOnAccent(bool b) { onAccent = b; repaint(); }

    void paintButton(Graphics& g, bool highlighted, bool down) override
    {
        auto pill = getLocalBounds().toFloat().reduced(4.f);
        if (down) pill.translate(0.f, 1.f);
        Path p; p.addRoundedRectangle(pill, pill.getHeight() * 0.5f);
        if (! down)
        {
            DropShadow(Colours::black.withAlpha(onAccent ? 0.14f : 0.12f), 6, { 0, 3 }).drawForPath(g, p);
            DropShadow(Colours::black.withAlpha(0.06f), 1, { 0, 1 }).drawForPath(g, p);
        }
        g.setColour(onAccent ? Colours::white : (highlighted ? theme::disc.brighter(0.03f) : theme::disc));
        g.fillPath(p);

        auto font = Fonts::inter(11.f, 0.10f, true);
        g.setFont(font);
        const float textW = GlyphArrangement::getStringWidth(font, getButtonText());
        const float gearS = 12.f, gap = 7.f;
        const float x0 = pill.getCentreX() - (gearS + gap + textW) * 0.5f;
        const Colour fg = onAccent ? theme::accent : theme::ink;
        drawGear(g, { x0, pill.getCentreY() - gearS * 0.5f, gearS, gearS }, fg);
        g.setColour(fg);
        g.drawText(getButtonText(), Rectangle<float>(x0 + gearS + gap, pill.getY(), textW + 2.f, pill.getHeight()), Justification::centredLeft);
    }
private:
    bool onAccent = false;
};

// ---- Audio settings dialog content --------------------------------------------------------------
class SettingsContent : public Component
{
public:
    SettingsContent(StandalonePluginHolder& h)
        : holder(h),
          selector(h.deviceManager, 0, 2, 0, 2, false, false, true, false),
          mute("Mute input to avoid feedback")
    {
        selector.setItemHeight(28);
        addAndMakeVisible(selector);
        mute.setClickingTogglesState(true);
        mute.getToggleStateValue().referTo(holder.getMuteInputValue());
        addAndMakeVisible(mute);
        setSize(640, 560);   // wide enough that long device names are not squeezed in the combo boxes
    }

    void paint(Graphics& g) override { g.fillAll(theme::bg); }

    void resized() override
    {
        auto r = getLocalBounds().reduced(16, 12);
        mute.setBounds(r.removeFromBottom(32));
        r.removeFromBottom(8);
        selector.setBounds(r);
    }

private:
    StandalonePluginHolder& holder;
    AudioDeviceSelectorComponent selector;
    ToggleButton mute;
};

// ---- Settings window: the close button just hides it (a plain DialogWindow does nothing on close) --
class SettingsWindow : public DialogWindow
{
public:
    explicit SettingsWindow(StandalonePluginHolder& h)
        : DialogWindow("Audio Settings", theme::bg, true, true)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new SettingsContent(h), true);
        setResizable(false, false);
    }
    void closeButtonPressed() override { setVisible(false); }
};

// ---- Strip under the title bar ------------------------------------------------------------------
class StripBar : public Component, private Value::Listener, private ChangeListener
{
public:
    static constexpr int heightPx = 44;

    explicit StripBar(StandalonePluginHolder& h) : holder(h)
    {
        muted.referTo(holder.getMuteInputValue());
        muted.addListener(this);
        holder.deviceManager.addChangeListener(this);
        settings.onClick = [this] { openSettings(); };
        addAndMakeVisible(settings);
        settings.setOnAccent(isMuted());
    }

    ~StripBar() override
    {
        holder.deviceManager.removeChangeListener(this);
        dialog = nullptr;   // owned here, destroyed before the holder's device manager
    }

    void paint(Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        if (isMuted())
        {
            g.fillAll(theme::accent);
            g.setFont(Fonts::inter(12.f, 0.02f));
            g.setColour(Colours::white);
            g.drawText("Audio input is muted to avoid a feedback loop", r.withTrimmedLeft(16.f), Justification::centredLeft);
        }
        else
        {
            g.fillAll(theme::bg);
            g.setFont(Fonts::mono(10.f, 0.08f));
            g.setColour(theme::faint);
            g.drawText(deviceText(), r.withTrimmedRight(16.f), Justification::centredRight);
        }
    }

    void resized() override
    {
        const int w = 118, h = 36;
        const int y = (getHeight() - h) / 2;
        settings.setBounds(isMuted() ? getWidth() - 16 - w + 4 : 16 - 4, y, w, h);
    }

private:
    bool isMuted() const { return (bool) muted.getValue(); }

    String deviceText() const
    {
        if (auto* d = holder.deviceManager.getCurrentAudioDevice())
            return d->getName() + String::fromUTF8(" \xc2\xb7 ") + String(d->getCurrentSampleRate() / 1000.0, 1) + " kHz"
                 + String::fromUTF8(" \xc2\xb7 ") + String(d->getCurrentBufferSizeSamples());
        return "No audio device";
    }

    void valueChanged(Value&) override          { settings.setOnAccent(isMuted()); resized(); repaint(); }
    void changeListenerCallback(ChangeBroadcaster*) override { repaint(); }

    void openSettings()
    {
        if (dialog == nullptr)
        {
            // non-modal, owned by the strip: no deferred deletion by the modal manager at quit
            dialog = std::make_unique<SettingsWindow>(holder);
            if (auto* top = getTopLevelComponent())
                dialog->centreAroundComponent(top, dialog->getWidth(), dialog->getHeight());
        }
        dialog->setVisible(true);    // the close button only hides it
        dialog->toFront(true);
    }

    StandalonePluginHolder& holder;
    Value muted;
    SettingsButton settings;
    std::unique_ptr<SettingsWindow> dialog;
};

// ---- Window content: strip + plugin editor ------------------------------------------------------
class MainContent : public Component
{
public:
    explicit MainContent(StandalonePluginHolder& h)
        : holder(h), strip(h), editor(h.processor->hasEditor() ? h.processor->createEditorIfNeeded() : nullptr)
    {
        addAndMakeVisible(strip);
        if (editor != nullptr) addAndMakeVisible(editor.get());
        const int w = editor != nullptr ? editor->getWidth()  : 620;
        const int hh = editor != nullptr ? editor->getHeight() : 400;
        setSize(w, hh + StripBar::heightPx);
    }

    ~MainContent() override
    {
        if (editor != nullptr)
        {
            holder.processor->editorBeingDeleted(editor.get());
            editor = nullptr;
        }
    }

    void resized() override
    {
        auto r = getLocalBounds();
        strip.setBounds(r.removeFromTop(StripBar::heightPx));
        if (editor != nullptr) editor->setTopLeftPosition(r.getTopLeft());
    }

    // the editor changes size when the user picks another UI size: follow it (the window follows us)
    void childBoundsChanged(Component* child) override
    {
        if (child == editor.get() && editor != nullptr)
        {
            const int w = editor->getWidth(), h = editor->getHeight() + StripBar::heightPx;
            if (w != getWidth() || h != getHeight()) setSize(w, h);
        }
    }

private:
    StandalonePluginHolder& holder;
    StripBar strip;
    std::unique_ptr<AudioProcessorEditor> editor;
};

class MainWindow : public DocumentWindow
{
public:
    explicit MainWindow(StandalonePluginHolder& h)
        : DocumentWindow(JucePlugin_Name, theme::bg, DocumentWindow::minimiseButton | DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainContent(h), true);
        setResizable(false, false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }
};

// ---- Application --------------------------------------------------------------------------------
class RogerThatApp : public JUCEApplication
{
public:
    RogerThatApp()
    {
        PropertiesFile::Options o;
        o.applicationName = JucePlugin_Name;
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        o.folderName = "~/.config";
       #else
        o.folderName = "";
       #endif
        appProperties.setStorageParameters(o);
    }

    const String getApplicationName() override       { return JucePlugin_Name; }
    const String getApplicationVersion() override    { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    void initialise(const String&) override
    {
        LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);

       #ifdef JucePlugin_PreferredChannelConfigurations
        constexpr StandalonePluginHolder::PluginInOuts channels[] { JucePlugin_PreferredChannelConfigurations };
        const Array<StandalonePluginHolder::PluginInOuts> channelConfig(channels, numElementsInArray(channels));
       #else
        const Array<StandalonePluginHolder::PluginInOuts> channelConfig;
       #endif

        holder = std::make_unique<StandalonePluginHolder>(appProperties.getUserSettings(), false, String{}, nullptr, channelConfig, false);
        window = std::make_unique<MainWindow>(*holder);
    }

    void shutdown() override
    {
        if (window != nullptr) window->setVisible(false);
        window = nullptr;          // destroys the editor (and the settings dialog) before the processor
        holder = nullptr;
        LookAndFeel::setDefaultLookAndFeel(nullptr);
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (holder != nullptr)
        {
            holder->savePluginState();
            holder->saveAudioDeviceState();
        }
        if (ModalComponentManager::getInstance()->cancelAllModalComponents())
            Timer::callAfterDelay(100, [] { if (auto* app = JUCEApplicationBase::getInstance()) app->systemRequestedQuit(); });
        else
            quit();
    }

private:
    LightLookAndFeel lookAndFeel;
    ApplicationProperties appProperties;
    std::unique_ptr<StandalonePluginHolder> holder;
    std::unique_ptr<MainWindow> window;
};

} // namespace rt

JUCE_CREATE_APPLICATION_DEFINE(rt::RogerThatApp)
