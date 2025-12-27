#include "PresetManager.h"
#include "PluginProcessor.h"

//==============================================================================
PresetManager::PresetManager (AuricOmega76AudioProcessor& processor)
    : audioProcessor (processor)
{
}

juce::File PresetManager::getPresetDirectory() const
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    return base.getChildFile ("AuricOmega76").getChildFile ("Presets");
}

void PresetManager::ensurePresetDirectory()
{
    auto dir = getPresetDirectory();
    if (! dir.exists())
        dir.createDirectory();
}

void PresetManager::rebuildPresetMenu (juce::ComboBox& presetBox, std::vector<juce::File>& presetFiles)
{
    presetBox.clear (juce::dontSendNotification);
    presetFiles.clear();

    presetBox.addItem ("DEFAULT", 1);

    auto dir = getPresetDirectory();
    juce::Array<juce::File> files;
    dir.findChildFiles (files, juce::File::findFiles, false, "*.xml");
    files.sort();

    int itemId = 2;
    for (auto& f : files)
    {
        presetFiles.push_back (f);
        presetBox.addItem (f.getFileNameWithoutExtension(), itemId++);
    }

    if (presetBox.getSelectedId() == 0)
        presetBox.setSelectedId (1, juce::dontSendNotification);
}

void PresetManager::loadFactoryDefault()
{
    for (auto* p : audioProcessor.getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            rp->beginChangeGesture();
            rp->setValueNotifyingHost (rp->getDefaultValue());
            rp->endChangeGesture();
        }
    }
}

void PresetManager::loadPresetFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    juce::XmlDocument doc (file);
    std::unique_ptr<juce::XmlElement> xml (doc.getDocumentElement());
    if (xml == nullptr) return;

    auto vt = juce::ValueTree::fromXml (*xml);
    if (vt.isValid())
        audioProcessor.apvts.replaceState (vt);
}

void PresetManager::updatePresetButtonsEnabled (juce::ComboBox& presetBox, juce::TextButton& deleteButton)
{
    const int id = presetBox.getSelectedId();
    deleteButton.setEnabled (id > 1);
}

void PresetManager::deleteSelectedPreset (juce::ComboBox& presetBox, std::vector<juce::File>& presetFiles,
                                         juce::Component* parentComponent)
{
    const int id = presetBox.getSelectedId();
    if (id <= 1) return;

    const int idx = id - 2;
    if (idx < 0 || idx >= (int) presetFiles.size()) return;

    auto fileToDelete = presetFiles[(size_t) idx];

    juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon,
                                       "Delete Preset",
                                       "Delete preset \"" + fileToDelete.getFileNameWithoutExtension()
                                       + "\"?\nThis cannot be undone.",
                                       "Delete",
                                       "Cancel",
                                       parentComponent,
                                       juce::ModalCallbackFunction::create (
                                           [this, &presetBox, &presetFiles, fileToDelete] (int result)
                                           {
                                               if (result == 0) return;
                                               if (fileToDelete.existsAsFile())
                                                   fileToDelete.deleteFile();

                                               rebuildPresetMenu (presetBox, presetFiles);
                                               presetBox.setSelectedId (1, juce::sendNotification);
                                               // Note: updatePresetButtonsEnabled would need button reference
                                           }));
}