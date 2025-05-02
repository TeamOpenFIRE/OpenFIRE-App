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

                if(buffer.size() >= 3) {
                    emit Serial_SetProgressRange(5);
                    emit Serial_ProgressUpdate(1, "Getting Board Info");

                    ////* Opening board message bits *////
                    App_Common::board.versionNumber = buffer.takeFirst().constData();
                    printf("Version number: %s\n", App_Common::board.versionNumber.constData());

                    App_Common::board.boardType = buffer.takeFirst().constData();
                    printf("Board type: %s\n", App_Common::board.boardType.constData());

                    memcpy(&App_Common::tinyUSBtable, buffer.at(0).constData(), buffer.at(0).length());
                    memcpy(&App_Common::tinyUSBtable_orig, &App_Common::tinyUSBtable, buffer.takeFirst().length());
                    
                    if(buffer.size()) if(buffer.takeFirst().at(0) == (char)OF_Const::sError)
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

                    ////* toggles *////
                    if(OneShotSend((char)OF_Const::sGetToggles, true)) {
                        memset(App_Common::boolSettings, false, sizeof(App_Common::boolSettings));
                        if(!BatchStoreSettings( App_Common::boolSettings[App_Common::dataCurrent],
                                                App_Common::OFPresets.boolTypes_Strings,
                                                sizeof(App_Common::boolSettings[App_Common::dataCurrent]) / OF_Const::boolTypesCount))
                            return false;

                        memcpy(App_Common::boolSettings[App_Common::dataOrig],
                               App_Common::boolSettings[App_Common::dataCurrent],
                               sizeof(App_Common::boolSettings[App_Common::dataCurrent]));

                        emit Serial_ProgressUpdate(2, "Getting Settings (1)");

                        ////* pins *////
                        if(App_Common::boolSettings[App_Common::dataCurrent][OF_Const::customPins]) {
                            port.clear();
                            if(OneShotSend((char)OF_Const::sGetPins, true)) {
                                App_Common::inputsMap_orig.clear(), App_Common::inputsMap.clear();

                                if(!BatchStoreSettings(&App_Common::inputsMap_orig,
                                                       App_Common::OFPresets.boardInputs_Strings,
                                                       0))
                                    return false;
                            } else {
                                printf("Didn't receive any data in time!\n");
                                return false;
                            }
                        } else for(int i = 0; i < OF_Const::boardInputsCount; ++i)
                            App_Common::inputsMap_orig[i] = OF_Const::btnUnmapped;

                        App_Common::inputsMap = App_Common::inputsMap_orig;

                        emit Serial_ProgressUpdate(3, "Getting Settings (2)");

                        ////* settings *////
                        port.clear();
                        if(OneShotSend((char)OF_Const::sGetSettings, true)) {
                            memset(App_Common::settingsTable, 0, sizeof(App_Common::settingsTable));

                            if(!BatchStoreSettings(App_Common::settingsTable[App_Common::dataCurrent],
                                                   App_Common::OFPresets.settingsTypes_Strings,
                                                   sizeof(App_Common::settingsTable[App_Common::dataCurrent]) / OF_Const::settingsTypesCount))
                                return false;

                            memcpy(App_Common::settingsTable[App_Common::dataOrig],
                                   App_Common::settingsTable[App_Common::dataCurrent],
                                   sizeof(App_Common::settingsTable[App_Common::dataCurrent]));

                            emit Serial_ProgressUpdate(4, "Getting Profiles Data");

                            // profiles
                            App_Common::profilesTable.clear(), App_Common::profilesTable_orig.clear();

                            port.clear();
                            if(OneShotSend((char)OF_Const::sGetProfile, true)) {

                                if(BatchStoreSettings(nullptr, App_Common::OFPresets.profSettingTypes_Strings, sizeof(float))) {
                                    App_Common::profilesTable_orig = App_Common::profilesTable;
                                    App_Common::board.previousProfile = App_Common::board.selectedProfile;
                                    emit Serial_ProgressUpdate(5, "Successfully synced data!");
                                    return true;
                                } else return false;
                            } else {
                                printf("Couldn't send any data in time! Was it disconnected mid-transaction?\n");
                                return false;
                            }
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
                    RequestToReboot();
                    return false;
                }
            } else {
                QMessageBox::warning(nullptr,   "Data hasn't arrived! (Stale state?)",
                                                "Device was detected, but initial settings request wasn't received in time!\n"
                                                "This can happen if the app was unexpectedly closed and the gun is in a stale docked state.\n\n"
                                                "Try selecting the device again.");
                RequestToReboot();
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

bool AppSerial::BatchStoreSettings(void *dataPtr, const std::unordered_map<std::string, int> &dataMap, const size_t &dataSize)
{
    QByteArray buf;
    uint8_t sizeRead = 0;
    while(true) {
        if(port.bytesAvailable() && port.peek(1).at(0) == (char)OF_Const::serialTerminator) break;
        else if(!port.bytesAvailable()) { if(!port.waitForReadyRead(500)) break; }
        else {
            buf = RecvDataName();

            port.read((char*)&sizeRead, 1);

            // is this string detected in strings map?
            if(dataMap.count(buf.constData())) {
                // For Pins/Inputs Map data (write to InputsMap rather than pointer)
                if(dataPtr == &App_Common::inputsMap_orig)
                    port.read((char*)&App_Common::inputsMap_orig[dataMap.at(buf.constData())], sizeRead);
                // For Profile Data (has extra bits)
                else if(&dataMap == &App_Common::OFPresets.profSettingTypes_Strings) {
                    // Current Profile bit has no extra profile bit like the rest of the data
                    if(dataMap.at(buf.constData()) == OF_Const::profCurrent) {
                        port.read((char*)&App_Common::board.selectedProfile, sizeRead);
                    } else {
                        size_t profNum = 0;
                        port.read((char*)&profNum, 1);

                        if(profNum == App_Common::profilesTable.size())
                            App_Common::profilesTable << App_Common::profilesTable_s();

                        port.read((char*)&App_Common::profilesTable[profNum] + (dataSize * dataMap.at(buf.constData())), sizeRead);
                    }
                // All other (Generic) data
                } else port.read((char*)dataPtr + (dataSize * dataMap.at(buf.constData())),  sizeRead);
            // String not detected, skip over
            } else {
                printf("No data found for %s\n", buf.constData());
                // skip the profile num byte if reading profile type data
                if(&dataMap == &App_Common::OFPresets.profSettingTypes_Strings) port.read(1);
                port.read(sizeRead);
            }
        }
    }
    // if exited from serialTerminator at start of RX buffer, prune that out before exiting
    port.read(1);
    return true;
}

QByteArray AppSerial::RecvDataName()
{
    QByteArray data;
    size_t pos = 0;
    while(true) {
        pos += port.read(&RXbuf[pos], port.peek(port.bytesAvailable()).indexOf('\0') < 0 ? port.bytesAvailable() : port.peek(port.bytesAvailable()).indexOf('\0')+1);
        if(RXbuf[pos-1] != '\0') {
            if(!port.waitForReadyRead(500)) return "";
        } else {
            data.append(RXbuf, pos);
            return data;
        }
    }
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

        if(OneShotSend((char)OF_Const::sCommitStart, true)) {
            // in case board sends a stale temp/analog state response.
            emit Serial_ProgressUpdate(0, "Waiting for board...");
            do {
                if(port.read(1).at(0) == (char)OF_Const::sCommitStart) break;
                if(!port.bytesAvailable()) if(!port.waitForReadyRead(1000)) break;
            } while (port.bytesAvailable());

            port.clear();

            emit Serial_ProgressUpdate(1, "Sending Toggles...");
            if(!BatchSendSettings(App_Common::boolSettings[App_Common::dataCurrent],
                                  App_Common::OFPresets.boolTypes_Strings,
                                  sizeof(App_Common::boolSettings[App_Common::dataCurrent]) / OF_Const::boolTypesCount))
                return false;

            if(App_Common::boolSettings[OF_Const::customPins]) {
                emit Serial_ProgressUpdate(2, "Sending Pins Map...");
                // convert pins map to temp buffer
                int8_t inputMapFW[OF_Const::boardInputsCount];
                for(size_t i = 0; i < OF_Const::boardInputsCount; ++i)
                    inputMapFW[i] = App_Common::inputsMap.value(i);

                if(!BatchSendSettings(inputMapFW,
                                      App_Common::OFPresets.boardInputs_Strings,
                                      sizeof(inputMapFW) / OF_Const::boardInputsCount))
                    return false;
            }

            emit Serial_ProgressUpdate(3, "Sending Settings...");
            if(!BatchSendSettings(App_Common::settingsTable[App_Common::dataCurrent],
                                  App_Common::OFPresets.settingsTypes_Strings,
                                  sizeof(App_Common::settingsTable[App_Common::dataCurrent]) / OF_Const::settingsTypesCount))
                return false;

            emit Serial_ProgressUpdate(4, "Sending Profile Data...");
            for(size_t i = 0; i < App_Common::profilesTable.count(); ++i) {
                if(!BatchSendSettings(&App_Common::profilesTable[i],
                                      App_Common::OFPresets.profSettingTypes_Strings,
                                      sizeof(float),
                                      i))
                    return false;
            }

            emit Serial_ProgressUpdate(5, "Sending TinyUSB ID Data...");
            TXbuf[0] = (char)OF_Const::sCommitID;
            memcpy(&TXbuf[1], (uint8_t*)&App_Common::tinyUSBtable, sizeof(App_Common::tinyUSBtable_s));
            for(size_t sendAttempt = 1;; ++sendAttempt) {
                if(OneShotSend(TXbuf, sizeof(uint8_t)+sizeof(App_Common::tinyUSBtable_s), true)) {
                    port.read(RXbuf, sizeof(App_Common::tinyUSBtable_s));
                    if(memcmp(&App_Common::tinyUSBtable, RXbuf, sizeof(App_Common::tinyUSBtable_s))) {
                        if(sendAttempt >= 3) return false;
                    } else break;
                } else return false;
            }

            emit Serial_ProgressUpdate(6, "Saving...");
            if(OneShotSend((char)OF_Const::sSave, true)) {
                if(char newBuf[2] = {(char)OF_Const::sSave, (char)true}; memcmp(port.read(2).constData(), newBuf, sizeof(newBuf)) == 0) {
                    emit Serial_ProgressUpdate(7);
                    return true;
                } else return false;
            } else return false;
        } else return false;
    } else return false;
}

bool AppSerial::BatchSendSettings(void *dataPtr, const std::unordered_map<std::string, int> &dataMap, const size_t &dataSize, const size_t &profNum)
{
    size_t txLen = 0;
    bool profCurrentMarked = false;
    if(&dataMap == &App_Common::OFPresets.boolTypes_Strings)
        TXbuf[txLen++] = OF_Const::sCommitToggles;
    else if(&dataMap == &App_Common::OFPresets.boardInputs_Strings)
        TXbuf[txLen++] = OF_Const::sCommitPins;
    else if(&dataMap == &App_Common::OFPresets.settingsTypes_Strings)
        TXbuf[txLen++] = OF_Const::sCommitSettings;
    else if(&dataMap == &App_Common::OFPresets.profSettingTypes_Strings)
        TXbuf[txLen++] = OF_Const::sCommitProfile;
    for(auto &pair : dataMap) {
        txLen = 1;
        strcpy(&TXbuf[txLen], pair.first.c_str());
        // std::string length doesn't account for terminator
        txLen += pair.first.length()+1;

        if(dataMap == App_Common::OFPresets.profSettingTypes_Strings) {
            if(pair.second < OF_Const::profIrSens) continue;
            else if(pair.second == OF_Const::profCurrent) {
                if(profCurrentMarked) continue;
                TXbuf[txLen++] = sizeof(uint8_t);
                TXbuf[txLen++] = App_Common::board.selectedProfile;
                profCurrentMarked = true;
            } else {
                if(pair.second == OF_Const::profName)
                     TXbuf[txLen++] = sizeof(App_Common::profilesTable_s::profName);
                else TXbuf[txLen++] = dataSize;

                TXbuf[txLen++] = profNum;

                memcpy(&TXbuf[txLen], (uint8_t*)dataPtr + (dataSize * pair.second), TXbuf[txLen-2]);
                txLen += TXbuf[txLen-2];
            }
        } else {
            TXbuf[txLen++] = dataSize;
            memcpy(&TXbuf[txLen], (uint8_t*)dataPtr + (dataSize * pair.second), dataSize);
            txLen += dataSize;
        }

        // try to resend data if not matching, fail after three tries
        for(size_t sendAttempt = 1;; ++sendAttempt) {
            if(OneShotSend(TXbuf, txLen, true)) {
                port.read(RXbuf, port.bytesAvailable());
                if(memcmp(&TXbuf[1], RXbuf, txLen-1)) {
                    if(sendAttempt >= 3) return false;
                } else break;
            } else return false;
        }
    }
    return true;
}

void AppSerial::Disconnect()
{
    char buf[] = {(char)OF_Const::serialTerminator, (char)OF_Const::serialTerminator, (char)OF_Const::serialTerminator};
    OneShotSend(buf, 3);
    port.close();

    port.setPortName("");
}

void AppSerial::RequestToReboot()
{
    if(QMessageBox::critical(nullptr, "Reset Board to Bootloader?",
                                      "<p>The board you selected did not respond to the app properly.</p>"
                                      "<p>This can usually be resolved by rebooting the microcontroller to its bootloader, and then updating the board to the latest firmware, which can be found at:</p>"
                                      "<p><a href='https://github.com/TeamOpenFIRE/OpenFIRE-Firmware/releases/latest'><span style=' text-decoration: underline; color:#8ab4f8;'>https://github.com/TeamOpenFIRE/OpenFIRE-Firmware/releases/latest</span></a></p>"
                                      "<p>Would you like to reboot this board to apply an update?</p>",
                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        RebootToBootldr();
}

void AppSerial::RebootToBootldr()
{
    // The py script had this backwards. huh.
    port.setBaudRate(QSerialPort::Baud1200);
    port.setDataTerminalReady(false);
    port.close();
}
