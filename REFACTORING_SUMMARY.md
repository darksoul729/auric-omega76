# AuricOmega76 - Struktur File & Fungsi

## Struktur Folder

```
Source/
├── PluginProcessor.h/cpp     - Audio processing core
├── PluginEditor.h/cpp        - Main UI editor
├── AuricHelpers.h/cpp        - Helper functions (font, styling)
├── AuricKnob.h               - Custom rotary knob component
├── AuricLookAndFeel.h/cpp    - Custom Look and Feel
├── GainReductionMeter.h/cpp  - Analog-style GR meter
├── SegmentedSwitch.h/cpp     - 3-way segmented switch
└── PresetManager.h/cpp       - Preset load/save/delete
```

---

## Fungsi Setiap File

### PluginProcessor.h/cpp
- Audio DSP processing utama
- Parameter layout (APVTS)
- Sidechain HPF filter
- Envelope & gain smoothing
- State save/load

### PluginEditor.h/cpp
- Main UI window
- Layout semua komponen
- Timer untuk meter update
- LED indicators logic
- UI scale (S/M/L)
- Theme colors (`AuricTheme`)
- Font helpers (`AuricFonts`)
- `LedComponent` class

### AuricHelpers.h/cpp
- `makeFont()` - Font compatible JUCE lama/baru
- `styleLabelGold()` - Label styling gold
- `trackCaps()` - Letter spacing untuk brand text
- `styleKnobAuric()` - Knob styling

### AuricKnob.h
- Custom `Slider` rotary
- Shift + drag = fine adjust
- Popup value saat drag
- Double-click reset
- `setupAuricKnob()` helper function

### AuricLookAndFeel.h/cpp
- Custom rendering untuk:
  - Rotary sliders (knobs)
  - Toggle buttons
  - Text buttons
  - ComboBox
  - Labels
  - Popup bubble
- UI scale support

### GainReductionMeter.h/cpp
- Analog-style VU meter
- Ballistics (attack/release smoothing)
- Needle animation
- Arc scale rendering

### SegmentedSwitch.h/cpp
- `SegmentedSwitch3` - 3-pilihan switch (A/B/C)
- Optional left legend
- Optional sub-labels
- Mouse interaction (hover, click, drag)
- `SegmentedSwitchAttachment` - Binding ke `AudioParameterChoice`

### PresetManager.h/cpp
- `getPresetDirectory()` - Lokasi preset folder
- `rebuildPresetMenu()` - Populate ComboBox
- `loadFactoryDefault()` - Load default preset
- `loadPresetFile()` - Load dari file
- `deleteSelectedPreset()` - Hapus preset

---

## Komponen UI

| Komponen | Class | Fungsi |
|----------|-------|--------|
| Input Knob | `AuricKnob` | Input gain |
| Release Knob | `AuricKnob` | Release time |
| Edge Knob | `AuricKnob` | Edge/character |
| Mode Knob | `AuricKnob` | Compression mode |
| Mix Knob | `AuricKnob` | Dry/wet mix |
| Omega Mix Knob | `AuricKnob` | Omega blend |
| GR Meter | `GainReductionMeterComponent` | Gain reduction display |
| Omega Mode | `SegmentedSwitch3` | A/C/Ω mode |
| Routing | `SegmentedSwitch3` | A/D/Ω routing |
| UI Scale | `SegmentedSwitch3` | S/M/L size |
| SC HPF | `ToggleButton` | Sidechain HPF on/off |
| PWR | `ToggleButton` | Power on/off |
| LED 1 & 2 | `LedComponent` | Status indicators |
| Preset Box | `ComboBox` | Preset selection |

---

## Status

✅ Kompilasi bersih (no warnings/errors)  
✅ Semua komponen modular  
✅ Siap untuk build
