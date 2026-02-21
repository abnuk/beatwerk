#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "SampleEngine.h"
#include "LookAndFeel.h"

class SampleBrowserComponent;

class SampleTreeItem : public juce::TreeViewItem
{
public:
    SampleTreeItem (const juce::File& f, SampleBrowserComponent& browser);

    bool mightContainSubItems() override;
    juce::String getUniqueName() const override;
    void paintItem (juce::Graphics& g, int width, int height) override;
    void itemOpennessChanged (bool isNowOpen) override;
    void itemClicked (const juce::MouseEvent&) override;
    juce::var getDragSourceDescription() override;
    std::unique_ptr<juce::Component> createItemComponent() override;
    int getItemHeight() const override { return 22; }
    bool canBeSelected() const override { return true; }

    const juce::File& getFile() const { return file; }
    static bool isAudioFile (const juce::File& f);

private:
    juce::File file;
    SampleBrowserComponent& owner;
    bool hasScanned = false;

    void scanDirectory();
};

class SampleTreeView : public juce::TreeView
{
public:
    bool isInterestedInFileDrag (const juce::StringArray&) override { return false; }
};

class SampleBrowserComponent : public juce::Component,
                                public juce::FileDragAndDropTarget
{
public:
    SampleBrowserComponent (SampleEngine& engine);
    ~SampleBrowserComponent() override;

    void resized() override;
    void paint (juce::Graphics& g) override;

    void setSamplesDirectory (const juce::File& dir);
    void refresh();
    void revealFile (const juce::File& file);

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragMove (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    SampleEngine& getSampleEngine() { return sampleEngine; }
    juce::File getSamplesDirectory() const { return samplesDir; }
    juce::File getHighlightedDropTarget() const { return highlightedDropTarget; }

    static const juce::String dragSourceId;

private:
    SampleEngine& sampleEngine;
    juce::File samplesDir;

    SampleTreeView treeView;
    std::unique_ptr<SampleTreeItem> rootItem;

    juce::TextButton refreshButton { "Refresh" };

    bool fileDragActive = false;
    juce::File highlightedDropTarget;

    juce::File getDropTargetDirectory (int x, int y) const;
    void updateDropTargetHighlight (int x, int y);
};
