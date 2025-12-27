#pragma once
#include <JuceHeader.h>

class AuricOmega76AudioProcessor;

//==============================================================================
// Preset Management Helper Class
class PresetManager
{
public:
    explicit PresetManager (AuricOmega76AudioProcessor& processor);
    
    // Directory management
    juce::File getPresetDirectory() const;
    void ensurePresetDirectory();
    
    // Preset operations
    void rebuildPresetMenu (juce::ComboBox& presetBox, std::vector<juce::File>& presetFiles);
    void loadFactoryDefault();
    void loadPresetFile (const juce::File& file);
    void updatePresetButtonsEnabled (juce::ComboBox& presetBox, juce::TextButton& deleteButton);
    void deleteSelectedPreset (juce::ComboBox& presetBox, std::vector<juce::File>& presetFiles,
                              juce::Component* parentComponent);

private:
    AuricOmega76AudioProcessor& audioProcessor;
};