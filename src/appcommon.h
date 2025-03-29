/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Common shared assets & constants.

    Copyright (C) 2025  Team OpenFIRE

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

#ifndef APPCOMMON_H
#define APPCOMMON_H

#include "../boards/OpenFIREshared.h"

#include <QString>
#include <QVector>
#include <QMap>

// Maximum amount of GPIO that the RP2040 microcontroller has available
#define PINS_COUNT 30

class App_Common
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

    enum {
        dataCurrent = 0,
        dataOrig
    } dataBlocks_e;

    typedef struct boardInfo_t {
        uint8_t    selectedProfile;
        uint8_t    previousProfile;
        QByteArray boardType;
        QByteArray versionNumber;
        QByteArray versionCodename;
    } boardInfo_s;

    typedef struct tinyUSBtable_t {
        uint16_t   tinyUSBid;
        QByteArray tinyUSBname;
    } tinyUSBtable_s;

    typedef struct profilesTable_t {
        int32_t  topOffset      = 0;
        int32_t  bottomOffset   = 0;
        int32_t  leftOffset     = 0;
        int32_t  rightOffset    = 0;
        float    TLled          = 0;
        float    TRled          = 0;
        uint8_t  irSensitivity  = 0;
        uint8_t  runMode        = 0;
        uint8_t  layoutType     = false;
        uint32_t color          = 0;
        QByteArray profName     = "";
    } profilesTable_s;

    // Currently loaded board object
    static inline boardInfo_s board;

    //// TODO: merge orig into main arrays to make them 2D arrays (where second array = main or orig)

    /// @brief      Current array of booleans
    /// @details    Meant for toggle/on-off type settings specifically
    static inline bool boolSettings[2][OF_Const::boolTypesCount] = { false };

    /// @brief      Current array of tunable settings
    static inline uint32_t settingsTable[2][OF_Const::settingsTypesCount] = { 0 };

    /// @brief      Array of I2C peripheral devices that can be toggled
    static inline bool i2cPeriphs[2][OF_Const::i2cDevicesCount] = { false };

    static inline uint32_t i2cOledPrefs[2][OF_Const::oledSettingsTypes] = { false };

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
