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
    adafruitItsyRP2040,
    adafruitKB2040,
    arduinoNanoRP2040,
    waveshareZero,
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
    btnHome,
    btnPump,
    rumblePin,
    solenoidPin,
    rumbleSwitch,
    solenoidSwitch,
    autofireSwitch,
    ledR,
    ledG,
    ledB,
    neoPixel,
    camSDA,
    camSCL,
    periphSDA,
    periphSCL,
    analogX,
    analogY,
    tempPin
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
    rumbleFF
};

enum settingsTypes_e {
    rumbleStrength = 0,
    rumbleInterval,
    solenoidNormalInterval,
    solenoidFastInterval,
    solenoidHoldLength,
    customLEDcount,
    autofireWaitFactor,
    holdToPauseLength
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

typedef struct boardInfo_t {
    uint8_t type = nothing;
    float versionNumber = 0.0;
    QString versionCodename;
    uint8_t selectedProfile;
    uint8_t previousProfile;
} boardInfo_s;

typedef struct tinyUSBtable_t {
    QString tinyUSBid;
    QString tinyUSBname;
} tinyUSBtable_s;

typedef struct profilesTable_t {
    uint16_t xScale;
    uint16_t yScale;
    uint16_t xCenter;
    uint16_t yCenter;
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
    {camSCL, pinDigital},      {camSDA, pinDigital},
    {btnUnmapped, pinDigital}, {btnReserved, pinNothing}, // 23, 24, 25
    {btnReserved, pinNothing}, {btnReserved, pinNothing}, // are unused/unexposed
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog},  // ADC pins
    {btnUnmapped, pinAnalog},  {-2, pinNothing}           // ADC, padding
};

const boardLayout_t adafruitItsyRP2040Layout[] = {
    {btnGunUp, pinDigital},    {btnGunDown, pinDigital},
    {camSDA, pinDigital},      {camSCL, pinDigital},
    {btnGunLeft, pinDigital},  {btnGunRight, pinDigital},
    {btnTrigger, pinDigital},  {btnGunA, pinDigital},
    {btnGunB, pinDigital},     {btnGunC, pinDigital},
    {btnStart, pinDigital},    {btnSelect, pinDigital},
    {btnPedal, pinDigital},    {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {btnUnmapped, pinDigital}, {btnUnmapped, pinDigital},
    {btnUnmapped, pinDigital}, {btnReserved, pinNothing},
    {btnReserved, pinNothing}, {btnReserved, pinNothing},
    {rumblePin, pinDigital},   {solenoidPin, pinDigital},
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog},
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog}
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
    {btnUnmapped, pinAnalog},  {btnUnmapped, pinAnalog}
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
