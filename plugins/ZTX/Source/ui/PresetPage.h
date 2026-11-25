#pragma once
#include "Widgets.h"

namespace ztx
{
class PresetPage final : public Page, private juce::ListBoxModel
{
public:
    explicit PresetPage (ZtxProcessor&);
    ~PresetPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick() override {}
    void refresh();
    std::function<void()> onPresetLoaded;

private:
    struct Entry { juce::String name, category, desc; int factory = -1; juce::File file; };
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void selectCategory (int c);
    void initPatch();

    class CategoryList;
    std::unique_ptr<CategoryList> categories;
    juce::ListBox list { "presets", this };
    std::vector<Entry> all, shown;
    int category = 0;    // 0 = All, 1..8 factory categories, 9 = User
    juce::TextEditor nameEd;
    juce::TextButton saveBtn { "SAVE USER PRESET" }, deleteBtn { "DELETE" }, folderBtn { "OPEN FOLDER" }, initBtn { "INIT PATCH" };
    juce::Rectangle<int> catPod, listPod, infoPod;
    juce::String selectedDesc;
};
} // namespace ztx
