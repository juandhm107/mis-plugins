#include "PresetPage.h"
#include "../presets/Presets.h"

namespace ztx
{
using namespace theme;

class PresetPage::CategoryList final : public juce::Component
{
public:
    explicit CategoryList (PresetPage& o) : owner (o) {}
    juce::StringArray names;
    int selected = 0;
    void paint (juce::Graphics& g) override
    {
        const int h = 30;
        for (int i = 0; i < names.size(); ++i)
        {
            auto r = juce::Rectangle<int> (0, i * h, getWidth(), h).reduced (2);
            if (i == selected) { g.setColour (acid.withAlpha (0.22f)); g.fillRoundedRectangle (r.toFloat(), 6); g.setColour (acid); g.fillRect (r.removeFromLeft (3)); }
            g.setColour (i == selected ? text : silver);
            g.setFont (label (12.0f));
            g.drawText (names[i].toUpperCase(), r.reduced (10, 0), juce::Justification::centredLeft);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = e.y / 30;
        if (i >= 0 && i < names.size()) { selected = i; owner.selectCategory (i); repaint(); }
    }
private:
    PresetPage& owner;
};

PresetPage::PresetPage (ZtxProcessor& p) : Page (p)
{
    categories = std::make_unique<CategoryList> (*this);
    categories->names.add ("All");
    categories->names.addArray (presetCategories());
    categories->names.add ("User");
    addAndMakeVisible (*categories);
    list.setRowHeight (30);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (list);
    nameEd.setTextToShowWhenEmpty ("preset name...", textDim);
    nameEd.setFont (mono (13.0f));
    addAndMakeVisible (nameEd);
    for (auto* b : { &saveBtn, &deleteBtn, &folderBtn, &initBtn }) addAndMakeVisible (*b);
    saveBtn.getProperties().set ("accent", true);
    saveBtn.onClick = [this]
    {
        auto nm = nameEd.getText().trim();
        if (nm.isEmpty()) nm = proc.currentPresetName + " var";
        if (saveUserPreset (proc, nm)) { refresh(); if (onPresetLoaded) onPresetLoaded(); }
    };
    deleteBtn.onClick = [this]
    {
        const int r = list.getSelectedRow();
        if (r >= 0 && r < (int) shown.size() && shown[(size_t) r].factory < 0) { shown[(size_t) r].file.deleteFile(); refresh(); }
    };
    deleteBtn.setTooltip ("Delete the selected USER preset (factory presets are read-only)");
    folderBtn.onClick = [] { userPresetFolder().revealToUser(); };
    initBtn.onClick = [this] { initPatch(); };
    initBtn.setTooltip ("Reset everything to a neutral starting point");
    refresh();
}

PresetPage::~PresetPage() = default;

void PresetPage::initPatch()
{
    proc.resetAllParams();
    for (auto& v : proc.variations) v = Pattern();
    proc.frozen = false;
    proc.selectVariation (0);
    proc.currentPresetName = "Init";
    proc.presetIndex = -1;
    if (onPresetLoaded) onPresetLoaded();
}

void PresetPage::refresh()
{
    all.clear();
    const auto& lib = factoryPresets();
    for (int i = 0; i < (int) lib.size(); ++i)
        all.push_back ({ lib[(size_t) i].name, lib[(size_t) i].category, lib[(size_t) i].description, i, {} });
    for (auto& f : userPresets())
        all.push_back ({ f.getFileNameWithoutExtension(), "User", "User preset - " + f.getFullPathName(), -1, f });
    selectCategory (category);
}

void PresetPage::selectCategory (int c)
{
    category = c;
    shown.clear();
    const auto cats = presetCategories();
    for (auto& e : all)
        if (c == 0 || (c == 9 && e.factory < 0) || (c >= 1 && c <= 8 && e.category == cats[c - 1]))
            shown.push_back (e);
    list.updateContent();
    list.repaint();
}

int PresetPage::getNumRows() { return (int) shown.size(); }

void PresetPage::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= (int) shown.size()) return;
    const auto& e = shown[(size_t) row];
    const bool current = e.name == proc.currentPresetName;
    auto r = juce::Rectangle<int> (0, 0, w, h).reduced (2, 1);
    if (selected || current) { g.setColour ((current ? acid : silver).withAlpha (0.16f)); g.fillRoundedRectangle (r.toFloat(), 5); }
    g.setColour (current ? acid : text);
    g.setFont (mono (13.0f, current));
    g.drawText (e.name, r.reduced (10, 0), juce::Justification::centredLeft);
    g.setColour (textDim);
    g.setFont (mono (10.0f));
    g.drawText (e.category, r.reduced (10, 0), juce::Justification::centredRight);
}

void PresetPage::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= (int) shown.size()) return;
    const auto e = shown[(size_t) row];
    if (e.factory >= 0) applyFactoryPreset (proc, e.factory);
    else loadUserPreset (proc, e.file);
    selectedDesc = e.desc;
    nameEd.setText (e.name, false);
    list.repaint();
    repaint();
    if (onPresetLoaded) onPresetLoaded();
}

void PresetPage::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    catPod = r.removeFromLeft (250);
    r.removeFromLeft (10);
    infoPod = r.removeFromRight (330);
    r.removeFromRight (10);
    listPod = r;
    categories->setBounds (catPod.reduced (10, 12).withTrimmedTop (24));
    list.setBounds (listPod.reduced (10, 12).withTrimmedTop (24));
    auto a = infoPod.reduced (16, 12);
    a.removeFromTop (190);
    nameEd.setBounds (a.removeFromTop (30)); a.removeFromTop (8);
    saveBtn.setBounds (a.removeFromTop (32)); a.removeFromTop (8);
    auto row = a.removeFromTop (30);
    deleteBtn.setBounds (row.removeFromLeft (row.getWidth() / 2).reduced (0, 0).withTrimmedRight (4));
    folderBtn.setBounds (row.withTrimmedLeft (4));
    a.removeFromTop (20);
    initBtn.setBounds (a.removeFromTop (30));
}

void PresetPage::paint (juce::Graphics& g)
{
    drawPod (g, catPod.toFloat(), 81);
    drawPod (g, listPod.toFloat(), 82, juce::Colour (0xff0f110d));
    drawPod (g, infoPod.toFloat(), 83);
    g.setColour (silver); g.setFont (label (11.0f));
    g.drawText ("SOUND LIBRARY", catPod.reduced (14, 10).removeFromTop (14), juce::Justification::left);
    g.drawText (juce::String ((int) shown.size()) + " PRESETS", listPod.reduced (14, 10).removeFromTop (14), juce::Justification::left);
    auto a = infoPod.reduced (16, 12);
    g.drawText ("CURRENT", a.removeFromTop (16), juce::Justification::left);
    drawGlitchText (g, proc.currentPresetName.toUpperCase(), a.removeFromTop (40).toFloat(), mono (20.0f, true), 31);
    g.setColour (textDim); g.setFont (mono (12.0f));
    const auto& lib = factoryPresets();
    const juce::String desc = proc.presetIndex >= 0 && proc.presetIndex < (int) lib.size()
                                  ? juce::String (lib[(size_t) proc.presetIndex].category) + "\n" + lib[(size_t) proc.presetIndex].description
                                  : selectedDesc;
    g.drawFittedText (desc, a.removeFromTop (70), juce::Justification::topLeft, 5);
    g.setColour (silverDim); g.setFont (mono (10.0f));
    g.drawFittedText ("Click a preset to load it. Presets store engines, FX, modulation and all four pattern variations.",
                      a.removeFromTop (50), juce::Justification::topLeft, 3);
}
} // namespace ztx
