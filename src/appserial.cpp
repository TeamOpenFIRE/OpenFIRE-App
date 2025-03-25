/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Serial input/output routines.

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

#include "appserial.h"
#include "appcommon.h"
#include "../boards/OpenFIREshared.h"
#include <QMessageBox>
#include <qtconcurrentrun.h>

bool AppSerial::SearchPorts()
{
    QList<QSerialPortInfo> serialFoundList = QSerialPortInfo::availablePorts();
    if(!serialFoundList.isEmpty()) {
        // Yeah, sue me, we reading this backwards to make stack management easier.
        for(int i = serialFoundList.length() - 1; i >= 0; --i) {
            if(serialFoundList.at(i).vendorIdentifier() == 0xF143) {
                printf("Found device @ %s\n", serialFoundList.at(i).systemLocation().toLocal8Bit().constData());
            } else {
                if(!serialFoundList.at(i).systemLocation().contains("tty"))
                    printf("Deleting dummy device %s\n", serialFoundList.at(i).systemLocation().toLocal8Bit().constData());
                serialFoundList.removeAt(i);
            }
        }

        // Compare to and replace original list only if entries differ.
        if(currentPorts.size() != serialFoundList.size()) {
            currentPorts = serialFoundList;
            printf("Current ports list does not match new list, overriding...\n");
            currentPortsNames = GeneratePortsList(currentPorts);
            return true;
        } else for(const auto &foundPort : serialFoundList) {
            if(!currentPortsNames.contains(foundPort.portName())) {
                currentPorts = serialFoundList;
                printf("%s not found in current ports, overriding old serial devices list...\n", foundPort.portName().toLocal8Bit().constData());
                currentPortsNames = GeneratePortsList(currentPorts);
                return true;
            }
        }
        return false;
    } else return true;
}

QStringList AppSerial::GeneratePortsList(const QList<QSerialPortInfo> &portsList)
{
    QStringList newList;

    for(const auto &newPort : portsList)
        newList.append(newPort.portName());

    return newList;
}

bool AppSerial::GetSettings(const QString &portName)
{
    if(port.isOpen()) {
        OneShotSend((char)OF_Const::serialTerminator);
        port.close();
    }

    for(const auto &curPort : currentPorts)
        if(portName == curPort.portName()+" (" + curPort.description() + ')')
            port.setPort(curPort);

    if(!port.portName().isEmpty()) {
        port.setBaudRate(QSerialPort::Baud9600);
        if(port.open(QIODevice::ReadWrite)) {
            // windows needs DTR enabled to actually read responses.
            port.setDataTerminalReady(true);
            port.clear();

            if(char buf[] = {(char)OF_Const::sDock1, (char)OF_Const::sDock2}; OneShotSend(buf, 2, true)) {
                QList<QByteArray> buffer = port.readLine().split((char)OF_Const::serialTerminator);

                if(buffer.size() >= 5) {
                    emit Serial_SetProgressRange(5);
                    emit Serial_ProgressUpdate(1, "Getting Board Info");

                    App_Common::board.versionNumber = buffer.takeFirst().constData();
                    printf("Version number: %s\n", App_Common::board.versionNumber.constData());

                    App_Common::board.versionCodename = buffer.takeFirst().constData();
                    printf("Version codename: %s\n", App_Common::board.versionCodename.constData());

                    App_Common::board.boardType = buffer.takeFirst().constData();
                    printf("Board type: %s\n", App_Common::board.boardType.constData());

                    App_Common::board.selectedProfile = buffer.takeFirst().toInt();
                    App_Common::board.previousProfile = App_Common::board.selectedProfile;

                    memcpy(&App_Common::tinyUSBtable.tinyUSBid, buffer.at(0).constData(), 2);
                    App_Common::tinyUSBtable.tinyUSBname = &buffer.takeFirst().constData()[2];
                    App_Common::tinyUSBtable_orig = App_Common::tinyUSBtable;

                    if(buffer.size()) if(buffer.takeFirst().at(0) == OF_Const::sError)
                        ShowError("Device Error: Camera not available!",
                                  "<p>Data received from the board indicates that the camera is in a bad state.<br>"
                                  "This can happen if, for example, the camera wires are crossed<br>"
                                  "(data wire to clock pin, clock wire to data pin),<br>"
                                  "or the camera pins are wired to a different component,<br>"
                                  "such as a button or Force Feedback output.</p>"
                                  "<p>You are able to change the camera pins in the <i>Boards Layout</i> tab<br>"
                                  "if they should be mapped different GPIO;<br>"
                                  "Otherwise, the camera wires must be resoldered to resolve this error.</p>",
                                  QMessageBox::Warning);

                    // toggles
                    if(OneShotSend((char)OF_Const::sGetToggles, true)) {
                        // booleans
                        memset(App_Common::boolSettings, false, OF_Const::boolTypesCount);
                        for(uint8_t i = 0;; i++) {
                            if(port.bytesAvailable() && i < OF_Const::boolTypesCount)
                                port.read((char*)&App_Common::boolSettings[i], sizeof(bool));
                            else break;
                        }
                        memcpy(App_Common::boolSettings_orig, App_Common::boolSettings, sizeof(App_Common::boolSettings));

                        emit Serial_ProgressUpdate(2, "Getting Settings (1)");

                        // pins
                        if(App_Common::boolSettings[OF_Const::customPins]) {
                            port.clear();
                            if(OneShotSend((char)OF_Const::sGetPins, true)) {
                                App_Common::inputsMap_orig.clear(), App_Common::inputsMap.clear();

                                for(uint8_t i = 0;; i++)
                                    if(port.bytesAvailable() && i < OF_Const::boardInputsCount)
                                        port.read((char*)&App_Common::inputsMap_orig[i], sizeof(int8_t));
                                    else break;
                            } else {
                                printf("Didn't receive any data in time!\n");
                                return false;
                            }
                        } else for(int i = 0; i < OF_Const::boardInputsCount; i++)
                                App_Common::inputsMap_orig[i] = OF_Const::btnUnmapped;

                        App_Common::inputsMap = App_Common::inputsMap_orig;

                        emit Serial_ProgressUpdate(3, "Getting Settings (2)");

                        // settings
                        port.clear();
                        if(OneShotSend((char)OF_Const::sGetSettings, true)) {
                            memset(App_Common::settingsTable, 0, sizeof(App_Common::settingsTable));
                            while(true) {
                                if(!port.bytesAvailable()) if(!port.waitForReadyRead(2000)) break;
                                if(port.peek(1).at(0) == (char)OF_Const::serialTerminator) break;
                                else {
                                    uint8_t i = port.read(1).at(0);
                                    if(i < OF_Const::settingsTypesCount)
                                        port.read((char*)&App_Common::settingsTable[i], sizeof(uint32_t));
                                }
                            }
                            memcpy(App_Common::settingsTable_orig, App_Common::settingsTable, sizeof(App_Common::settingsTable));

                            emit Serial_ProgressUpdate(4, "Getting Profiles Data");

                            // profiles
                            App_Common::profilesTable.clear(), App_Common::profilesTable_orig.clear();

                            for(uint8_t i = 0;; i++) {
                                port.clear();
                                if(char buf[] = {(char)OF_Const::sGetProfile, (char)i}; OneShotSend(buf, 2, true)) {
                                    if(port.peek(1).at(0) == (char)OF_Const::serialTerminator) {
                                        break;
                                    } else {
                                        App_Common::profilesTable << App_Common::profilesTable_s(), App_Common::profilesTable_orig << App_Common::profilesTable_s();

                                        while(port.bytesAvailable()) {
                                            switch(port.read(1).at(0)) {
                                            case (char)OF_Const::profTopOffset:    port.read((char*)&App_Common::profilesTable[i].topOffset,     sizeof(uint32_t)); break;
                                            case (char)OF_Const::profBottomOffset: port.read((char*)&App_Common::profilesTable[i].bottomOffset,  sizeof(uint32_t)); break;
                                            case (char)OF_Const::profLeftOffset:   port.read((char*)&App_Common::profilesTable[i].leftOffset,    sizeof(uint32_t)); break;
                                            case (char)OF_Const::profRightOffset:  port.read((char*)&App_Common::profilesTable[i].rightOffset,   sizeof(uint32_t)); break;
                                            case (char)OF_Const::profTLled:        port.read((char*)&App_Common::profilesTable[i].TLled,         sizeof(float));    break;
                                            case (char)OF_Const::profTRled:        port.read((char*)&App_Common::profilesTable[i].TRled,         sizeof(float));    break;
                                            case (char)OF_Const::profIrSens:       port.read((char*)&App_Common::profilesTable[i].irSensitivity, sizeof(uint8_t));  break;
                                            case (char)OF_Const::profRunMode:      port.read((char*)&App_Common::profilesTable[i].runMode,       sizeof(uint8_t));  break;
                                            case (char)OF_Const::profIrLayout:     port.read((char*)&App_Common::profilesTable[i].layoutType,    sizeof(uint8_t));  break;
                                            case (char)OF_Const::profColor:        port.read((char*)&App_Common::profilesTable[i].color,         sizeof(uint32_t)); break;
                                            case (char)OF_Const::profName:                           App_Common::profilesTable[i].profName = port.read(16);         break;
                                            default: break;
                                            }
                                        }

                                        App_Common::profilesTable_orig[i] = App_Common::profilesTable.at(i);
                                    }
                                } else break;
                            }

                            emit Serial_ProgressUpdate(5, "Successfully synced data!");
                            return true;
                        } else {
                            printf("Couldn't send any data in time! Was it disconnected mid-transaction?\n");
                            return false;
                        }
                    } else {
                        printf("Couldn't send any data in time! Was it disconnected mid-transaction?\n");
                        return false;
                    }
                } else {
                    printf("Port did not respond with expected response! Got: %s\n", buffer.join(' ').constData());
                    return false;
                }
            } else {
                QMessageBox::warning(nullptr,   "Data hasn't arrived! (Stale state?)",
                                     "Device was detected, but initial settings request wasn't received in time!\n"
                                     "This can happen if the app was unexpectedly closed and the gun is in a stale docked state.\n\n"
                                     "Try selecting the device again.");
                return false;
            }
        } else {
            QMessageBox::warning(nullptr,   "Serial port is already in use!",
                                            "This usually indicates that the port is being used by something else, e.g. Arduino IDE's serial monitor, or another command line app (stty, screen).\n\n"
                                            "Please close the offending application and try selecting this port again.");
            return false;
        }
    } else return false;
}

bool AppSerial::OneShotSend(const char* string, const unsigned int &len, const bool &waitForResponse)
{
    if(port.isOpen()) {
        port.write((len > 0) ? string : string, len);
        if(port.waitForBytesWritten(1000))
            if(waitForResponse)
                if(port.waitForReadyRead(1000)) return true;
                else return false;
            else return true;
        else return false;
    } else return false;
}

bool AppSerial::OneShotSend(const char &chara, const bool &waitForResponse)
{
    if(port.isOpen()) {
        port.write(&chara);
        if(port.waitForBytesWritten(1000))
            if(waitForResponse)
                if(port.waitForReadyRead(1000)) return true;
                else return false;
            else return true;
        else return false;
    } else return false;
}

bool AppSerial::CommitSettings()
{
    if(port.isOpen()) {
        emit Serial_SetProgressRange(7);

        if(OneShotSend((char)OF_Const::sCommitStart)) {
            port.clear();

            emit Serial_ProgressUpdate(1, "Sending Toggles...");
            for(uint8_t i = 0; i < OF_Const::boolTypesCount; i++) {
                if(char buf[3] = {(char)OF_Const::sCommitToggles, (char)i, (char)App_Common::boolSettings[i]}; OneShotSend(buf, 3, true)) {
                    if(port.read(1).at(0) != App_Common::boolSettings[i]) {
                        OneShotSend((char)OF_Const::serialTerminator);
                        return false;
                    }
                } else return false;
            }

            if(App_Common::boolSettings[OF_Const::customPins]) {
                emit Serial_ProgressUpdate(2, "Sending Pins Map...");
                for(uint8_t i = 0; i < OF_Const::boardInputsCount; i++) {
                    if(char buf[3] = {(char)OF_Const::sCommitPins, (char)i, (char)App_Common::inputsMap.value(i)}; OneShotSend(buf, 3, true)) {
                        if(port.read(1).at(0) != App_Common::inputsMap.value(i)) {
                            OneShotSend((char)OF_Const::serialTerminator);
                            return false;
                        }
                    } else return false;
                }
            }

            emit Serial_ProgressUpdate(3, "Sending Settings...");
            for(uint8_t i = 0; i < OF_Const::settingsTypesCount; i++) {
                char buf[6] = {(char)OF_Const::sCommitSettings, (char)i};
                memcpy(&buf[2], (uint8_t*)&App_Common::settingsTable[i], sizeof(uint32_t));
                if(OneShotSend(buf, sizeof(buf), true)) {
                    if(memcmp(port.read(4).constData(), &App_Common::settingsTable[i], sizeof(uint32_t))) {
                        OneShotSend((char)OF_Const::serialTerminator);
                        return false;
                    }
                } else return false;
            }

            emit Serial_ProgressUpdate(4, "Sending Profile Data...");
            for(uint8_t i = 0; i < App_Common::profilesTable.count(); i++) {

            }

            emit Serial_ProgressUpdate(5, "Sending TinyUSB ID Data...");
            char buf[18] = {(char)OF_Const::sCommitID, (char)OF_Const::usbPID};
            memcpy(&buf[2], (uint8_t*)&App_Common::tinyUSBtable.tinyUSBid, sizeof(uint16_t));
            if(OneShotSend(buf, 4, true)) {
                if(memcmp(port.read(2).constData(), &App_Common::tinyUSBtable.tinyUSBid, sizeof(uint16_t))) {
                    OneShotSend((char)OF_Const::serialTerminator);
                    return false;
                } else {
                    memset(&buf[1], '\0', sizeof(buf)-1);
                    buf[1] = (char)OF_Const::usbName;
                    memcpy(&buf[2], (uint8_t*)App_Common::tinyUSBtable.tinyUSBname.constData(), App_Common::tinyUSBtable.tinyUSBname.size());
                    if(OneShotSend(buf, sizeof(buf), true)) {
                        if(strcmp(port.read(16).constData(), App_Common::tinyUSBtable.tinyUSBname.constData())) {
                            OneShotSend((char)OF_Const::serialTerminator);
                            return false;
                        }
                    } else return false;
                }
            } else return false;

            emit Serial_ProgressUpdate(6, "Saving...");
            if(OneShotSend((char)OF_Const::sSave, true)) {
                if(char newBuf[2] = {(char)OF_Const::sSave, (char)true}; memcmp(port.read(2).constData(), newBuf, sizeof(newBuf))) {
                    emit Serial_ProgressUpdate(7);
                    return true;
                } else return false;
            } else return false;
        } else return false;
    } else return false;
}

void AppSerial::Disconnect()
{
    char buf[] = {(char)OF_Const::serialTerminator, (char)OF_Const::serialTerminator, (char)OF_Const::serialTerminator};
    OneShotSend(buf, 3);
    port.close();

    port.setPortName("");
}
