# UI Integration Guide for D/A Calibration Factors

## Overview
D/A calibration factors are now saved and loaded with the calibration YAML file. The backend functionality is complete - D/A factors are persisted across application restarts. However, for full integration, UI controls should be added to the CalibrationFactor dialog using Visual Studio's Resource Editor.

## Current Status (After This PR)
- ✅ **Backend logic implemented**: D/A factors are saved/loaded with YAML files
- ✅ **Persistence working**: D/A factors no longer reset on application restart
- ✅ **CalibrationFactor class updated**: Includes D/A member variables
- ⚠️ **UI controls missing**: D/A factors can be edited via the existing "D/A Channels" menu item
- ⚠️ **Future work**: Integrate D/A controls into CalibrationFactor dialog, then remove D/A Channels menu

## How to Use (Current Workflow)
Until UI integration is complete:
1. Open "D/A Channels" from the menu to edit D/A calibration factors
2. Open "Calibration Factors" dialog
3. Click "Save" - this now saves **both** A/D and D/A calibration factors to YAML
4. Next time you start the app, click "Load" in "Calibration Factors" - D/A factors will be restored
5. No need to manually configure D/A factors on each startup anymore! 🎉

## Future UI Integration Steps

### Step 1: Open Resource Editor
1. Open `src/DigitShowDST.rc` in Visual Studio
2. Navigate to Dialog → IDD_Calibration_Factor

### Step 2: Expand Dialog Size
The current dialog is 362x280 pixels. You'll need to expand it to accommodate D/A calibration controls.

Suggested new size: **362x450** pixels (add ~170 pixels height)

### Step 3: Add D/A Calibration Group Box
Add a new GroupBox control below the "Initial Specimen Data" group:
- **Caption**: "D/A Calibration Factors (CH0-CH7)"
- **Position**: Below Initial Specimen Data group
- **Size**: Similar width to other groups (~300 pixels)
- **Height**: ~120 pixels

### Step 4: Add D/A Calibration Controls
For each of the 8 D/A channels, add:

#### Labels (Static Text):
- "CH0 (Motor)"
- "CH1 (Motor Cluch)"
- "CH2 (Motor Speed)"
- "CH3 (EP1)"
- "CH4 (EP2)"
- "CH5 (-----------)"
- "CH6 (-----------)"
- "CH7 (-----------)"

#### Edit Controls (already defined in resource.h):
**For DA_Cal_a** (linear coefficient):
- IDC_EDIT_DA_Cala00 through IDC_EDIT_DA_Cala07

**For DA_Cal_b** (constant coefficient):
- IDC_EDIT_DA_Calb00 through IDC_EDIT_DA_Calb07

#### Layout Suggestion:
```
   Label          DA_Cal_a     DA_Cal_b
   --------       --------     --------
   CH0 (Motor)    [edit]       [edit]
   CH1 (...)      [edit]       [edit]
   ...
```

### Step 5: Add Column Headers
Add static text labels above the edit controls:
- "DA_Cal_a" - Linear coefficient (a) in: Output = a*Physical + b
- "DA_Cal_b" - Constant coefficient (b)

### Step 6: Update Dialog Layout
Adjust the following controls:
- Move "Close" button down to bottom of expanded dialog
- Move status text (IDC_STATIC_STATUS) down accordingly
- Ensure Load/Save buttons are accessible

### Step 7: Add DDX Code (Already Done)
The DoDataExchange code will need to be uncommented once UI controls are added. Add this code to `CalibrationFactor.cpp` in `DoDataExchange()`:

```cpp
// D/A calibration factors (8 channels)
static constexpr std::array<int, CHANNELS_DA> IDS_DA_CALA = {
    IDC_EDIT_DA_Cala00, IDC_EDIT_DA_Cala01, IDC_EDIT_DA_Cala02,
    IDC_EDIT_DA_Cala03, IDC_EDIT_DA_Cala04, IDC_EDIT_DA_Cala05,
    IDC_EDIT_DA_Cala06, IDC_EDIT_DA_Cala07};

static constexpr std::array<int, CHANNELS_DA> IDS_DA_CALB = {
    IDC_EDIT_DA_Calb00, IDC_EDIT_DA_Calb01, IDC_EDIT_DA_Calb02,
    IDC_EDIT_DA_Calb03, IDC_EDIT_DA_Calb04, IDC_EDIT_DA_Calb05,
    IDC_EDIT_DA_Calb06, IDC_EDIT_DA_Calb07};

// Bind D/A calibration factors
for (size_t i = 0; i < CHANNELS_DA; ++i)
{
    DDX_Text(pDX, IDS_DA_CALA[i], m_DA_Cala[i]);
    DDX_Text(pDX, IDS_DA_CALB[i], m_DA_Calb[i]);
}
```

### Step 8: Remove D/A Channel Menu
After UI integration is complete and tested:
1. Open `src/DigitShowDST.rc`
2. Navigate to Menu → IDR_MAINFRAME
3. Remove or disable the "D/A Channel" menu item
4. Remove or comment out `OnDAChannel()` handler in `MainFrm.cpp`

## Testing Checklist
After UI integration:
- [ ] Open CalibrationFactor dialog - verify D/A controls are visible
- [ ] Edit D/A calibration values - verify they update correctly
- [ ] Click "Save" button - verify D/A values are saved to YAML
- [ ] Close and reopen app
- [ ] Click "Load" button - verify D/A values are loaded correctly
- [ ] Click "Update" button - verify D/A values are applied to global arrays
- [ ] Test that D/A calibration affects control output correctly

## Reference Files
- Schema: `schemas/calibration_factor.schema.json` - Shows structure of da_calibration_data
- Old dialog: `IDD_DA_Channel` in `src/DigitShowDST.rc` - Can copy layout/labels from here
- Resource IDs: `src/resource.h` - Lines 274-290 define IDC_EDIT_DA_Cala* and IDC_EDIT_DA_Calb*

## Notes
- The resource IDs (IDC_EDIT_DA_Cala00-07, IDC_EDIT_DA_Calb00-07) already exist from the old DA_Channel dialog
- These can be reused in the CalibrationFactor dialog
- The backend code already handles saving/loading D/A factors to/from YAML
- Only the UI layout needs to be created manually

## Default Values
The default D/A calibration values (from Variables.hpp):
```cpp
DA_Cal_a = {0.0, 0.0, 0.0033333, 0.017854906, 0.018384256, 0.0, 0.0, 0.0}
DA_Cal_b = {0.0, 0.0, 0.0, -0.286962967, -0.335375138, 0.0, 0.0, 0.0}
```

These will be displayed when the dialog opens and can be edited by the user.
