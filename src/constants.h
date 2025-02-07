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

#include <QMainWindow>

enum boardTypes_e {
    nothing = 0,
    rpipico,
    rpipicow,
    adafruitItsyRP2040,
    adafruitKB2040,
    arduinoNanoRP2040,
    waveshareZero,
    vccgndYD,
    presetBoardsCount,
    generic = 255
};

enum boardInputs_e {
    btnReserved = -1,
    btnUnmapped = 0,
    btnTrigger,
    btnGunA,
    btnGunB,
    btnGunC,
    btnStart,
    btnSelect,
    btnGunUp,
    btnGunDown,
    btnGunLeft,
    btnGunRight,
    btnPedal,
    btnPedal2,
    btnHome,
    btnPump,
    rumblePin,
    solenoidPin,
    rumbleSwitch,
    solenoidSwitch,
    autofireSwitch,
    neoPixel,
    ledR,
    ledG,
    ledB,
    camSDA,
    camSCL,
    periphSDA,
    periphSCL,
    battery,
    analogX,
    analogY,
    tempPin,
    boardInputsCount
};

enum boolTypes_e {
    customPins = 0,
    rumble,
    solenoid,
    autofire,
    simplePause,
    holdToPause,
    commonAnode,
    lowButtonsMode,
    rumbleFF,
    boolTypesCount
};

enum settingsTypes_e {
    rumbleStrength = 0,
    rumbleInterval,
    solenoidNormalInterval,
    solenoidFastInterval,
    solenoidHoldLength,
    autofireWaitFactor,
    holdToPauseLength,
    customLEDcount,
    customLEDstatic,
    customLEDcolor1,
    customLEDcolor2,
    customLEDcolor3,
    settingsTypesCount
};

enum pinTypes_e {
    pinNothing = 0,
    pinDigital,
    pinAnalog
};

enum layoutTypes_e {
    layoutSquare = 0,
    layoutDiamond
};

const uint8_t boardCustomPresetsCount[presetBoardsCount] = {
    0, // padding
    0, // rpipico
    0, // rpipicow
    1, // itsybitsy
    0, // KB2040
    0, // arduinoNano
    0, // waveshareZero
    0, // vccgnd
};

const QStringList rpipicoPresetsList = {
    "EZCon"
};

const QStringList adafruitItsyBitsyRP2040PresetsList = {
    "SAMCO 1.1"
};

/* Inputs map order:
 * Trigger, A, B, C, Start, Select,
 * Up, Down, Left, Right, Pedal, Pedal2,
 * Home, Pump, Rumble Sig, Solenoid Sig, Rumble Switch, Solenoid Switch,
 * Autofire Switch, NeoPixel, LEDR, LEDG, LEDB, Camera SDA,
 * Camera SCL, Periph SDA, Periph SCL, Battery, Analog X, Analog Y,
 * Temperature
 */

const int8_t rpipicoPresets[1][boardInputsCount-1] = {
    // name 1: this is currently a placeholder for testing
    {1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1,
     -1}
    ,
    // name 2: subsequent layouts go here
};

const int8_t adafruitItsyBitsyRP2040Presets[2][boardInputsCount-1] = {
    // SAMCO 2.0
    /*
    {6, 27, 26, -1, 28, 29,
     9, 7, 8, 10, 4, -1,
     11, -1, 24, 25, -1, -1,
     -1, -1, -1, -1, -1, 2,
     3, -1, -1, -1, -1, -1,
     -1}
    ,*/
    // SAMCO 1.1
    {10, 6, 7, -1, -1, -1,
     -1, -1, -1, -1, 27, -1,
     9, -1, 8, -1, -1, -1,
     -1, -1, -1, -1, -1, 2,
     3, -1, -1, -1, -1, -1,
     -1}
    ,
};

typedef struct boardInfo_t {
    uint8_t type = nothing;
    QString versionNumber;
    QString versionCodename;
    uint8_t selectedProfile;
    uint8_t previousProfile;
} boardInfo_s;

typedef struct tinyUSBtable_t {
    QString tinyUSBid;
    QString tinyUSBname;
} tinyUSBtable_s;

typedef struct profilesTable_t {
    uint16_t topOffset;
    uint16_t bottomOffset;
    uint16_t leftOffset;
    uint16_t rightOffset;
    uint16_t TLled;
    uint16_t TRled;
    uint8_t irSensitivity;
    uint8_t runMode;
    bool layoutType;
    uint32_t color;
    QString profName;
} profilesTable_s;

typedef struct boardLayout_t {
    int8_t pinAssignment;
    uint8_t pinType;
} boardLayout_s;

const boardLayout_t rpipicoLayout[] = {
    {btnGunA, pinDigital},     {btnGunB, pinDigital},
    {btnGunC, pinDigital},     {btnStart, pinDigital},
    {btnSelect, pinDigital},   {btnHome, pinDigital},
    {btnGunUp, pinDigital},    {btnGunDown, pinDigital},
    {btnGunLeft, pinDigital},  {btnGunRight, pinDigital},
    {ledR, pinDigital},        {ledG, pinDigital},
    {ledB, pinDigital},        {btnPump, pinDigital},
    {btnPedal, pinDigital},    {btnTrigger, pinDigital},
    {solenoidPin, pinDigital}, {rumblePin, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {camSDA, pinDigital},      {camSCL, pinDigital},
    {btnUnmapped, pinDigital}, {btnReserved, pinNothing}, // 23, 24, 25
    {btnReserved, pinNothing}, {btnReserved, pinNothing}, // are unused/unexposed
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog},  // ADC pins
    {tempPin, pinAnalog},      {-2, pinNothing}           // ADC, padding
};

const boardLayout_t adafruitItsyRP2040Layout[] = {
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {camSDA, pinDigital},      {camSCL, pinDigital},
    {btnPedal, pinDigital},    {btnUnmapped, pinDigital},
    {btnTrigger, pinDigital},  {btnGunDown, pinDigital},
    {btnGunLeft, pinDigital},  {btnGunUp, pinDigital},
    {btnGunRight, pinDigital}, {btnGunC, pinDigital},
    {btnUnmapped, pinDigital}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {rumblePin, pinDigital},   {solenoidPin, pinDigital},
    {btnGunB, pinAnalog},      {btnGunA, pinAnalog},
    {btnStart, pinAnalog},     {btnSelect, pinAnalog}
};

const boardLayout_t adafruitKB2040Layout[] = {
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {camSDA, pinDigital},      {camSCL, pinDigital},
    {btnGunB, pinDigital},     {rumblePin, pinDigital},
    {btnGunC, pinDigital},     {solenoidPin, pinDigital},
    {btnSelect, pinDigital},   {btnStart, pinDigital},
    {btnGunRight, pinDigital},

    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing},

    {btnGunUp, pinDigital},    {btnGunLeft, pinDigital},
    {btnGunDown, pinDigital},

    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing},

    {tempPin, pinAnalog},      {btnHome, pinAnalog},
    {btnTrigger, pinAnalog},   {btnGunA, pinAnalog}
};

const boardLayout_t arduinoNanoRP2040Layout[] = {
    {btnTrigger, pinDigital},  {btnPedal, pinDigital},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnGunA, pinDigital},     {btnGunC, pinDigital},
    {btnUnmapped, pinDigital}, {btnGunB, pinDigital},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {camSDA, pinDigital},      {camSCL, pinDigital},
    {btnReserved, pinNothing}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog},
    {tempPin, pinAnalog},      {btnUnmapped, pinAnalog}
};

const boardLayout_t waveshareZeroLayout[] = {
    {btnTrigger, pinDigital},  {btnGunA, pinDigital},
    {btnGunB, pinDigital},     {btnGunC, pinDigital},
    {btnStart, pinDigital},    {btnSelect, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {camSDA, pinDigital},      {camSCL, pinDigital},
    {solenoidPin, pinDigital}, {rumblePin, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog},
    {btnUnmapped, pinAnalog},  {tempPin, pinAnalog}
};

const boardLayout_t genericLayout[] = {
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnReserved, pinNothing}, // 23, 24, 25
    {btnReserved, pinNothing}, {btnReserved, pinNothing}, // are (usually) unused/unexposed
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog},
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog}
};

#endif // CONSTANTS_H
