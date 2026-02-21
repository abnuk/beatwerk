#include "SampleBrowserComponent.h"

//==============================================================================
// SampleFileItemComponent - handles native drag for audio file tree items
//==============================================================================

class SampleFileItemComponent : public juce::Component
{
public:
    SampleFileItemComponent (SampleTreeItem& item, SampleBrowserComponent& browser)
        : treeItem (item), owner (browser)
    {
        setInterceptsMouseClicks (true, false);
    }

    void paint (juce::Graphics& g) override
    {
        if (treeItem.isSelected())
        {
            g.setColour (DarkLookAndFeel::accent.withAlpha (0.25f));
            g.fillAll();
        }

        g.setColour (DarkLookAndFeel::textBright);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (treeItem.getDisplayName(),
                    juce::Rectangle<int> (4, 0, getWidth() - 8, getHeight()),
                    juce::Justification::centredLeft, true);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        treeItem.setSelected (true, true);
        nativeDragStarted = false;

        if (e.mods.isPopupMenu())
        {
            popupTriggered = true;
            owner.showItemContextMenu (treeItem.getFile(), e.getScreenPosition());
            return;
        }

        popupTriggered = false;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (nativeDragStarted || popupTriggered)
            return;

        if (e.getDistanceFromDragStart() < 5)
            return;

        nativeDragStarted = true;

        juce::StringArray files;
        files.add (treeItem.getFile().getFullPathName());

        juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, this);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! nativeDragStarted && ! popupTriggered)
            owner.getSampleEngine().previewSample (treeItem.getFile());

        nativeDragStarted = false;
        popupTriggered = false;
    }

private:
    SampleTreeItem& treeItem;
    SampleBrowserComponent& owner;
    bool nativeDragStarted = false;
    bool popupTriggered = false;
};

//==============================================================================
// SampleTreeItem
//==============================================================================

SampleTreeItem::SampleTreeItem (const juce::File& f, SampleBrowserComponent& browser,
                                const juce::String& customDisplayName)
    : file (f), owner (browser), displayName (customDisplayName)
{
}

bool SampleTreeItem::mightContainSubItems()
{
    return file.isDirectory();
}

juce::String SampleTreeItem::getUniqueName() const
{
    if (displayName.isNotEmpty())
        return file.getFullPathName() + "|" + displayName;

    return file.getFullPathName();
}

juce::String SampleTreeItem::getDisplayName() const
{
    if (displayName.isNotEmpty())
        return displayName;

    return file.getFileName();
}

void SampleTreeItem::paintItem (juce::Graphics& g, int width, int height)
{
    bool isDropTarget = file.isDirectory()
                        && owner.getHighlightedDropTarget() == file;

    if (isDropTarget)
    {
        g.setColour (DarkLookAndFeel::accent.withAlpha (0.2f));
        g.fillRect (0, 0, width, height);
        g.setColour (DarkLookAndFeel::accent);
        g.drawRect (0, 0, width, height, 1);
    }
    else if (isSelected())
    {
        g.setColour (DarkLookAndFeel::accent.withAlpha (0.25f));
        g.fillRect (0, 0, width, height);
    }

    auto textArea = juce::Rectangle<int> (4, 0, width - 8, height);

    if (file.isDirectory())
    {
        g.setColour (isDropTarget ? DarkLookAndFeel::accent.brighter (0.3f)
                                  : DarkLookAndFeel::accent);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    }
    else
    {
        g.setColour (DarkLookAndFeel::textBright);
        g.setFont (juce::FontOptions (12.0f));
    }

    g.drawText (getDisplayName(), textArea, juce::Justification::centredLeft, true);
}

void SampleTreeItem::itemOpennessChanged (bool isNowOpen)
{
    if (isNowOpen && ! hasScanned && file.isDirectory())
        scanDirectory();
}

void SampleTreeItem::itemClicked (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        owner.showItemContextMenu (file, e.getScreenPosition());
        return;
    }

    if (! file.isDirectory() && isAudioFile (file))
        owner.getSampleEngine().previewSample (file);
}

juce::var SampleTreeItem::getDragSourceDescription()
{
    if (! file.isDirectory() && isAudioFile (file))
        return SampleBrowserComponent::dragSourceId + ":" + file.getFullPathName();

    return {};
}

std::unique_ptr<juce::Component> SampleTreeItem::createItemComponent()
{
    if (! file.isDirectory() && isAudioFile (file))
        return std::make_unique<SampleFileItemComponent> (*this, owner);

    return nullptr;
}

bool SampleTreeItem::isAudioFile (const juce::File& f)
{
    auto ext = f.getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3";
}

void SampleTreeItem::scanDirectory()
{
    hasScanned = true;
    clearSubItems();

    auto children = file.findChildFiles (
        juce::File::findFilesAndDirectories | juce::File::ignoreHiddenFiles, false);

    children.sort();

    juce::Array<juce::File> dirs, audioFiles;
    for (auto& child : children)
    {
        if (child.isDirectory())
            dirs.add (child);
        else if (isAudioFile (child))
            audioFiles.add (child);
    }

    for (auto& d : dirs)
        addSubItem (new SampleTreeItem (d, owner));

    for (auto& f : audioFiles)
        addSubItem (new SampleTreeItem (f, owner));
}

//==============================================================================
// SampleBrowserComponent
//==============================================================================

const juce::String SampleBrowserComponent::dragSourceId = "MPSSampleDrag";

SampleBrowserComponent::SampleBrowserComponent (SampleEngine& engine)
    : sampleEngine (engine)
{
    treeView.setColour (juce::TreeView::backgroundColourId, DarkLookAndFeel::bgDark);
    treeView.setColour (juce::TreeView::linesColourId, DarkLookAndFeel::bgLight);
    treeView.setColour (juce::TreeView::selectedItemBackgroundColourId,
                         DarkLookAndFeel::accent.withAlpha (0.15f));
    treeView.setColour (juce::TreeView::dragAndDropIndicatorColourId, DarkLookAndFeel::accent);
    treeView.setMultiSelectEnabled (false);
    addAndMakeVisible (treeView);

    searchField.setColour (juce::TextEditor::backgroundColourId, DarkLookAndFeel::bgDark);
    searchField.setColour (juce::TextEditor::textColourId, DarkLookAndFeel::textBright);
    searchField.setColour (juce::TextEditor::outlineColourId, DarkLookAndFeel::bgLight);
    searchField.setColour (juce::TextEditor::focusedOutlineColourId, DarkLookAndFeel::accent);
    searchField.setTextToShowWhenEmpty ("Search...", DarkLookAndFeel::textDim);
    searchField.setFont (juce::FontOptions (12.0f));
    searchField.onTextChange = [this] { performSearch(); };
    searchField.onEscapeKey = [this]
    {
        searchField.clear();
        performSearch();
    };
    addAndMakeVisible (searchField);

    clearSearchButton.onClick = [this]
    {
        searchField.clear();
        performSearch();
    };
    clearSearchButton.setVisible (false);
    addAndMakeVisible (clearSearchButton);

    refreshButton.onClick = [this] { refresh(); };
    addAndMakeVisible (refreshButton);
}

SampleBrowserComponent::~SampleBrowserComponent()
{
    treeView.setRootItem (nullptr);
}

void SampleBrowserComponent::resized()
{
    auto area = getLocalBounds();

    auto headerArea = area.removeFromTop (30);
    headerArea.reduce (4, 2);
    refreshButton.setBounds (headerArea.removeFromRight (70));

    auto searchArea = area.removeFromTop (26);
    searchArea.reduce (4, 2);
    if (clearSearchButton.isVisible())
        clearSearchButton.setBounds (searchArea.removeFromRight (22));
    searchField.setBounds (searchArea);

    area.removeFromTop (2);
    treeView.setBounds (area);
}

void SampleBrowserComponent::paint (juce::Graphics& g)
{
    g.fillAll (DarkLookAndFeel::bgDark);

    g.setColour (DarkLookAndFeel::bgMedium);
    g.fillRect (0, 0, getWidth(), 30);

    g.setColour (DarkLookAndFeel::textBright);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("Samples", 8, 0, getWidth() - 80, 30, juce::Justification::centredLeft);

    g.setColour (DarkLookAndFeel::accent.withAlpha (0.5f));
    g.fillRect (getWidth() - 2, 0, 2, getHeight());

    if (fileDragActive)
    {
        g.setColour (DarkLookAndFeel::accent.withAlpha (0.08f));
        g.fillRect (getLocalBounds());
        g.setColour (DarkLookAndFeel::accent);
        g.drawRect (getLocalBounds(), 2);

        if (highlightedDropTarget.isDirectory())
        {
            auto targetName = highlightedDropTarget == samplesDir
                                  ? juce::String ("Samples (root)")
                                  : highlightedDropTarget.getRelativePathFrom (samplesDir);

            g.setColour (DarkLookAndFeel::accent);
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText ("Drop into: " + targetName,
                        4, getHeight() - 20, getWidth() - 8, 18,
                        juce::Justification::centredLeft);
        }
    }
}

void SampleBrowserComponent::setSamplesDirectory (const juce::File& dir)
{
    samplesDir = dir;
    refresh();
}

void SampleBrowserComponent::refresh()
{
    treeView.setRootItem (nullptr);
    rootItem.reset();

    if (samplesDir.isDirectory())
    {
        rootItem = std::make_unique<SampleTreeItem> (samplesDir, *this);
        treeView.setRootItem (rootItem.get());
        rootItem->setOpen (true);
        treeView.setRootItemVisible (false);
    }
}

void SampleBrowserComponent::revealFile (const juce::File& file)
{
    if (rootItem == nullptr || ! file.existsAsFile())
        return;

    if (searchField.getText().trim().isNotEmpty())
    {
        searchField.clear();
        refresh();
    }

    auto relativePath = file.getRelativePathFrom (samplesDir);
    juce::StringArray pathParts;
    pathParts.addTokens (relativePath, juce::File::getSeparatorString(), "");

    juce::TreeViewItem* current = rootItem.get();

    for (int i = 0; i < pathParts.size(); ++i)
    {
        current->setOpen (true);

        bool found = false;
        for (int j = 0; j < current->getNumSubItems(); ++j)
        {
            auto* child = current->getSubItem (j);
            auto* treeItem = dynamic_cast<SampleTreeItem*> (child);
            if (treeItem != nullptr && treeItem->getFile().getFileName() == pathParts[i])
            {
                current = child;
                found = true;
                break;
            }
        }

        if (! found)
            return;
    }

    current->setSelected (true, true);

    auto safeThis = juce::Component::SafePointer<SampleBrowserComponent> (this);
    juce::MessageManager::callAsync ([safeThis]
    {
        if (safeThis != nullptr)
            if (auto* selected = safeThis->treeView.getSelectedItem (0))
                safeThis->treeView.scrollToKeepItemVisible (selected);
    });
}

bool SampleBrowserComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
    {
        juce::File file (f);
        if (file.isDirectory() || SampleTreeItem::isAudioFile (file))
            return true;
    }
    return false;
}

void SampleBrowserComponent::fileDragEnter (const juce::StringArray&, int x, int y)
{
    fileDragActive = true;
    updateDropTargetHighlight (x, y);
    repaint();
}

void SampleBrowserComponent::fileDragMove (const juce::StringArray&, int x, int y)
{
    updateDropTargetHighlight (x, y);
}

void SampleBrowserComponent::fileDragExit (const juce::StringArray&)
{
    fileDragActive = false;
    highlightedDropTarget = juce::File();
    treeView.repaint();
    repaint();
}

void SampleBrowserComponent::filesDropped (const juce::StringArray& files, int x, int y)
{
    fileDragActive = false;
    highlightedDropTarget = juce::File();
    treeView.repaint();
    repaint();

    auto targetDir = getDropTargetDirectory (x, y);

    juce::StringArray audioFiles;
    juce::StringArray dirPaths;
    juce::StringArray conflictNames;

    for (auto& filePath : files)
    {
        juce::File sourceFile (filePath);
        auto destFile = targetDir.getChildFile (sourceFile.getFileName());

        if (sourceFile == destFile)
            continue;

        if (sourceFile.isDirectory())
        {
            dirPaths.add (filePath);
            if (destFile.isDirectory())
                conflictNames.add (sourceFile.getFileName() + " (folder)");
        }
        else if (SampleTreeItem::isAudioFile (sourceFile))
        {
            audioFiles.add (filePath);
            if (destFile.existsAsFile())
                conflictNames.add (sourceFile.getFileName());
        }
    }

    if (audioFiles.isEmpty() && dirPaths.isEmpty())
        return;

    auto safeThis = juce::Component::SafePointer<SampleBrowserComponent> (this);

    auto doCopy = [safeThis, audioFiles, dirPaths, targetDir]()
    {
        for (auto& path : dirPaths)
        {
            juce::File src (path);
            src.copyDirectoryTo (targetDir.getChildFile (src.getFileName()));
        }

        juce::File lastCopiedFile;

        for (auto& path : audioFiles)
        {
            juce::File src (path);
            auto dest = targetDir.getChildFile (src.getFileName());
            src.copyFileTo (dest);
            lastCopiedFile = dest;
        }

        if (safeThis != nullptr)
        {
            safeThis->refresh();

            if (lastCopiedFile.existsAsFile())
                safeThis->revealFile (lastCopiedFile);
        }
    };

    if (conflictNames.isEmpty())
    {
        doCopy();
    }
    else
    {
        auto msg = "The following items already exist:\n\n"
                 + conflictNames.joinIntoString ("\n")
                 + "\n\nOverwrite?";

        auto* alert = new juce::AlertWindow ("Files Already Exist", msg,
                                              juce::MessageBoxIconType::WarningIcon);
        alert->addButton ("Overwrite", 1);
        alert->addButton ("Cancel", 0);

        alert->enterModalState (true, juce::ModalCallbackFunction::create (
            [doCopy, alert] (int result)
            {
                if (result == 1)
                    doCopy();
                delete alert;
            }), false);
    }
}

juce::File SampleBrowserComponent::getDropTargetDirectory (int x, int y) const
{
    auto localPoint = treeView.getLocalPoint (this, juce::Point<int> (x, y));

    if (auto* item = treeView.getItemAt (localPoint.y))
    {
        if (auto* treeItem = dynamic_cast<const SampleTreeItem*> (item))
        {
            if (treeItem->getFile().isDirectory())
                return treeItem->getFile();
            return treeItem->getFile().getParentDirectory();
        }
    }

    return samplesDir;
}

void SampleBrowserComponent::updateDropTargetHighlight (int x, int y)
{
    auto newTarget = getDropTargetDirectory (x, y);

    if (newTarget != highlightedDropTarget)
    {
        highlightedDropTarget = newTarget;
        treeView.repaint();
    }
}

//==============================================================================
// Search
//==============================================================================

void SampleBrowserComponent::performSearch()
{
    auto text = searchField.getText().trim();
    bool hasSearch = text.isNotEmpty();

    clearSearchButton.setVisible (hasSearch);
    resized();

    if (! hasSearch)
    {
        refresh();
        return;
    }

    treeView.setRootItem (nullptr);
    rootItem.reset();

    if (! samplesDir.isDirectory())
        return;

    rootItem = std::make_unique<SampleTreeItem> (samplesDir, *this);
    rootItem->markAsScanned();
    treeView.setRootItem (rootItem.get());
    treeView.setRootItemVisible (false);
    rootItem->setOpen (true);

    auto allFiles = samplesDir.findChildFiles (juce::File::findFiles, true);
    auto lowerText = text.toLowerCase();

    for (auto& f : allFiles)
    {
        if (! SampleTreeItem::isAudioFile (f))
            continue;

        if (f.getFileNameWithoutExtension().toLowerCase().contains (lowerText))
            rootItem->addSubItem (new SampleTreeItem (f, *this));
    }
}

void SampleBrowserComponent::refreshAfterChange()
{
    if (searchField.getText().trim().isNotEmpty())
        performSearch();
    else
        refresh();
}

//==============================================================================
// Context menu, delete, move
//==============================================================================

void SampleBrowserComponent::showItemContextMenu (const juce::File& file,
                                                    juce::Point<int> screenPos)
{
    juce::PopupMenu menu;

    const int deleteId = 1;
    const int revealId = 2;
    const int moveIdBase = 100;

    menu.addItem (deleteId, "Delete");

    auto targetFolders = collectTargetFolders (file);

    if (! targetFolders.isEmpty())
    {
        juce::PopupMenu moveMenu;

        for (int i = 0; i < targetFolders.size(); ++i)
        {
            auto label = targetFolders[i] == samplesDir
                             ? juce::String ("Samples (root)")
                             : targetFolders[i].getRelativePathFrom (samplesDir);
            moveMenu.addItem (moveIdBase + i, label);
        }

        menu.addSubMenu ("Move to", moveMenu);
    }

    menu.addSeparator();
    menu.addItem (revealId, "Reveal in Finder");

    auto safeThis = juce::Component::SafePointer<SampleBrowserComponent> (this);

    menu.showMenuAsync (
        juce::PopupMenu::Options().withTargetScreenArea (
            juce::Rectangle<int> (screenPos.x, screenPos.y, 1, 1)),
        [safeThis, file, targetFolders] (int result)
        {
            if (safeThis == nullptr || result == 0)
                return;

            if (result == 1)
                safeThis->deleteItem (file);
            else if (result == 2)
                file.revealToUser();
            else if (result >= 100 && result < 100 + targetFolders.size())
                safeThis->moveItem (file, targetFolders[result - 100]);
        });
}

void SampleBrowserComponent::deleteItem (const juce::File& file)
{
    auto name = file.getFileName();
    bool isDir = file.isDirectory();

    auto msg = isDir
                   ? "Delete the folder \"" + name + "\" and all its contents?"
                   : "Delete \"" + name + "\"?";

    auto* alert = new juce::AlertWindow ("Confirm Delete", msg,
                                          juce::MessageBoxIconType::WarningIcon);
    alert->addButton ("Delete", 1);
    alert->addButton ("Cancel", 0);

    auto safeThis = juce::Component::SafePointer<SampleBrowserComponent> (this);

    alert->enterModalState (true, juce::ModalCallbackFunction::create (
        [safeThis, file, isDir, alert] (int result)
        {
            if (result == 1)
            {
                if (isDir)
                    file.deleteRecursively();
                else
                    file.deleteFile();

                if (safeThis != nullptr)
                    safeThis->refreshAfterChange();
            }

            delete alert;
        }), false);
}

void SampleBrowserComponent::moveItem (const juce::File& source, const juce::File& targetDir)
{
    auto dest = targetDir.getChildFile (source.getFileName());

    if (dest.exists())
    {
        auto* alert = new juce::AlertWindow (
            "Already Exists",
            "\"" + source.getFileName() + "\" already exists in the target folder. Overwrite?",
            juce::MessageBoxIconType::WarningIcon);
        alert->addButton ("Overwrite", 1);
        alert->addButton ("Cancel", 0);

        auto safeThis = juce::Component::SafePointer<SampleBrowserComponent> (this);

        alert->enterModalState (true, juce::ModalCallbackFunction::create (
            [safeThis, source, dest, alert] (int result)
            {
                if (result == 1 && safeThis != nullptr)
                {
                    if (dest.isDirectory())
                        dest.deleteRecursively();
                    else
                        dest.deleteFile();

                    source.moveFileTo (dest);
                    safeThis->refreshAfterChange();
                }

                delete alert;
            }), false);
        return;
    }

    source.moveFileTo (dest);
    refreshAfterChange();
}

juce::Array<juce::File> SampleBrowserComponent::collectTargetFolders (
    const juce::File& excludeItem) const
{
    juce::Array<juce::File> result;

    auto currentDir = excludeItem.isDirectory()
                          ? excludeItem
                          : excludeItem.getParentDirectory();

    if (samplesDir != currentDir)
        result.add (samplesDir);

    auto dirs = samplesDir.findChildFiles (
        juce::File::findDirectories | juce::File::ignoreHiddenFiles, true);
    dirs.sort();

    for (auto& d : dirs)
    {
        if (d == currentDir)
            continue;

        if (excludeItem.isDirectory() && d.isAChildOf (excludeItem))
            continue;

        result.add (d);
    }

    return result;
}
