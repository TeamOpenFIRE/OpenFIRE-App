/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Copyright (C) 2024  Team OpenFIRE

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "../boards/OpenFIREshared.h"
#include <QString>
#include <QVector>
#include <QMap>

class App_Const
{
public:
    enum {
        pBoxIRsens = 0,
        pBoxRunMode,
        pBoxLayout,
        pBoxAR
    } profileBoxesTypes_e;

    /// @brief      Types of objects that can be made interactive
    /// @details    These are required for mouse over interactivity.
    ///             Different types distinguish which tab it's made for,
    ///             to activate or modify the appropriate things.
    enum {
        trackPinbox = 0,
        trackSettingsItem,
        trackProfileItem,
        trackTestItem
    } uiTrackableObjects_e;

    static inline const QStringList testLabelNames = {
        "Trigger",
        "Button A",
        "Button B",
        "Start",
        "Select",
        "D-Pad Up",
        "D-Pad Down",
        "D-Pad Left",
        "D-Pad Right",
        "Button C",
        "Pedal",
        "Pedal 2",
        "Pump Action",
        "Home"
    };

    typedef struct boardInfo_t {
        uint8_t selectedProfile;
        uint8_t previousProfile;
        QString boardType;
        QString versionNumber;
        QString versionCodename;
    } boardInfo_s;

    typedef struct tinyUSBtable_t {
        QString tinyUSBid;
        QString tinyUSBname;
    } tinyUSBtable_s;

    typedef struct profilesTable_t {
        uint16_t topOffset      = 0;
        uint16_t bottomOffset   = 0;
        uint16_t leftOffset     = 0;
        uint16_t rightOffset    = 0;
        uint16_t TLled          = 0;
        uint16_t TRled          = 0;
        uint8_t irSensitivity   = 0;
        uint8_t runMode         = 0;
        uint8_t layoutType      = false;
        uint32_t color          = 0;
        QString profName        = "";
    } profilesTable_s;

    // Currently loaded board object
    static inline boardInfo_s board;

    /// @brief      Current array of booleans
    /// @details    Meant for toggle/on-off type settings specifically
    static inline bool boolSettings[OF_Const::boolTypesCount] = { false };

    /// @brief      Array of booleans last synced from the microcontroller
    /// @details    This is only updated on saving and loading settings successfully
    static inline bool boolSettings_orig[OF_Const::boolTypesCount] = { false };

    /// @brief      Current array of tunable settings
    static inline uint32_t settingsTable[OF_Const::settingsTypesCount] = { 0 };

    /// @brief      Array of tunables last synced from the microcontroller
    /// @details    This is only updated on saving and loading settings successfully
    static inline uint32_t settingsTable_orig[OF_Const::settingsTypesCount] = { 0 };

    // Currently loaded board's TinyUSB identifier info
    static inline tinyUSBtable_s tinyUSBtable;
    // TinyUSB ident, as loaded from the board
    static inline tinyUSBtable_s tinyUSBtable_orig;

    // Current calibration profiles
    static inline QVector<profilesTable_s> profilesTable;
    // Calibration profiles, as loaded from the board
    static inline QVector<profilesTable_s> profilesTable_orig;

    // Map of what inputs are put where,
    // Key = button/output, Value = pin number occupying, if any.
    // Value of -1 means unmapped.
    // Key order based on boardInputs_e, minus 1
    // Map functions used in deduplication
    static inline QMap<uint8_t, int8_t> inputsMap;
    // Inputs map, as loaded from the board
    static inline QMap<uint8_t, int8_t> inputsMap_orig;
};

#endif // CONSTANTS_H
