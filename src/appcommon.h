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

#define BUTTON_COUNT 14

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
        trackButtonMapItem,
        trackTestItem
    } uiTrackableObjects_e;

    enum {
        dataCurrent = 0,
        dataOrig,
        dataTablesCount
    } dataBlocks_e;

    // Single instance of presets and board info
    static inline OF_Const OFPresets;

    typedef struct boardInfo_t {
        int        selectedProfile;
        int        previousProfile;
        QByteArray type;
        QByteArray arch;
        QByteArray version;
    } boardInfo_s;

    typedef struct tinyUSBtable_t {
        uint16_t   tinyUSBid;
        char       tinyUSBname[16];
    } tinyUSBtable_s;

    typedef struct profilesTable_t {
        int32_t    topOffset     = 0;
        int32_t    bottomOffset  = 0;
        int32_t    leftOffset    = 0;
        int32_t    rightOffset   = 0;
        float      TLled         = 0;
        float      TRled         = 0;
        float      AdjX          = 0;
        float      AdjY          = 0;
        uint32_t   irSensitivity = 0;
        uint32_t   runMode       = 0;
        uint32_t   layoutType    = 0;
        uint32_t   aspectRatio   = 0;
        uint32_t   color         = 0;
        char       profName[16]  = "";
    } profilesTable_s;

    // Currently loaded board object
    static inline boardInfo_s board;

    /// @brief      Current array of booleans
    /// @details    Meant for toggle/on-off type settings specifically
    static inline bool boolSettings[dataTablesCount][OF_Const::boolTypesCount] = { false };

    /// @brief      Current array of tunable settings
    static inline uint32_t settingsTable[dataTablesCount][OF_Const::settingsTypesCount] = { 0 };

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

    enum {
        inputFuncData = 0,
        inputFuncOrder,
        inputFuncTypes
    } kbInputs_e;

    enum {
        inputMouse = 0,
        inputKB,
        inputGamepad,
        inputTypes
    } inputFuncTypes_e;

    // Map of input funcs
    // array 1 = original and current data array
    // array 2 = buttons count
    // array 3 = data for button ((func type : func num) * 3)
    static inline uint8_t inputFuncTable[dataTablesCount][BUTTON_COUNT][inputTypes*2];

    // Keyboard inputs reference map
    // first int is the value representing the key used by the firmware,
    // second int is the desired order (since no Map allows
    static const inline QMap<std::string, QVector<int>> keyboardInputsMap = {
        {"Player-relative Start Key",   {0xFF,      0 }},
        {"Player-relative Coin Key",    {0xFE,      1 }},
        {"Up Arrow",                    {0xDA,      2 }},
        {"Down Arrow",                  {0xD9,      3 }},
        {"Left Arrow",                  {0xD8,      4 }},
        {"Right Arrow",                 {0xD7,      5 }},
        {"Enter/Return",                {0xB0,      6 }},
        {"Backspace",                   {0xB2,      7 }},
        {"Escape",                      {0xB1,      8 }},
        {"Left Ctrl",                   {0x80,      9 }},
        {"Right Ctrl",                  {0x84,      10}},
        {"Left Alt",                    {0x82,      11}},
        {"Right Alt",                   {0x86,      12}},
        {"Left Shift",                  {0x81,      13}},
        {"Right Shift",                 {0x85,      14}},
        {"Tab",                         {0xB3,      15}},
        {"A",                           {'a',       16}},
        {"B",                           {'b',       17}},
        {"C",                           {'c',       18}},
        {"D",                           {'d',       19}},
        {"E",                           {'e',       20}},
        {"F",                           {'f',       21}},
        {"G",                           {'g',       22}},
        {"H",                           {'h',       23}},
        {"I",                           {'i',       24}},
        {"J",                           {'j',       25}},
        {"K",                           {'k',       26}},
        {"L",                           {'l',       27}},
        {"M",                           {'m',       28}},
        {"N",                           {'n',       29}},
        {"O",                           {'o',       30}},
        {"P",                           {'p',       31}},
        {"Q",                           {'q',       32}},
        {"R",                           {'r',       33}},
        {"S",                           {'s',       34}},
        {"T",                           {'t',       35}},
        {"U",                           {'u',       36}},
        {"V",                           {'v',       37}},
        {"W",                           {'w',       38}},
        {"X",                           {'x',       39}},
        {"Y",                           {'y',       40}},
        {"Z",                           {'z',       41}},
        {"F1",                          {0xC2,      42}},
        {"F2",                          {0xC3,      43}},
        {"F3",                          {0xC4,      44}},
        {"F4",                          {0xC5,      45}},
        {"F5",                          {0xC6,      46}},
        {"F6",                          {0xC7,      47}},
        {"F7",                          {0xC8,      48}},
        {"F8",                          {0xC9,      49}},
        {"F9",                          {0xCA,      50}},
        {"F10",                         {0xCB,      51}},
        {"F11",                         {0xCC,      52}},
        {"F12",                         {0xCD,      53}}
    };

    // Used for combobox elements so that the order isn't haphazard
    // TODO: is a map the best way of doing this? hrm
    static inline char* kbOrderedStrings[54];

    static const inline QMap<std::string, QVector<int>> mouseMap = {
        {"Left Click",          {0b00000001, 0}},
        {"Right Click",         {0b00000010, 1}},
        {"Middle Click",        {0b00000100, 2}},
        {"Side Button Back",    {0b00001000, 3}},
        {"Side Button Forward", {0b00010000, 4}}
    };

    // Used for combobox elements so that the order isn't haphazard
    static inline char* mouseOrderedStrings[5];

    // Gamepad inputs reference map
    // first int is the value representing the key used by the firmware,
    // second int is the desired order (since no Map allows
    static const inline QMap<std::string, QVector<int>> gamepadMap = {
        {"A Button",            {0,  0 }},
        {"B Button",            {1,  1 }},
        // C Button (N/A)
        {"X Button",            {3,  2 }},
        {"Y Button",            {4,  3 }},
        // Z Button (N/A)
        {"Left Shoulder",       {6,  4 }},
        {"Right Shoulder",      {7,  5 }},
        {"Left Trigger",        {8,  6 }},
        {"Right Trigger",       {9,  7 }},
        {"Select Button",       {10, 8 }},
        {"Start Button",        {11, 9 }},
        // Home Button (N/A)
        {"Left Stick Click",    {13, 10}},
        {"Right Stick Click",   {14, 11}},
        {"D-Pad Up",            {15, 12}},
        {"D-Pad Down",          {16, 13}},
        {"D-Pad Left",          {17, 14}},
        {"D-Pad Right",         {18, 15}}
    };

    // Used for combobox elements so that the order isn't haphazard
    static inline char* gpadOrderedStrings[16];

    static inline const char* inputFuncTypesStrings[3] = {
        "Mouse",
        "Keyboard",
        "Gamepad"
    };

    static const inline QMap<std::string, QVector<int>> *inputFuncMaps[3] = {
        &mouseMap,
        &keyboardInputsMap,
        &gamepadMap
    };

    static inline const char* i2cTypeLabels[2] = {
        "SDA",
        "SCL"
    };

    static inline const char* spiTypeLabels[4] = {
        "RX",
        "TX",
        "SCK",
        "CSn"
    };
};

#endif // CONSTANTS_H
