#include "appserial.h"
#include "constants.h"
#include "../boards/OpenFIREshared.h"
#include <QMessageBox>

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
        } else for(const auto &port : serialFoundList) {
            if(!currentPortsNames.contains(port.portName())) {
                currentPorts = serialFoundList;
                printf("%s not found in current ports, overriding old serial devices list...\n", port.portName().toLocal8Bit().constData());
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

    for(const auto &port : portsList)
        newList.append(port.portName());

    return newList;
}

bool AppSerial::GetSettings(const QString &portName)
{
    for(const auto &curPort : currentPorts)
        if(portName == curPort.portName())
            port.setPort(curPort);

    if(!port.portName().isEmpty()) {
        port.setBaudRate(QSerialPort::Baud9600);
        if(port.open(QIODevice::ReadWrite)) {
            //serialActive = true;

            // windows needs DTR enabled to actually read responses.
            port.setDataTerminalReady(true);

            port.write("XP");
            if(port.waitForBytesWritten(500)) {
                if(port.waitForReadyRead(500)) {
                    QByteArray bufStr = port.readLine().trimmed();
                    QList<QByteArray> buffer = bufStr.split(',');

                    if(buffer.at(0) == "CAMERROR: Not available") {
                        QMessageBox::warning(nullptr,  "Device Error: Camera not available!",
                                             "Data received from the board indicates that the camera is in a bad state.\n"
                                             "This can happen if the camera wires are crossed (data wire to clock pin, clock wire to data pin).\n\n"
                                             "The camera must be removed or resoldered to resolve this.");
                        buffer.takeFirst();
                    }

                    if(buffer.at(0).contains("OpenFIRE")) {
                        printf("OpenFIRE gun detected!\n");

                        emit Serial_SetProgressRange(5);

                        App_Const::board.versionNumber = buffer.at(1).constData();
                        printf("Version number: %s\n", App_Const::board.versionNumber.toLocal8Bit().constData());

                        App_Const::board.versionCodename = buffer.at(2).constData();
                        printf("Version codename: %s\n", App_Const::board.versionCodename.toLocal8Bit().constData());

                        App_Const::board.boardType = buffer.at(3).constData();
                        printf("Board type: %s\n", App_Const::board.boardType.toLocal8Bit().constData());

                        App_Const::board.selectedProfile = buffer.at(4).toInt();
                        App_Const::board.previousProfile = App_Const::board.selectedProfile;

                        // get TUSB info
                        port.write("Xli");
                        port.waitForBytesWritten(500);
                        port.waitForReadyRead(500);

                        bufStr = port.readLine().trimmed();
                        buffer = bufStr.split(',');
                        App_Const::tinyUSBtable.tinyUSBid = buffer.at(0);
                        App_Const::tinyUSBtable_orig.tinyUSBid = App_Const::tinyUSBtable.tinyUSBid;

                        if(buffer[1] == "SERIALREADERR01")
                            App_Const::tinyUSBtable.tinyUSBname = "";
                        else App_Const::tinyUSBtable.tinyUSBname = buffer.at(1);

                        App_Const::tinyUSBtable_orig.tinyUSBname = App_Const::tinyUSBtable.tinyUSBname;

                        emit Serial_ProgressUpdate(1, "Getting Settings (1)");

                        port.clear();

                        // get toggles
                        port.write("Xlb");
                        if(port.waitForBytesWritten(500)) {
                            if(port.waitForReadyRead(500)) {
                                QString bufStr = port.readLine().trimmed();
                                QStringList buffer = bufStr.split(',');

                                // booleans
                                for(uint8_t i = 0; i < OF_Const::boolTypesCount; i++) {
                                    if(!buffer.isEmpty()) {
                                        App_Const::boolSettings[i] = buffer[i].toInt();
                                        App_Const::boolSettings_orig[i] = App_Const::boolSettings[i];
                                    } else break;
                                }

                                emit Serial_ProgressUpdate(2, "Getting Settings (2)");

                                // pins
                                if(App_Const::boolSettings[OF_Const::customPins]) {
                                    port.clear();
                                    port.write("Xlp");
                                    port.waitForBytesWritten(500);
                                    port.waitForReadyRead(500);
                                    App_Const::inputsMap_orig.clear(), App_Const::inputsMap.clear();
                                    bufStr = port.readLine().trimmed();
                                    buffer = bufStr.split(',');
                                    for(uint8_t i = 0; i < OF_Const::boardInputsCount; i++) {
                                        if(!buffer.isEmpty())
                                            App_Const::inputsMap_orig[i] = buffer[i].toInt();
                                        else break;
                                    }
                                } else {
                                    for(int i = 0; i < OF_Const::boardInputsCount; i++)
                                        App_Const::inputsMap_orig[i] = OF_Const::btnUnmapped;
                                }

                                emit Serial_ProgressUpdate(3, "Getting Settings (3)");

                                App_Const::inputsMap = App_Const::inputsMap_orig;

                                // settings
                                port.clear();
                                port.write("Xls");
                                port.waitForBytesWritten(500);
                                port.waitForReadyRead(500);
                                bufStr = port.readLine().trimmed();
                                buffer = bufStr.split(',');
                                for(uint8_t i = 0; i < OF_Const::settingsTypesCount; i++) {
                                    if(!buffer.isEmpty()) {
                                        App_Const::settingsTable[i] = buffer[i].toInt();
                                        App_Const::settingsTable_orig[i] = App_Const::settingsTable[i];
                                    } else break;
                                }

                                emit Serial_ProgressUpdate(4, "Getting Profiles Data");

                                // profiles
                                App_Const::profilesTable.clear(), App_Const::profilesTable_orig.clear();

                                for(uint8_t i = 0;; i++) {
                                    port.clear();
                                    port.write("XlP" + QByteArray::number(i));
                                    port.waitForBytesWritten(500);
                                    if(port.waitForReadyRead(500)) {
                                        // TODO (in fw): could be safer if each line was prepended with what type of profile table value it is.
                                        bufStr = port.readLine().trimmed();
                                        buffer = bufStr.split(',');

                                        App_Const::profilesTable << App_Const::profilesTable_s(), App_Const::profilesTable_orig << App_Const::profilesTable_s();

                                        // copy settings
                                        App_Const::profilesTable[i].topOffset = buffer.takeFirst().toInt(),
                                        App_Const::profilesTable[i].bottomOffset = buffer.takeFirst().toInt(),
                                        App_Const::profilesTable[i].leftOffset = buffer.takeFirst().toInt(),
                                        App_Const::profilesTable[i].rightOffset = buffer.takeFirst().toInt(),
                                        App_Const::profilesTable[i].TLled = buffer.takeFirst().toFloat(),
                                        App_Const::profilesTable[i].TRled = buffer.takeFirst().toFloat(),
                                        App_Const::profilesTable[i].irSensitivity = buffer.takeFirst().toInt(),
                                        App_Const::profilesTable[i].runMode = buffer.takeFirst().toInt(),
                                        App_Const::profilesTable[i].layoutType = buffer.takeFirst().toInt(),
                                        App_Const::profilesTable[i].color = buffer.takeFirst().toLong(),
                                        App_Const::profilesTable[i].profName = buffer.takeFirst().toLocal8Bit();

                                        App_Const::profilesTable_orig[i] = App_Const::profilesTable.at(i);
                                    } else break;
                                }

                                emit Serial_ProgressUpdate(5, "Successfully synced data!");

                                return true;

                            } else QMessageBox::warning(nullptr, "Sync Error: Data hasn't arrived!!",
                                                                 "Device was detected, but settings request wasn't received in time!\n"
                                                                 "This can happen if the app was closed in the middle of an operation.\n\n"
                                                                 "Try selecting the device again.");
                        } else {
                            printf("Couldn't send any data in time! Does the port even exist???\n");
                            return false;
                        }
                    } else {
                        printf("Port did not respond with expected response! Got: %s\n", buffer.join(',').constData());
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
                printf("Couldn't send any data in time! Does the port even exist???\n");
                return false;
            }
        } else {
            QMessageBox::warning(nullptr,   "Serial port is already in use!",
                                            "This usually indicates that the port is being used by something else, e.g. Arduino IDE's serial monitor, or another command line app (stty, screen).\n\n"
                                            "Please close the offending application and try selecting this port again.");
            return false;
        }
    } else return false;
    return false;
}

bool AppSerial::OneShotSend(const QByteArray &string)
{
    if(port.isOpen()) {
        port.write(string);
        if(port.waitForBytesWritten(1000))
            return true;
        else return false;
    } else return false;
}

bool AppSerial::CommitSettings()
{
    if(port.isOpen()) {
        // send a signal so the gun pauses its test outputs for the save op.
        port.write("Xm");
        port.waitForBytesWritten(1000);

        QStringList serialQueue;
        for(uint8_t i = 0; i < OF_Const::boolTypesCount; i++)
            serialQueue.append(QString("Xm.0.%1.%2").arg(i).arg(App_Const::boolSettings[i]));

        if(App_Const::boolSettings[OF_Const::customPins])
            for(uint8_t i = 0; i < App_Const::inputsMap.count(); i++)
                serialQueue.append(QString("Xm.1.%1.%2").arg(i).arg(App_Const::inputsMap.value(i)));

        for(uint8_t i = 0; i < OF_Const::settingsTypesCount; i++)
            serialQueue.append(QString("Xm.2.%1.%2").arg(i).arg(App_Const::settingsTable[i]));

        serialQueue.append(QString("Xm.3.0.%1").arg(App_Const::tinyUSBtable.tinyUSBid));
        if(!App_Const::tinyUSBtable.tinyUSBname.isEmpty())
            serialQueue.append(QString("Xm.3.1.%1").arg(App_Const::tinyUSBtable.tinyUSBname));

        for(uint8_t i = 0; i < 4; i++) {
            serialQueue.append(QString("Xm.P.i.%1.%2").arg(i).arg(App_Const::profilesTable[i].irSensitivity));
            serialQueue.append(QString("Xm.P.r.%1.%2").arg(i).arg(App_Const::profilesTable[i].runMode));
            serialQueue.append(QString("Xm.P.l.%1.%2").arg(i).arg(App_Const::profilesTable[i].layoutType));
            serialQueue.append(QString("Xm.P.c.%1.%2").arg(i).arg(App_Const::profilesTable[i].color));
            serialQueue.append(QString("Xm.P.n.%1.%2").arg(i).arg(App_Const::profilesTable[i].profName));
        }
        serialQueue.append("XS");

        emit Serial_SetProgressRange(serialQueue.length()-1);

        // throw out whatever's in the buffer if there's anything there.
        port.clear();

        for(uint8_t i = 0; i < serialQueue.length(); i++) {
            port.write(serialQueue.at(i).toLocal8Bit());
            port.waitForBytesWritten(1000);
            if(port.waitForReadyRead(1000)) {
                QString buffer = port.readLine();
                if(buffer.contains("OK:") || buffer.contains("NOENT:")) {
                    emit Serial_ProgressUpdate(i, "Committing settings to microcontroller...");
                } else if(i == serialQueue.length() - 1 && buffer.contains("Saving preferences...")) {
                    emit Serial_ProgressUpdate(serialQueue.length()-1, "Successfully synced settings!");
                    return true;
                } else return false;
            } else return false;
        }
    } else return false;
    return false;
}

void AppSerial::Disconnect()
{
    if(port.isOpen()) {
        port.write("XE");
        port.waitForBytesWritten(500);
        port.waitForReadyRead(500);
        port.close();
    }

    port.setPortName("");
}
