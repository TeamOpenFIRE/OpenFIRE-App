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

#endif // CONSTANTS_H
