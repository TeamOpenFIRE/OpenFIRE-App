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

#include "appmainwindow.h"
#include "constants.h"
#include "ui_appmainwindow.h"
#include "ui_about.h"

#include <QGraphicsScene>
#include <QMessageBox>
#include <QRadioButton>
#include <QSvgRenderer>
#include <QSvgWidget>
#include <QSerialPortInfo>
#include <QtDebug>
#include <QProgressBar>
#include <QProcess>
#include <QStorageInfo>
#include <QThread>
#include <QColorDialog>
#include <QInputDialog>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>

#define PINS_COUNT 30
#define PROFILES_COUNT 4

guiWindow::guiWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::guiWindow)
{
    ui->setupUi(this);

#if !defined(Q_OS_MAC) && !defined(Q_OS_WIN)
    if(qEnvironmentVariable("USER") != "root") {
        QProcess *externalProg = new QProcess;
        QStringList args;
        externalProg->start("/usr/bin/groups", args);
        externalProg->waitForFinished();
        if(!externalProg->readAllStandardOutput().contains("dialout")) {
            QMessageBox::critical(this, "ERROR: User doesn't have serial permissions!",
                                        "Currently, your user is not allowed to have access to serial devices.\n\n"
                                        "To add yourself to the right group, run this command in a terminal and then re-login to your session:\n\n"
                                        "sudo usermod -aG dialout " + qEnvironmentVariable("USER"));
            exit(0);
        }
    } else {
        QMessageBox::critical(this, "ERROR: Running as root is not allowed!", "Please run the OpenFIRE app as a normal user.");
        exit(2);
    }
#endif

    connect(&serialPort, &QSerialPort::readyRead, this, &guiWindow::serialPort_readyRead);

    // These can actually stay, tho.
    for(uint8_t i = 0; i < PROFILES_COUNT; i++) {
        renameBtn[i] = new QPushButton();
        renameBtn[i]->setFlat(true);
        renameBtn[i]->setFixedWidth(20);
        renameBtn[i]->setIcon(QIcon(":/icon/edit.png"));
        connect(renameBtn[i], SIGNAL(clicked()), this, SLOT(renameBoxes_clicked()));
        selectedProfile[i] = new QRadioButton(QString("%1.").arg(i+1));
        connect(selectedProfile[i], SIGNAL(toggled(bool)), this, SLOT(selectedProfile_isChecked(bool)));
        topOffset[i] = new QLabel("0");
        bottomOffset[i] = new QLabel("0");
        leftOffset[i] = new QLabel("0");
        rightOffset[i] = new QLabel("0");
        TLled[i] = new QLabel("0");
        TRled[i] = new QLabel("0");
        irSens[i] = new QComboBox();
        runMode[i] = new QComboBox();
        layoutMode[i] = new QComboBox();
        color[i] = new QPushButton();
        topOffset[i]->setAlignment(Qt::AlignCenter);
        bottomOffset[i]->setAlignment(Qt::AlignCenter);
        leftOffset[i]->setAlignment(Qt::AlignCenter);
        rightOffset[i]->setAlignment(Qt::AlignCenter);
        TLled[i]->setAlignment(Qt::AlignCenter);
        TRled[i]->setAlignment(Qt::AlignCenter);
        irSens[i]->addItem("Default");
        irSens[i]->addItem("Higher");
        irSens[i]->addItem("Highest");
        connect(irSens[i], SIGNAL(activated(int)), this, SLOT(irBoxes_activated(int)));
        runMode[i]->addItem("Normal");
        runMode[i]->addItem("1-Frame Avg");
        runMode[i]->addItem("2-Frame Avg");
        layoutMode[i]->addItems({"Square", "Diamond"});
        connect(layoutMode[i], SIGNAL(activated(int)), this, SLOT(layoutBoxes_activated(int)));
        connect(runMode[i], SIGNAL(activated(int)), this, SLOT(runModeBoxes_activated(int)));
        color[i]->setFixedWidth(32);
        connect(color[i], SIGNAL(clicked()), this, SLOT(colorBoxes_clicked()));
        ui->profilesArea->addWidget(renameBtn[i], i+1, 0, 1, 1);
        ui->profilesArea->addWidget(selectedProfile[i], i+1, 1, 1, 1);
        ui->profilesArea->addWidget(topOffset[i], i+1, 2, 1, 1);
        ui->profilesArea->addWidget(bottomOffset[i], i+1, 4, 1, 1);
        ui->profilesArea->addWidget(leftOffset[i], i+1, 6, 1, 1);
        ui->profilesArea->addWidget(rightOffset[i], i+1, 8, 1, 1);
        ui->profilesArea->addWidget(TLled[i], i+1, 10, 1, 1);
        ui->profilesArea->addWidget(TRled[i], i+1, 12, 1, 1);
        ui->profilesArea->addWidget(irSens[i], i+1, 14, 1, 1);
        ui->profilesArea->addWidget(runMode[i], i+1, 16, 1, 1);
        ui->profilesArea->addWidget(layoutMode[i], i+1, 18, 1, 1);
        ui->profilesArea->addWidget(color[i], i+1, 20, 1, 1);
    }

    // Setup test screen buttons
    for(uint8_t i = 0; i < 16; i++) {
        testLabel[i] = new QLabel;

        // temperature sensor
        if(i == 14) testLabel[i]->setText(OF_Const::valuesNameList[OF_Const::tempPin]);
        // analog stick
        else if(i == 15) testLabel[i]->setText("Analog Stick");
        // every other standard input
        else testLabel[i]->setText(OF_Const::valuesNameList[i+1]);

        testLabel[i]->setEnabled(false);
        testLabel[i]->setAlignment(Qt::AlignCenter);
        testLabel[i]->setFrameStyle(QFrame::Box | QFrame::Raised);

        // analog stick
        if(i == 15)      ui->buttonsTestLayout->addWidget(testLabel[i], 3, 3, 1, 1);
        // temp sensor
        else if(i == 14) ui->buttonsTestLayout->addWidget(testLabel[i], 3, 1, 1, 1);
        // third/second/first row of buttons
        else if(i > 9)   ui->buttonsTestLayout->addWidget(testLabel[i], 2, i-10, 1, 1);
        else if(i > 4)   ui->buttonsTestLayout->addWidget(testLabel[i], 1, i-5, 1, 1);
        else             ui->buttonsTestLayout->addWidget(testLabel[i], 0, i, 1, 1);
    }

    ui->buttonsTestLayout->setRowMinimumHeight(0, 32);
    ui->buttonsTestLayout->setRowMinimumHeight(1, 32);
    ui->buttonsTestLayout->setRowMinimumHeight(2, 32);
    ui->buttonsTestLayout->setRowMinimumHeight(3, 32);

    // Setup Test Mode screen colors
    testPointTLPen.setColor(Qt::green);
    testPointTRPen.setColor(Qt::green);
    testPointBLPen.setColor(Qt::blue);
    testPointBRPen.setColor(Qt::blue);
    testPointMedPen.setColor(Qt::gray);
    testPointDPen.setColor(Qt::red);
    testPointTLPen.setWidth(3);
    testPointTRPen.setWidth(3);
    testPointBLPen.setWidth(3);
    testPointBRPen.setWidth(3);
    testPointMedPen.setWidth(3);
    testPointDPen.setWidth(3);
    testPointTL.setPen(testPointTLPen);
    testPointTR.setPen(testPointTRPen);
    testPointBL.setPen(testPointBLPen);
    testPointBR.setPen(testPointBRPen);
    testPointMed.setPen(testPointMedPen);
    testPointD.setPen(testPointDPen);

    // Actually setup the Test Mode scene
    testScene = new QGraphicsScene();
    testScene->setSceneRect(0, 0, 1024, 768);
    testScene->setBackgroundBrush(Qt::darkGray);
    ui->testView->setScene(testScene);
    testScene->addItem(&testBox);
    testScene->addItem(&testPointTL);
    testScene->addItem(&testPointTR);
    testScene->addItem(&testPointBL);
    testScene->addItem(&testPointBR);
    testScene->addItem(&testPointMed);
    testScene->addItem(&testPointD);
    // TODO: is there a way of dynamically scaling QGraphicsViews?
    ui->testView->scale(0.5, 0.5);

    // hiding tUSB elements by default since this can't be done from default
    ui->tUSBLayoutAdvanced->setVisible(false);

    // set hidden by default until a board with presets is loaded
    ui->presetsBox->setHidden(true);

    // Finally get to the thing!
    aliveTimer = new QTimer();
    connect(aliveTimer, &QTimer::timeout, this, &guiWindow::aliveTimer_timeout);
    statusBar()->showMessage("Welcome to the OpenFIRE app!", 3000);
    PortsSearch();
    usbName.prepend("[No device]");
    ui->productIdConverted->setEnabled(false);
    ui->productIdInput->setValidator(new QIntValidator());
    // TODO: what's a good validator to only accept character values within the range of an unsigned char?
    //ui->productNameInput->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_]+"), this));
    ui->comPortSelector->addItems(usbName);
}

guiWindow::~guiWindow()
{
    if(serialPort.isOpen()) {
        statusBar()->showMessage("Sending undock request to board...");
        serialPort.write("XE");
        serialPort.waitForBytesWritten(2000);
        serialPort.waitForReadyRead(2000);
        serialPort.close();
    }
    delete ui;
}


void guiWindow::PortsSearch()
{
    serialFoundList = QSerialPortInfo::availablePorts();
    if(serialFoundList.isEmpty()) {
        QMessageBox::critical(this, "ERROR: No devices detected!",  "Is the microcontroller board currently running OpenFIRE and is currently plugged in?\n"
                                                                    "Make sure it's connected and recognized by the PC.\n\n"
                                                                    "This app will now close.");
        exit(1);
    } else {
        // Yeah, sue me, we reading this backwards to make stack management easier.
        for(int i = serialFoundList.length() - 1; i >= 0; --i) {
            if(serialFoundList[i].vendorIdentifier() == 0xF143) {
                usbName.prepend(serialFoundList[i].systemLocation());
                printf("Found device @ %s\n", serialFoundList[i].systemLocation().toLocal8Bit().constData());
            } else {
                printf("Deleting dummy device %s\n", serialFoundList[i].systemLocation().toLocal8Bit().constData());
                serialFoundList.removeAt(i);
            }
        }
        if(!usbName.length()) {
            QMessageBox::critical(this, "ERROR: No devices detected!",  "Is the microcontroller board currently running OpenFIRE and is currently plugged in?\n"
                                                                        "Make sure it's connected and recognized by the PC.\n\n"
                                                                        "This app will now close.");
            exit(1);
        }
    }
}


// Bool returns success (false if failed)
bool guiWindow::SerialInit(int portNum)
{
    serialPort.setPort(serialFoundList[portNum]);
    serialPort.setBaudRate(QSerialPort::Baud9600);
    if(serialPort.open(QIODevice::ReadWrite)) {
        qDebug() << "Opened port successfully!";
        serialActive = true;
        // windows needs DTR enabled to actually read responses.
        serialPort.setDataTerminalReady(true);
        serialPort.write("XP");
        if(serialPort.waitForBytesWritten(2000)) {
            if(serialPort.waitForReadyRead(2000)) {
                QByteArray bufStr = serialPort.readLine().trimmed();
                QList<QByteArray> buffer = bufStr.split(',');
                if(buffer[0].contains("OpenFIRE")) {
                    printf("OpenFIRE gun detected!\n");

                    board.versionNumber = buffer[1].constData();
                    printf("Version number: %s\n", board.versionNumber.toLocal8Bit().constData());

                    board.versionCodename = buffer[2].constData();
                    printf("Version codename: %s\n", board.versionCodename.toLocal8Bit().constData());

                    board.boardType = buffer[3].constData();
                    printf("Board type: %s\n", board.boardType.toLocal8Bit().constData());

                    board.selectedProfile = buffer[4].toInt();
                    board.previousProfile = board.selectedProfile;
                    selectedProfile[board.selectedProfile]->setChecked(true);

                    serialPort.write("Xli");
                    serialPort.waitForReadyRead(1000);
                    bufStr = serialPort.readLine().trimmed();
                    buffer = bufStr.split(',');
                    tinyUSBtable.tinyUSBid = buffer[0];
                    tinyUSBtable_orig.tinyUSBid = tinyUSBtable.tinyUSBid;
                    if(buffer[1] == "SERIALREADERR01")
                        tinyUSBtable.tinyUSBname = "";
                    else tinyUSBtable.tinyUSBname = buffer[1];

                    tinyUSBtable_orig.tinyUSBname = tinyUSBtable.tinyUSBname;

                    SerialLoad();
                    return true;
                } else if(buffer[0].contains("Device not available")) {
                    QMessageBox::warning(this,  "Device Error: Camera not available!",
                                                "Data received from the board indicates that the camera is in a bad state.\n"
                                                "This can happen if the camera wires are crossed (data wire to clock pin, clock wire to data pin).\n\n"
                                                "The camera must be removed or resoldered to resolve this.");
                    return false;
                } else {
                    printf("Port did not respond with expected response! Seong fucked this up again.");
                    return false;
                }
            } else {
                QMessageBox::warning(this,  "Data hasn't arrived! (Stale state?)",
                                            "Device was detected, but initial settings request wasn't received in time!\n"
                                            "This can happen if the app was unexpectedly closed and the gun is in a stale docked state.\n\n"
                                            "Try selecting the device again.");
                return false;
            }
        } else {
            printf("Couldn't send any data in time! Does the port even exist??? Fucking dammit Seong!?!?!?");
            return false;
        }
    } else {
        QMessageBox::warning(this,  "Serial port is already in use!",
                                    "This usually indicates that the port is being used by something else, e.g. Arduino IDE's serial monitor, or another command line app (stty, screen).\n\n"
                                    "Please close the offending application and try selecting this port again.");
        return false;
    }
}


void guiWindow::SerialLoad()
{
    serialActive = true;
    serialPort.clear();
    serialPort.write("Xlb");
    if(serialPort.waitForBytesWritten(2000)) {
        if(serialPort.waitForReadyRead(2000)) {
            // booleans
            QString bufStr = serialPort.readLine().trimmed();
            QStringList buffer = bufStr.split(',');
            for(uint8_t i = 0; i < OF_Const::boolTypesCount; i++) {
                if(!buffer.isEmpty()) {
                    boolSettings[i] = buffer[i].toInt();
                    boolSettings_orig[i] = boolSettings[i];
                } else break;
            }

            // pins
            if(boolSettings[OF_Const::customPins]) {
                serialPort.clear();
                serialPort.write("Xlp");
                serialPort.waitForBytesWritten(2000);
                serialPort.waitForReadyRead(2000);
                bufStr = serialPort.readLine().trimmed();
                buffer = bufStr.split(',');
                for(uint8_t i = 0; i < OF_Const::boardInputsCount; i++) {
                    if(!buffer.isEmpty())
                        inputsMap_orig[i] = buffer[i].toInt();
                    else break;
                }
            } else {
                for(int i = 0; i < OF_Const::boardInputsCount; i++)
                    inputsMap_orig[i] = OF_Const::btnUnmapped;
            }

            inputsMap = inputsMap_orig;

            // settings
            serialPort.clear();
            serialPort.write("Xls");
            serialPort.waitForBytesWritten(2000);
            serialPort.waitForReadyRead(2000);
            bufStr = serialPort.readLine().trimmed();
            buffer = bufStr.split(',');
            for(uint8_t i = 0; i < OF_Const::settingsTypesCount; i++) {
                if(!buffer.isEmpty()) {
                    settingsTable[i] = buffer[i].toInt();
                    settingsTable_orig[i] = settingsTable[i];
                } else break;
            }

            // profiles
            profilesTable.resize(4), profilesTable_orig.resize(4);
            for(uint8_t i = 0; i < PROFILES_COUNT; i++) {
                serialPort.clear();
                serialPort.write(QString("XlP%1").arg(i).toLocal8Bit());
                serialPort.waitForBytesWritten(2000);
                if(serialPort.waitForReadyRead(2000)) {
                    // TODO (in fw): needs to be a loooot safer than it is tbh. We make a lot of assumptions here that could get hairy.
                    bufStr = serialPort.readLine().trimmed();
                    buffer = bufStr.split(',');

                    topOffset[i]->setText(buffer[0]), profilesTable[i].topOffset = buffer[0].toInt(), profilesTable_orig[i].topOffset = profilesTable[i].topOffset;
                    bottomOffset[i]->setText(buffer[1]), profilesTable[i].bottomOffset = buffer[1].toInt(), profilesTable_orig[i].bottomOffset = profilesTable[i].bottomOffset;
                    leftOffset[i]->setText(buffer[2]), profilesTable[i].leftOffset = buffer[2].toInt(), profilesTable_orig[i].leftOffset = profilesTable[i].leftOffset;
                    rightOffset[i]->setText(buffer[3]), profilesTable[i].rightOffset = buffer[3].toInt(), profilesTable_orig[i].rightOffset = profilesTable[i].rightOffset;
                    TLled[i]->setText(buffer[4]), profilesTable[i].TLled = buffer[4].toFloat(), profilesTable_orig[i].TLled = profilesTable[i].TLled;
                    TRled[i]->setText(buffer[5]), profilesTable[i].TRled = buffer[5].toFloat(), profilesTable_orig[i].TRled = profilesTable[i].TRled;
                    profilesTable[i].irSensitivity = buffer[6].toInt(), profilesTable_orig[i].irSensitivity = profilesTable[i].irSensitivity, irSens[i]->setCurrentIndex(profilesTable[i].irSensitivity);
                    profilesTable[i].runMode = buffer[7].toInt(), profilesTable_orig[i].runMode = profilesTable[i].runMode, runMode[i]->setCurrentIndex(profilesTable[i].runMode);
                    layoutMode[i]->setCurrentIndex(buffer[8].toInt()), profilesTable[i].layoutType = buffer[8].toInt(), profilesTable_orig[i].layoutType = profilesTable[i].layoutType;
                    color[i]->setStyleSheet(QString("background-color: #%1").arg(buffer[9].toLong(), 6, 16, QLatin1Char('0'))), profilesTable[i].color = buffer[9].toLong(), profilesTable_orig[i].color = profilesTable[i].color;
                    selectedProfile[i]->setText(buffer[10]), profilesTable[i].profName = buffer[10].toLocal8Bit(), profilesTable_orig[i].profName = profilesTable[i].profName;
                } else break;
            }
            serialActive = false;
        } else {
            QMessageBox::warning(this,  "Sync Error: Data hasn't arrived!!",    "Device was detected, but settings request wasn't received in time!\n"
                                                                            "This can happen if the app was closed in the middle of an operation.\n\n"
                                                                            "Try selecting the device again.");
            //qDebug() << "Didn't receive any data in time! Dammit Seong, you jiggled the cable too much again!";
        }
    } else {
        printf("Couldn't send any data in time! Does the port even exist??? Fucking dammit Seong!?!?!?\n");
    }
}


void guiWindow::BoxesUpdate()
{
    // enabling custom pins
    if(boolSettings[OF_Const::customPins]) {
        // enable pinboxes
        for(int i = 0; i < PINS_COUNT; i++)
            pinBoxes[i]->setEnabled(true);

        // if the custom pins setting *grabbed from the gun* has been set
        if(boolSettings_orig[OF_Const::customPins]) {
            // reset pinboxes
            for(int i = 0; i < PINS_COUNT; i++)
                pinBoxes[i]->setCurrentIndex(OF_Const::btnUnmapped+1);

            // set pinboxes to copied values (pinbox index is off by 1)
            for(int i = 0; i < inputsMap_orig.count(); i++)
                if(inputsMap_orig.value(i) > OF_Const::btnUnmapped && inputsMap_orig.value(i) < PINS_COUNT)
                    pinBoxes[inputsMap_orig.value(i)]->setCurrentIndex(i+1);

        // else, if the board *was using default maps* before switching to custom (no need to re-set pinboxes)
        } else {
            // copy original map, which clears this map (as boards using defaults comes with no actual map instated)
            inputsMap = inputsMap_orig;

            // copy presets to inputs map
            if(OF_Const::boardsPresetsMap.count(board.boardType.toStdString())) {
                for(int i = 0; i < PINS_COUNT; i++) {
                    if(OF_Const::boardsPresetsMap.at(board.boardType.toStdString()).pin[i] > OF_Const::btnUnmapped) {
                        inputsMap[OF_Const::boardsPresetsMap.at(board.boardType.toStdString()).pin[i]] = i;
                    }
                }
            }
        }

        return;

    // disabling custom pins, reset to presets
    } else {
        // reset inputs map, as it's not even referenced when custom pins are disabled
        for(int i = 0; i < inputsMap.size(); i++)
            inputsMap[i] = OF_Const::btnUnmapped;

        // copy preset layout to pinboxes
        if(OF_Const::boardsPresetsMap.count(board.boardType.toStdString()))
            for(int i = 0; i < PINS_COUNT; i++) {
                pinBoxes[i]->setEnabled(false);
                pinBoxes[i]->setCurrentIndex(OF_Const::boardsPresetsMap.at(board.boardType.toStdString()).pin[i]+1);
            }
        // generics don't come with mappings
        else for(int i = 0; i < PINS_COUNT; i++) {
            pinBoxes[i]->setEnabled(false);
            pinBoxes[i]->setCurrentIndex(OF_Const::btnUnmapped+1);
        }

        return;
    }
}


void guiWindow::DiffUpdate()
{
    settingsDiff = 0;

    if(boolSettings_orig[OF_Const::customPins] != boolSettings[OF_Const::customPins])
        settingsDiff++;

    if(boolSettings[OF_Const::customPins])
        // TODO: why is inputsMap getting an entry @ key 255???
        if(inputsMap_orig != inputsMap)
            settingsDiff++;

    for(uint8_t i = 1; i < OF_Const::boolTypesCount; i++)
        if(boolSettings_orig[i] != boolSettings[i])
            settingsDiff++;

    for(uint8_t i = 0; i < OF_Const::settingsTypesCount; i++)
        if(settingsTable_orig[i] != settingsTable[i])
            settingsDiff++;

    if(tinyUSBtable_orig.tinyUSBid != tinyUSBtable.tinyUSBid)
        settingsDiff++;

    if(tinyUSBtable_orig.tinyUSBname != tinyUSBtable.tinyUSBname)
        settingsDiff++;

    if(board.selectedProfile != board.previousProfile)
        settingsDiff++;

    for(uint8_t i = 0; i < PROFILES_COUNT; i++) {
        if(profilesTable_orig[i].profName != profilesTable[i].profName)
            settingsDiff++;

        if(profilesTable_orig[i].topOffset != profilesTable[i].topOffset)
            settingsDiff++;

        if(profilesTable_orig[i].bottomOffset != profilesTable[i].bottomOffset)
            settingsDiff++;

        if(profilesTable_orig[i].leftOffset != profilesTable[i].leftOffset)
            settingsDiff++;

        if(profilesTable_orig[i].rightOffset != profilesTable[i].rightOffset)
            settingsDiff++;

        if(profilesTable_orig[i].TLled != profilesTable[i].TLled)
            settingsDiff++;

        if(profilesTable_orig[i].TRled != profilesTable[i].TRled)
            settingsDiff++;

        if(profilesTable_orig[i].irSensitivity != profilesTable[i].irSensitivity)
            settingsDiff++;

        if(profilesTable_orig[i].runMode != profilesTable[i].runMode)
            settingsDiff++;

        if(profilesTable_orig[i].layoutType != profilesTable[i].layoutType)
            settingsDiff++;

        if(profilesTable_orig[i].color != profilesTable[i].color)
            settingsDiff++;
    }

    if(settingsDiff) {
        ui->confirmButton->setText("Save and Send Settings");
        ui->confirmButton->setEnabled(true);
    } else {
        ui->confirmButton->setText("[Nothing To Save]");
        ui->confirmButton->setEnabled(false);
    }
}


void guiWindow::SyncSettings()
{
    for(int i = 0; i < OF_Const::boolTypesCount; i++)
        boolSettings_orig[i] = boolSettings[i];

    if(boolSettings_orig[OF_Const::customPins])
        inputsMap_orig = inputsMap;
    else for(int i = 0; i < inputsMap.size(); i++)
        inputsMap_orig[i] = -1;

    for(int i = 0; i < OF_Const::settingsTypesCount; i++)
        settingsTable_orig[i] = settingsTable[i];

    tinyUSBtable_orig.tinyUSBid = tinyUSBtable.tinyUSBid;
    tinyUSBtable_orig.tinyUSBname = tinyUSBtable.tinyUSBname;
    board.previousProfile = board.selectedProfile;

    for(uint8_t i = 0; i < PROFILES_COUNT; i++) {
        profilesTable_orig[i].irSensitivity = profilesTable[i].irSensitivity;
        profilesTable_orig[i].runMode = profilesTable[i].runMode;
        profilesTable_orig[i].layoutType = profilesTable[i].layoutType;
        profilesTable_orig[i].color = profilesTable[i].color;
        profilesTable_orig[i].profName = profilesTable[i].profName;
    }
    LabelsUpdate();
}


QString guiWindow::PrettifyName()
{
    QString name;

    if(!tinyUSBtable.tinyUSBname.isEmpty()) {
        name = tinyUSBtable.tinyUSBname;
    } else {
        name = "Unnamed Device";
    }

    // append name of board to gun name string.
    if(OF_Const::boardNames.contains(board.boardType.toStdString()))
         return name + " | " + OF_Const::boardNames[board.boardType.toStdString()];
    else return name + " | " + OF_Const::boardNames["generic"];
}


void guiWindow::PixelsDiff()
{
    if( settingsTable[OF_Const::customLEDcount]  == settingsTable_orig[OF_Const::customLEDcount]  &&
        settingsTable[OF_Const::customLEDstatic] == settingsTable_orig[OF_Const::customLEDstatic] &&
        settingsTable[OF_Const::customLEDcolor1] == settingsTable_orig[OF_Const::customLEDcolor1] &&
        settingsTable[OF_Const::customLEDcolor2] == settingsTable_orig[OF_Const::customLEDcolor2] &&
        settingsTable[OF_Const::customLEDcolor3] == settingsTable_orig[OF_Const::customLEDcolor3]) {
        ui->pixelChangeNotice->setVisible(false);
    } else {
        ui->pixelChangeNotice->setVisible(true);
    }
}


void guiWindow::on_confirmButton_clicked()
{
    QMessageBox messageBox(QMessageBox::Information, "Commit Confirmation", "Are these settings okay?", QMessageBox::Yes | QMessageBox::No);
    messageBox.setInformativeText("These settings will be committed to your lightgun. Is that okay?");

    if(messageBox.exec() == QMessageBox::Yes) {
        if(serialPort.isOpen()) {
            serialActive = true;
            aliveTimer->stop();
            // send a signal so the gun pauses its test outputs for the save op.
            serialPort.write("Xm");
            serialPort.waitForBytesWritten(1000);

            QProgressBar *statusProgressBar = new QProgressBar();
            ui->statusBar->addPermanentWidget(statusProgressBar);
            ui->tabWidget->setEnabled(false);
            ui->comPortSelector->setEnabled(false);
            ui->confirmButton->setEnabled(false);

            QStringList serialQueue;
            for(uint8_t i = 0; i < OF_Const::boolTypesCount; i++)
                serialQueue.append(QString("Xm.0.%1.%2").arg(i).arg(boolSettings[i]));

            if(boolSettings[OF_Const::customPins])
                for(uint8_t i = 0; i < inputsMap.count(); i++)
                    serialQueue.append(QString("Xm.1.%1.%2").arg(i).arg(inputsMap.value(i)));

            for(uint8_t i = 0; i < OF_Const::settingsTypesCount; i++)
                serialQueue.append(QString("Xm.2.%1.%2").arg(i).arg(settingsTable[i]));

            serialQueue.append(QString("Xm.3.0.%1").arg(tinyUSBtable.tinyUSBid));
            if(!tinyUSBtable.tinyUSBname.isEmpty())
                serialQueue.append(QString("Xm.3.1.%1").arg(tinyUSBtable.tinyUSBname));

            for(uint8_t i = 0; i < 4; i++) {
                serialQueue.append(QString("Xm.P.i.%1.%2").arg(i).arg(profilesTable[i].irSensitivity));
                serialQueue.append(QString("Xm.P.r.%1.%2").arg(i).arg(profilesTable[i].runMode));
                serialQueue.append(QString("Xm.P.l.%1.%2").arg(i).arg(profilesTable[i].layoutType));
                serialQueue.append(QString("Xm.P.c.%1.%2").arg(i).arg(profilesTable[i].color));
                serialQueue.append(QString("Xm.P.n.%1.%2").arg(i).arg(profilesTable[i].profName));
            }
            serialQueue.append("XS");

            statusProgressBar->setRange(0, serialQueue.length()-1);
            bool success = true;

            // throw out whatever's in the buffer if there's anything there.
            while(!serialPort.atEnd()) {
                serialPort.readLine();
            }

            for(uint8_t i = 0; i < serialQueue.length(); i++) {
                serialPort.write(serialQueue[i].toLocal8Bit());
                serialPort.waitForBytesWritten(2000);
                if(serialPort.waitForReadyRead(2000)) {
                    QString buffer = serialPort.readLine();
                    if(buffer.contains("OK:") || buffer.contains("NOENT:")) {
                        statusProgressBar->setValue(statusProgressBar->value() + 1);
                    } else if(i == serialQueue.length() - 1 && buffer.contains("Saving preferences...")) {
                        for(uint8_t t = 0; t < 3; t++) {
                            if(serialPort.atEnd()) { serialPort.waitForReadyRead(2000); }
                            buffer = serialPort.readLine();
                            if(buffer.contains("Settings saved to")) {
                                success = true;
                                t = 3;
                            }
                        }
                        if(success) {
                            while(!serialPort.atEnd()) {
                                serialPort.readLine();
                            }
                        }
                    }
                }
            }

            ui->statusBar->removeWidget(statusProgressBar);
            delete statusProgressBar;
            ui->tabWidget->setEnabled(true);
            ui->comPortSelector->setEnabled(true);

            if(!success) { qDebug() << "Ah shit, it failed! What did you do, Seong?"; }
            else {
                statusBar()->showMessage("Sent settings successfully!", 5000);
                SyncSettings();
                PixelsDiff();
                DiffUpdate();
                ui->boardLabel->setText(PrettifyName());
            }

            serialActive = false;
            aliveTimer->start(ALIVE_TIMER);
            serialQueue.clear();

            if(!serialPort.atEnd()) {
                serialPort.readAll();
            }
        } else { qDebug() << "Wait, this port wasn't open to begin with!!! WTF SEONG!?!?"; }
    } else { statusBar()->showMessage("Save operation canceled.", 3000); }
}


void guiWindow::aliveTimer_timeout()
{
    if(serialPort.isOpen()) {
        serialPort.write(".");
        if(!serialPort.waitForBytesWritten(1)) {
            statusBar()->showMessage("Board hasn't responded to pulse; assuming it's been disconnected.");
            serialPort.close();
            ui->comPortSelector->setCurrentIndex(0);
        }
    }
}


void guiWindow::on_comPortSelector_currentIndexChanged(int index)
{
    // Clear stale states if any, and unmount old board if mounted.
    if(testMode) {
        testMode = false;
        ui->testView->setEnabled(false);
        ui->buttonsTestArea->setEnabled(true);
        ui->testBtn->setText("Enable IR Test Mode");
        ui->pinsTab->setEnabled(true);
        ui->settingsTab->setEnabled(true);
        ui->profilesTab->setEnabled(true);
        ui->feedbackTestsBox->setEnabled(true);
        ui->dangerZoneBox->setEnabled(true);
        serialActive = false;
    }
    if(serialPort.isOpen()) {
        serialActive = true;
        serialPort.write("XE");
        serialPort.waitForBytesWritten(2000);
        serialPort.waitForReadyRead(2000);
        serialPort.readAll();
        serialPort.close();
        serialActive = false;
    }

    if(index > 0) {
        printf("COM port set to %d\n", ui->comPortSelector->currentIndex());

        // try to init serial port
        // if returns false, it failed, so just turn the index back to initial.
        if(!SerialInit(index - 1)) {
            ui->comPortSelector->setCurrentIndex(0);
            aliveTimer->stop();
        // else, serial port is online! What do we got?
        } else {
            // Clears old board layout items
            if(pinBoxes[0] != nullptr) {
                for(uint8_t i = 0; i < 30; i++) {
                    delete pinBoxes[i];
                    delete padding[i];
                    delete pinLabel[i];
                }
            }

            if(PinsCenter != nullptr) {
                delete PinsCenter;
                delete PinsLeft;
                delete PinsRight;
                if(PinsCenterSub != nullptr)
                    delete PinsCenterSub;
                if(centerPic != nullptr)
                    delete centerPic;
            }

            PinsCenter = new QVBoxLayout();
            PinsCenterSub = new QGridLayout();
            PinsLeft = new QGridLayout();
            PinsRight = new QGridLayout();

            ui->PinsTopHalf->addLayout(PinsLeft);
            ui->PinsTopHalf->addLayout(PinsCenter);
            ui->PinsTopHalf->addLayout(PinsRight);

            for(uint8_t i = 0; i < 30; i++) {
                pinBoxes[i] = new QComboBox();
                pinBoxes[i]->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
                pinBoxes[i]->setProperty("slot", i);
                pinBoxes[i]->setProperty("prevMapping", OF_Const::btnUnmapped+1);
                connect(pinBoxes[i], SIGNAL(currentIndexChanged(int)), this, SLOT(pinBoxes_currentIndexChanged(int)));

                padding[i] = new QWidget();
                padding[i]->setMinimumHeight(25);

                // I2C channel coloring
                if(i & 0b0000010)
                    pinLabel[i] = new QLabel(QString("<font color=#FF8800>«GPIO%1»</font>").arg(i));
                else pinLabel[i] = new QLabel(QString("<font color=#0099FF>«GPIO%1»</font>").arg(i));

                pinLabel[i]->setEnabled(false);
                pinLabel[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                pinLabel[i]->setToolTip(QString("GPIO Pin number %1\n\nBlue pin numbers are members of I2C0\nOrange are members of I2C1").arg(i));
            }

            aliveTimer->start(ALIVE_TIMER);
            ui->versionLabel->setText(QString("v%1 - \"%2\"").arg(board.versionNumber, board.versionCodename));
            BoxesFill();
            LabelsUpdate();

            ui->boardLabel->setText(PrettifyName());

            // Drawing the actual board view page by referencing the board maps data from OpenFIREshared.h
            if(OF_Const::boardsBoxPositions.contains(board.boardType.toStdString())) {
                centerPic = new QSvgWidget(":/boardPics/" + board.boardType);
                QSvgRenderer *picRenderer = centerPic->renderer();
                picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);
                PinsCenter->addWidget(centerPic);

                for(int i = 0; i < PINS_COUNT; i++) {
                    if(OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] & OF_Const::posLeft) {
                        PinsLeft->addWidget(pinBoxes[i],
                                            OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] ^ OF_Const::posLeft,
                                            0);
                        PinsLeft->addWidget(pinLabel[i],
                                            OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] ^ OF_Const::posLeft,
                                            1);
                    } else if(OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] & OF_Const::posRight) {
                        PinsRight->addWidget(pinBoxes[i],
                                            OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] ^ OF_Const::posRight,
                                            1);
                        PinsRight->addWidget(pinLabel[i],
                                            OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] ^ OF_Const::posRight,
                                            0);
                    } else if(OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] & OF_Const::posMiddle) {
                        if(PinsCenterSub->isEmpty())
                            PinsCenter->addLayout(PinsCenterSub);

                        PinsCenterSub->addWidget(pinBoxes[i],
                                                 1,
                                                 OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] ^ OF_Const::posMiddle);
                        PinsCenterSub->addWidget(pinLabel[i],
                                                 0,
                                                 OF_Const::boardsBoxPositions.value(board.boardType.toStdString()).pin[i] ^ OF_Const::posMiddle);
                    }
                }
            } else {
                centerPic = new QSvgWidget(":/boardPics/generic");
                QSvgRenderer *picRenderer = centerPic->renderer();
                picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);

                for(int i = 0; i < PINS_COUNT; i++) {
                    if(OF_Const::boardsBoxPositions.value("generic").pin[i] & OF_Const::posLeft) {
                        PinsLeft->addWidget(pinBoxes[i],
                                            OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posLeft,
                                            0);
                        PinsLeft->addWidget(pinLabel[i],
                                            OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posLeft,
                                            1);
                    } else if(OF_Const::boardsBoxPositions.value("generic").pin[i] & OF_Const::posRight) {
                        PinsRight->addWidget(pinBoxes[i],
                                             OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posRight,
                                             1);
                        PinsRight->addWidget(pinLabel[i],
                                            OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posRight,
                                            0);
                    } else if(OF_Const::boardsBoxPositions.value("generic").pin[i] & OF_Const::posMiddle) {
                        if(PinsCenter->isEmpty())
                            PinsCenter->addLayout(PinsCenterSub);

                        PinsCenterSub->addWidget(pinBoxes[i],
                                                 1,
                                                 OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posMiddle);
                        PinsCenterSub->addWidget(pinLabel[i],
                                                 0,
                                                 OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posMiddle);
                    }
                }
            }

            int prevPadCount;
            for(int i = 1, padCount = 0; i < PinsLeft->rowCount(); i++) {
                if(PinsLeft->itemAtPosition(i, 0) == nullptr) {
                    PinsLeft->addWidget(padding[padCount], i, 0);
                    padCount++;
                    prevPadCount = padCount;
                }
            }
            for(int i = 1, padCount = prevPadCount; i < PinsRight->rowCount(); i++) {
                if(PinsRight->itemAtPosition(i, 0) == nullptr) {
                    PinsRight->addWidget(padding[padCount], i, 0);
                    padCount++;
                }
            }

            centerPic->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

            ui->tabWidget->setEnabled(true);
            ui->customPinsEnabled->setChecked(boolSettings[OF_Const::customPins]);

            if(inputsMap.value(OF_Const::rumblePin) >= 0)
                 ui->rumbleToggle->setEnabled(true),  ui->rumbleFFToggle->setEnabled(true);
            else ui->rumbleToggle->setEnabled(false), ui->rumbleFFToggle->setEnabled(false);
            ui->rumbleToggle->setChecked(boolSettings[OF_Const::rumble]);

            if(inputsMap.value(OF_Const::solenoidPin) >= 0)
                 ui->solenoidToggle->setEnabled(true);
            else ui->solenoidToggle->setEnabled(false);
            ui->solenoidToggle->setChecked(boolSettings[OF_Const::solenoid]);

            if((boolSettings[OF_Const::rumble] && boolSettings[OF_Const::rumbleFF]) || boolSettings[OF_Const::solenoid])
                ui->autofireToggle->setEnabled(true);
            else ui->autofireToggle->setEnabled(false);
            ui->autofireToggle->setChecked(boolSettings[OF_Const::autofire]);

            ui->simplePauseToggle->setChecked(boolSettings[OF_Const::simplePause]);
            ui->holdToPauseToggle->setChecked(boolSettings[OF_Const::holdToPause]);

            if(inputsMap.value(OF_Const::ledR) >= 0 && inputsMap.value(OF_Const::ledG) >= 0 && inputsMap.value(OF_Const::ledB) >= 0)
                 ui->commonAnodeToggle->setEnabled(true);
            else ui->commonAnodeToggle->setEnabled(false);
            ui->commonAnodeToggle->setChecked(boolSettings[OF_Const::commonAnode]);

            ui->lowButtonsToggle->setChecked(boolSettings[OF_Const::lowButtonsMode]);
            ui->rumbleFFToggle->setChecked(boolSettings[OF_Const::rumbleFF]);
            ui->rumbleIntensityBox->setEnabled(boolSettings[OF_Const::rumble]),          ui->rumbleIntensityBox->setValue(settingsTable[OF_Const::rumbleStrength]);
            ui->rumbleLengthBox->setEnabled(boolSettings[OF_Const::rumble]),             ui->rumbleLengthBox->setValue(settingsTable[OF_Const::rumbleInterval]);
            ui->holdToPauseLengthBox->setEnabled(boolSettings[OF_Const::holdToPause]),   ui->holdToPauseLengthBox->setValue(settingsTable[OF_Const::holdToPauseLength]);
            ui->solenoidNormalIntervalBox->setEnabled(boolSettings[OF_Const::solenoid]), ui->solenoidNormalIntervalBox->setValue(settingsTable[OF_Const::solenoidNormalInterval]);
            ui->solenoidFastIntervalBox->setEnabled(boolSettings[OF_Const::solenoid]),   ui->solenoidFastIntervalBox->setValue(settingsTable[OF_Const::solenoidFastInterval]);
            ui->solenoidHoldLengthBox->setEnabled(boolSettings[OF_Const::solenoid]),     ui->solenoidHoldLengthBox->setValue(settingsTable[OF_Const::solenoidHoldLength]);
            ui->autofireWaitFactorBox->setEnabled(boolSettings[OF_Const::autofire]),     ui->autofireWaitFactorBox->setValue(settingsTable[OF_Const::autofireWaitFactor]);

            ui->productIdInput->setText(tinyUSBtable.tinyUSBid);
            ui->productNameInput->setText(tinyUSBtable.tinyUSBname);

            if(inputsMap.value(OF_Const::neoPixel) >= 0)
                 ui->neopixelGroupBox->setEnabled(true);
            else ui->neopixelGroupBox->setEnabled(false);
            ui->neopixelStrandLengthBox->setValue(settingsTable[OF_Const::customLEDcount]);
            ui->customLEDstaticSpinbox->setValue(settingsTable[OF_Const::customLEDstatic]);
            ui->customLEDstaticBtn1->setStyleSheet(QString("background-color: #%1").arg(settingsTable[OF_Const::customLEDcolor1], 6, 16, QLatin1Char('0')));
            ui->customLEDstaticBtn2->setStyleSheet(QString("background-color: #%1").arg(settingsTable[OF_Const::customLEDcolor2], 6, 16, QLatin1Char('0')));
            ui->customLEDstaticBtn3->setStyleSheet(QString("background-color: #%1").arg(settingsTable[OF_Const::customLEDcolor3], 6, 16, QLatin1Char('0')));

            switch(tinyUSBtable.tinyUSBid.toInt()) {
            case 1:
                ui->tUSB_p1->setChecked(true);
                ui->tUSBLayoutAdvanced->setVisible(false);
                ui->tUSBLayoutSimple->setVisible(true);
                ui->tinyUSBLayoutToggle->setChecked(false);
                break;
            case 2:
                ui->tUSB_p2->setChecked(true);
                ui->tUSBLayoutAdvanced->setVisible(false);
                ui->tUSBLayoutSimple->setVisible(true);
                ui->tinyUSBLayoutToggle->setChecked(false);
                break;
            case 3:
                ui->tUSB_p3->setChecked(true);
                ui->tUSBLayoutAdvanced->setVisible(false);
                ui->tUSBLayoutSimple->setVisible(true);
                ui->tinyUSBLayoutToggle->setChecked(false);
                break;
            case 4:
                ui->tUSB_p4->setChecked(true);
                ui->tUSBLayoutAdvanced->setVisible(false);
                ui->tUSBLayoutSimple->setVisible(true);
                ui->tinyUSBLayoutToggle->setChecked(false);
                break;
            default:
                ui->tUSB_p1->setChecked(false);
                ui->tUSB_p2->setChecked(false);
                ui->tUSB_p3->setChecked(false);
                ui->tUSB_p4->setChecked(false);
                ui->tUSBLayoutSimple->setVisible(false);
                ui->tUSBLayoutAdvanced->setVisible(true);
                ui->tinyUSBLayoutToggle->setChecked(true);
                break;
            }
        }
    } else {
        ui->boardLabel->clear();
        ui->versionLabel->clear();

        if(serialPort.isOpen()) {
            serialActive = true;
            serialPort.write("XE");
            serialPort.waitForBytesWritten(2000);
            serialPort.waitForReadyRead(2000);
            serialPort.readAll();
            serialPort.close();
            testLabel[14]->setStyleSheet("");
            testLabel[15]->setStyleSheet("");
            if(testMode) {
                testMode = false;
                ui->testView->setEnabled(false);
                ui->buttonsTestArea->setEnabled(true);
                ui->testBtn->setText("Enable IR Test Mode");
                ui->pinsTab->setEnabled(true);
                ui->settingsTab->setEnabled(true);
                ui->profilesTab->setEnabled(true);
                ui->feedbackTestsBox->setEnabled(true);
                ui->dangerZoneBox->setEnabled(true);
                serialActive = false;
            }
            serialActive = false;
        }
        qDebug() << "COM port disabled!";
        aliveTimer->stop();
        ui->tabWidget->setEnabled(false);
    }
}

void guiWindow::BoxesFill()
{
    // update box types
    for(uint8_t i = 0; i < PINS_COUNT; i++) {
        pinBoxes[i]->addItems(OF_Const::valuesNameList);
        // clear out analog options for digital pins (< GPIO26)
        // (entrylist is offset by one, as "Unmapped" == -1 in our enum)
        if(i < 26) {
            pinBoxes[i]->removeItem(OF_Const::tempPin+1);
            pinBoxes[i]->removeItem(OF_Const::analogY+1);
            pinBoxes[i]->removeItem(OF_Const::analogX+1);
        }
        // filter out SCL/SDA if possible.
        // TODO: don't add separators, just disable them instead. see Nero code
        if(i & 1) {
            pinBoxes[i]->removeItem(OF_Const::camSDA+1);
            pinBoxes[i]->insertSeparator(OF_Const::camSDA+1);
            pinBoxes[i]->removeItem(OF_Const::periphSDA+1);
            pinBoxes[i]->insertSeparator(OF_Const::periphSDA+1);
        } else {
            pinBoxes[i]->removeItem(OF_Const::camSCL+1);
            pinBoxes[i]->insertSeparator(OF_Const::camSCL+1);
            pinBoxes[i]->removeItem(OF_Const::periphSCL+1);
            pinBoxes[i]->insertSeparator(OF_Const::periphSCL+1);
        }
    }

    ui->presetsBox->clear();

    if(OF_Const::boardsAltPresets.count(board.boardType.toStdString())) {
        ui->presetsBox->setHidden(false);
        ui->presetsBox->setEnabled(true);
        QList<OF_Const::boardAltPresetsMap_t> altPresets = OF_Const::boardsAltPresets.values(board.boardType.toStdString());
        for(auto &entry : altPresets)
            ui->presetsBox->addItem(entry.name);
    } else {
        ui->presetsBox->setEnabled(false);
        ui->presetsBox->setHidden(true);
    }
    BoxesUpdate();
}

// Only runs either on initial load or save
void guiWindow::LabelsUpdate()
{
    // because inputsMap uses pin no. starting from 0
    for(uint8_t i = 0; i < 16; i++) {
        if(i < 14) {
            if(inputsMap.value(i) >= 0) {
                testLabel[i]->setText(OF_Const::valuesNameList[i+1]);
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText(OF_Const::valuesNameList[i+1] + " (N/C)");
                testLabel[i]->setEnabled(false);
            }
        } else if(i == 14) {
            if(inputsMap.value(OF_Const::tempPin) >= 0) {
                testLabel[i]->setText("Temp Read...");
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText("Temp (N/C)");
                testLabel[i]->setEnabled(false);
            }
            testLabel[i]->setStyleSheet("");
        } else if(i == 15) {
            if(inputsMap.value(OF_Const::analogX) >=0 && inputsMap.value(OF_Const::analogY) >= 0) {
                testLabel[i]->setText("Analog");
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText("Analog (N/C)");
                testLabel[i]->setEnabled(false);
            }
            testLabel[i]->setStyleSheet("");
        }
    }
    if(inputsMap.value(OF_Const::ledR) >= 0) ui->redLedTestBtn->setEnabled(true);   else ui->redLedTestBtn->setEnabled(false);
    if(inputsMap.value(OF_Const::ledG) >= 0) ui->greenLedTestBtn->setEnabled(true); else ui->greenLedTestBtn->setEnabled(false);
    if(inputsMap.value(OF_Const::ledB) >= 0) ui->blueLedTestBtn->setEnabled(true);  else ui->blueLedTestBtn->setEnabled(false);
}

void guiWindow::pinBoxes_currentIndexChanged(int index)
{
    // using comboboxes' "slot" property to figure caller,
    // and "prevMapping" to get previous index, as this method immediately overwrites what it was mapped to.
    // always remember to sync the change to "prevMapping" property at the end of its logic path!

    if(index >= 0 && index < inputsMap.size()) {
        //printf("Requesting pinbox %d to set to %s\n", sender()->property("slot").toInt(), OF_Const::valuesNameList.at(index).toLocal8Bit().constData());
    } else printf("Oops! Seems like pinbox %d is trying to set itself to index %d, which is out of range!\n", sender()->property("slot").toInt(), index);

    // reset presets box, as it's no longer accurate for this layout
    if(ui->presetsBox->currentIndex() > -1)
        ui->presetsBox->setCurrentIndex(-1);

    // if it's being set to 0 (unmapped), unmap this pin without question.
    if(index <= 0) {
        if(sender()->property("prevMapping").toInt() > OF_Const::btnUnmapped+1) {
            inputsMap[sender()->property("prevMapping").toInt()-1] = OF_Const::btnUnmapped;
            sender()->setProperty("prevMapping", OF_Const::btnUnmapped+1);
        }

    // else, it's a function, so check for duplicates
    } else if(sender()->property("prevMapping").toInt() != index) {
        int8_t btnRequest = index - 1;

        // Remove whatever pin mapping that this function belonged to, if it was mapped
        if(inputsMap.value(btnRequest) > OF_Const::btnUnmapped)
            pinBoxes[inputsMap.value(btnRequest)]->setCurrentIndex(OF_Const::btnUnmapped+1);

        // unmap pinbox's previous function, if mapped to any
        if(sender()->property("prevMapping").toInt() > OF_Const::btnUnmapped+1)
            pinBoxes[inputsMap.value(sender()->property("prevMapping").toInt()-1)]->setCurrentIndex(OF_Const::btnUnmapped+1);

        // if function is I2C, check for other things
        if(btnRequest == OF_Const::camSDA) {
            // if it's mapped, check that cam clock pin isn't mapped to the opposite I2C channel
            if(inputsMap.value(OF_Const::camSCL) > OF_Const::btnUnmapped &&
               (sender()->property("slot").toInt() & 0b00000010) != (inputsMap.value(OF_Const::camSCL) & 0b00000010)) {
                // channels mismatched, unmap the other pin
                pinBoxes[inputsMap.value(OF_Const::camSCL)]->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera pins are not on the same I2C channel! Please check camera pin mappings.", 10000);
            // check that peripheral data isn't mapped to this I2C channel
            } else if(inputsMap.value(OF_Const::periphSDA) > OF_Const::btnUnmapped &&
                      (sender()->property("slot").toInt() & 0b00000010) == (inputsMap.value(OF_Const::periphSDA) & 0b00000010)) {
                // channels matched, unmap peripheral data
                pinBoxes[inputsMap.value(OF_Const::periphSDA)]->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Peripheral Data.", 10000);
            }
        } else if(btnRequest == OF_Const::camSCL) {
            // if it's mapped, check that cam clock pin isn't mapped to the opposite I2C channel
            if(inputsMap.value(OF_Const::camSDA) > OF_Const::btnUnmapped &&
               (sender()->property("slot").toInt() & 0b00000010) != (inputsMap.value(OF_Const::camSDA) & 0b00000010)) {
                // channels mismatched, unmap the other pin
                pinBoxes[inputsMap.value(OF_Const::camSDA)]->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera pins are not on the same I2C channel! Please check camera pin mappings.", 10000);
            // check that peripheral clock isn't mapped to this I2C channel
            } else if(inputsMap.value(OF_Const::periphSCL) > OF_Const::btnUnmapped &&
                      (sender()->property("slot").toInt() & 0b00000010) == (inputsMap.value(OF_Const::periphSCL) & 0b00000010)) {
                // channels matched, unmap peripheral clock
                pinBoxes[inputsMap.value(OF_Const::periphSCL)]->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera and Peripheral Clock pins clashed! Please remap Peripheral Clock.", 10000);
            }
        } else if(btnRequest == OF_Const::periphSDA) {
            // if it's mapped, check that current peripheral clock pin isn't mapped to the opposite I2C channel
            if(inputsMap.value(OF_Const::periphSCL) > OF_Const::btnUnmapped &&
               (sender()->property("slot").toInt() & 0b00000010) != (inputsMap.value(OF_Const::periphSCL) & 0b00000010)) {
                // channels mismatched, unmap the other pin
                pinBoxes[inputsMap.value(OF_Const::periphSCL)]->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Peripheral pins are not on the same I2C channel! Please check peripheral pin mappings.", 10000);
            // check that cam data isn't mapped to this I2C channel
            } else if(inputsMap.value(OF_Const::camSDA) > OF_Const::btnUnmapped &&
                      (sender()->property("slot").toInt() & 0b00000010) == (inputsMap.value(OF_Const::camSDA) & 0b00000010)) {
                // channels matched, unmap cam data
                pinBoxes[inputsMap.value(OF_Const::camSDA)]->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Camera Data.", 10000);
            }
        } else if(btnRequest == OF_Const::periphSCL) {
            // if it's mapped, check that current peripheral data pin isn't mapped to the opposite I2C channel
            if(inputsMap.value(OF_Const::periphSDA) > OF_Const::btnUnmapped &&
               (sender()->property("slot").toInt() & 0b00000010) != (inputsMap.value(OF_Const::periphSDA) & 0b00000010)) {
                // channels mismatched, unmap the other pin
                pinBoxes[inputsMap.value(OF_Const::periphSDA)]->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Peripheral pins are not on the same I2C channel! Please check peripheral pin mappings.", 10000);
            // check that cam clock isn't mapped to this I2C channel
            } else if(inputsMap.value(OF_Const::camSCL) > OF_Const::btnUnmapped &&
                      (sender()->property("slot").toInt() & 0b00000010) == (inputsMap.value(OF_Const::camSCL) & 0b00000010)) {
                // channels matched, unmap cam clock
                pinBoxes[inputsMap.value(OF_Const::camSCL)]->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Camera Clock.", 10000);
            }
        }

        // Then map the thing, and sync this change to the property value
        inputsMap[btnRequest] = sender()->property("slot").toInt();
        sender()->setProperty("prevMapping", index);
    }

    // update settings panel to reflect pins map changes and prevent illegal values/combinations
    if(inputsMap.value(OF_Const::rumblePin) >= 0)
        ui->rumbleToggle->setEnabled(true), ui->rumbleFFToggle->setEnabled(true);
    else {
        ui->rumbleToggle->setChecked(false),   ui->rumbleToggle->setEnabled(false),
        ui->rumbleFFToggle->setChecked(false), ui->rumbleFFToggle->setEnabled(false);
    }

    if(inputsMap.value(OF_Const::solenoidPin) >= 0)
        ui->solenoidToggle->setEnabled(true);
    else ui->solenoidToggle->setChecked(false), ui->solenoidToggle->setEnabled(false);

    if(inputsMap.value(OF_Const::neoPixel) >= 0)
        ui->neopixelGroupBox->setEnabled(true);
    else ui->neopixelGroupBox->setEnabled(false);

    if(inputsMap.value(OF_Const::ledR) >= 0 && inputsMap.value(OF_Const::ledG) >= 0 && inputsMap.value(OF_Const::ledB) >= 0)
        ui->commonAnodeToggle->setEnabled(true);
    else ui->commonAnodeToggle->setEnabled(false);

    DiffUpdate();
}

void guiWindow::irBoxes_activated(int index)
{
    // Demultiplexing to figure out which "pin" this combobox that's calling correlates to.
    uint8_t slot;
    QObject* obj = sender();
    for(uint8_t i = 0;;i++) {
        if(obj == irSens[i]) {
            slot = i;
            break;
        }
    }

    profilesTable[slot].irSensitivity = index;

    DiffUpdate();
}


void guiWindow::runModeBoxes_activated(int index)
{
    // Demultiplexing to figure out which "pin" this combobox that's calling correlates to.
    uint8_t slot;
    QObject* obj = sender();
    for(uint8_t i = 0;;i++) {
        if(obj == runMode[i]) {
            slot = i;
            break;
        }
    }

    profilesTable[slot].runMode = index;

    DiffUpdate();
}


void guiWindow::renameBoxes_clicked()
{
    // TODO: limit character length in the text dialog - for now, just use up to 15 characters.
    QString newLabel = QInputDialog::getText(this, "Input Name", QString("Set name for profile %1").arg(sender()->property("slot").toInt()+1));
    if(!newLabel.isEmpty()) {
        selectedProfile[sender()->property("slot").toInt()]->setText(newLabel.left(15));
        profilesTable[sender()->property("slot").toInt()].profName = newLabel.left(15).toLocal8Bit();
    }
    DiffUpdate();
}


void guiWindow::colorBoxes_clicked()
{
    QColor colorPick = QColorDialog::getColor(profilesTable[sender()->property("slot").toInt()].color);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        profilesTable[sender()->property("slot").toInt()].color = packedColor;
        color[sender()->property("slot").toInt()]->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));
        DiffUpdate();
    }
}


void guiWindow::layoutBoxes_activated(int arg1)
{
    // Demultiplexing to figure out which box we're using.
    uint8_t slot;
    QObject* obj = sender();
    for(uint8_t i = 0;;i++) {
        if(obj == layoutMode[i]) {
            slot = i;
            break;
        }
    }

    profilesTable[slot].layoutType = arg1;
    DiffUpdate();
}


void guiWindow::on_customPinsEnabled_stateChanged(int arg1)
{
    boolSettings[OF_Const::customPins] = arg1;
    BoxesUpdate();

    if(inputsMap.value(OF_Const::solenoidPin) > OF_Const::btnUnmapped) {
        ui->solenoidToggle->setEnabled(true);
    } else {
        ui->solenoidToggle->setEnabled(false);
        ui->solenoidToggle->setChecked(false);
    }

    if(inputsMap.value(OF_Const::rumblePin) > OF_Const::btnUnmapped) {
        ui->rumbleToggle->setEnabled(true);
        ui->rumbleFFToggle->setEnabled(true);
    } else {
        ui->rumbleToggle->setEnabled(false);
        ui->rumbleToggle->setChecked(false);
        ui->rumbleFFToggle->setEnabled(false);
        ui->rumbleFFToggle->setChecked(false);
    }

    DiffUpdate();
}


void guiWindow::on_presetsBox_currentIndexChanged(int index)
{
    if(index > -1) {
        // presets are inherently custom layouts
        if(!ui->customPinsEnabled->isChecked())
            ui->customPinsEnabled->setChecked(true);

        // clear pinBoxes to be safe
        for(uint8_t i = 0; i < PINS_COUNT; i++)
            pinBoxes[i]->setCurrentIndex(OF_Const::btnUnmapped+1);

        // set pinboxes to alt preset values (and let the index changed signal handle the rest)
        QList<OF_Const::boardAltPresetsMap_t> altPresets = OF_Const::boardsAltPresets.values(board.boardType.toStdString());
        for(int i = 0; i < PINS_COUNT; i++)
            pinBoxes[i]->setCurrentIndex(altPresets.at(index).pin[i]+1);

        DiffUpdate();
    }
}


void guiWindow::on_rumbleToggle_stateChanged(int arg1)
{
    boolSettings[OF_Const::rumble] = arg1;
    if(!arg1) {
        ui->rumbleFFToggle->setChecked(false);
        ui->rumbleFFToggle->setEnabled(false);
        ui->rumbleIntensityBox->setEnabled(false);
        ui->rumbleLengthBox->setEnabled(false);
        ui->rumbleTestBtn->setEnabled(false);
    } else {
        ui->rumbleFFToggle->setEnabled(true);
        ui->rumbleIntensityBox->setEnabled(true);
        ui->rumbleLengthBox->setEnabled(true);
        ui->rumbleTestBtn->setEnabled(true);
    }
    if(!(arg1 && boolSettings[OF_Const::rumbleFF]) && !boolSettings[OF_Const::solenoid]) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_solenoidToggle_stateChanged(int arg1)
{
    boolSettings[OF_Const::solenoid] = arg1;
    if(arg1) {
        ui->rumbleFFToggle->setChecked(false);
        ui->solenoidNormalIntervalBox->setEnabled(true);
        ui->solenoidFastIntervalBox->setEnabled(true);
        ui->solenoidHoldLengthBox->setEnabled(true);
        ui->solenoidTestBtn->setEnabled(true);
    } else {
        ui->solenoidNormalIntervalBox->setEnabled(false);
        ui->solenoidFastIntervalBox->setEnabled(false);
        ui->solenoidHoldLengthBox->setEnabled(false);
        ui->solenoidTestBtn->setEnabled(false);
    }
    if(!arg1 && !(boolSettings[OF_Const::rumble] && boolSettings[OF_Const::rumbleFF])) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_autofireToggle_stateChanged(int arg1)
{
    boolSettings[OF_Const::autofire] = arg1;
    if(arg1) { ui->autofireWaitFactorBox->setEnabled(true); } else { ui->autofireWaitFactorBox->setEnabled(false); }
    DiffUpdate();
}


void guiWindow::on_simplePauseToggle_stateChanged(int arg1)
{
    boolSettings[OF_Const::simplePause] = arg1;
    DiffUpdate();
}


void guiWindow::on_holdToPauseToggle_stateChanged(int arg1)
{
    boolSettings[OF_Const::holdToPause] = arg1;
    if(arg1) { ui->holdToPauseLengthBox->setEnabled(true); }
    else { ui->holdToPauseLengthBox->setEnabled(false); }
    DiffUpdate();
}


void guiWindow::on_commonAnodeToggle_stateChanged(int arg1)
{
    boolSettings[OF_Const::commonAnode] = arg1;
    DiffUpdate();
}


void guiWindow::on_lowButtonsToggle_stateChanged(int arg1)
{
    boolSettings[OF_Const::lowButtonsMode] = arg1;
    DiffUpdate();
}


void guiWindow::on_rumbleFFToggle_stateChanged(int arg1)
{
    boolSettings[OF_Const::rumbleFF] = arg1;
    if(arg1) { ui->solenoidToggle->setChecked(false); }
    if(!(arg1 && boolSettings[OF_Const::rumble]) && !boolSettings[OF_Const::solenoid]) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_rumbleIntensityBox_valueChanged(int arg1)
{
    settingsTable[OF_Const::rumbleStrength] = arg1;
    DiffUpdate();
}


void guiWindow::on_rumbleLengthBox_valueChanged(int arg1)
{
    settingsTable[OF_Const::rumbleInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_holdToPauseLengthBox_valueChanged(int arg1)
{
    settingsTable[OF_Const::holdToPauseLength] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidNormalIntervalBox_valueChanged(int arg1)
{
    settingsTable[OF_Const::solenoidNormalInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidFastIntervalBox_valueChanged(int arg1)
{
    settingsTable[OF_Const::solenoidFastInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidHoldLengthBox_valueChanged(int arg1)
{
    settingsTable[OF_Const::solenoidHoldLength] = arg1;
    DiffUpdate();
}


void guiWindow::on_autofireWaitFactorBox_valueChanged(int arg1)
{
    settingsTable[OF_Const::autofireWaitFactor] = arg1;
    DiffUpdate();
}

// decimal-to-hex conversion
void guiWindow::on_productIdInput_textChanged(const QString &arg1)
{
    qint32  iTest = arg1.toInt(NULL, 10);
    QString hex;

    if(iTest >= INT8_MIN && iTest <= INT8_MAX){
            hex = QString("%1").arg(iTest & 0xFF, 2, 16).simplified();
    } else if(iTest >= INT16_MIN && iTest <= INT16_MAX){
            hex = QString("%1").arg(iTest & 0xFFFF, 4, 16).simplified();
    } else {
            hex = QString("%1").arg(iTest, 8, 16).simplified();
    }
    ui->productIdConverted->setText(QString("0x%1").arg(hex));
}


void guiWindow::on_tUSB_p1_toggled(bool checked)
{
    if(checked) {
        tinyUSBtable.tinyUSBid = "1";
        tinyUSBtable.tinyUSBname = "FIRECon P1";
        ui->productIdInput->setText(tinyUSBtable.tinyUSBid);
        ui->productNameInput->setText(tinyUSBtable.tinyUSBname);
        DiffUpdate();
    }
}


void guiWindow::on_tUSB_p2_toggled(bool checked)
{
    if(checked) {
        tinyUSBtable.tinyUSBid = "2";
        tinyUSBtable.tinyUSBname = "FIRECon P2";
        ui->productIdInput->setText(tinyUSBtable.tinyUSBid);
        ui->productNameInput->setText(tinyUSBtable.tinyUSBname);
        DiffUpdate();
    }
}


void guiWindow::on_tUSB_p3_toggled(bool checked)
{
    if(checked) {
        tinyUSBtable.tinyUSBid = "3";
        tinyUSBtable.tinyUSBname = "FIRECon P3";
        ui->productIdInput->setText(tinyUSBtable.tinyUSBid);
        ui->productNameInput->setText(tinyUSBtable.tinyUSBname);
        DiffUpdate();
    }
}


void guiWindow::on_tUSB_p4_toggled(bool checked)
{
    if(checked) {
        tinyUSBtable.tinyUSBid = "4";
        tinyUSBtable.tinyUSBname = "FIRECon P4";
        ui->productIdInput->setText(tinyUSBtable.tinyUSBid);
        ui->productNameInput->setText(tinyUSBtable.tinyUSBname);
        DiffUpdate();
    }
}


void guiWindow::on_productIdInput_textEdited(const QString &arg1)
{
    tinyUSBtable.tinyUSBid = arg1;
    if(ui->productNameInput->text().isEmpty()) {
        switch(tinyUSBtable.tinyUSBid.toInt()) {
        case 1:
            ui->tUSB_p1->setChecked(true);
            break;
        case 2:
            ui->tUSB_p2->setChecked(true);
            break;
        case 3:
            ui->tUSB_p3->setChecked(true);
            break;
        case 4:
            ui->tUSB_p4->setChecked(true);
            break;
        default:
            ui->tUSB_p1->setChecked(false);
            ui->tUSB_p2->setChecked(false);
            ui->tUSB_p3->setChecked(false);
            ui->tUSB_p4->setChecked(false);
            break;
        }
    }

    DiffUpdate();
}


void guiWindow::on_productNameInput_textEdited(const QString &arg1)
{
    // TODO: there should be a way of using .toLocal8Bit() and checking if it's undefined,
    // as that indicates a character exceeds the normal char size, therefore
    // reset the lineEdit's text and don't change. But for now, weh.
    tinyUSBtable.tinyUSBname = arg1;
    DiffUpdate();
}


void guiWindow::on_tinyUSBLayoutToggle_stateChanged(int arg1)
{
    if(arg1) {
        ui->tUSBLayoutSimple->setVisible(false);
        ui->tUSBLayoutAdvanced->setVisible(true);
    } else {
        ui->tUSBLayoutAdvanced->setVisible(false);
        ui->tUSBLayoutSimple->setVisible(true);
    }
}


void guiWindow::selectedProfile_isChecked(bool isChecked)
{
    // apparently we get two signals at once? So just filter for the on.
    if(isChecked && !serialActive) {
        // Demultiplexing to figure out which "pin" this combobox that's calling correlates to.
        uint8_t slot;
        QObject* obj = sender();
        for(uint8_t i = 0;;i++) {
            if(obj == selectedProfile[i]) {
                slot = i;
                break;
            }
        }
        if(slot != board.selectedProfile) {
            serialPort.write(QString("XC%1").arg(slot+1).toLocal8Bit());
            board.selectedProfile = slot;
            DiffUpdate();
        }
    }
}


void guiWindow::on_neopixelStrandLengthBox_valueChanged(int arg1)
{
    settingsTable[OF_Const::customLEDcount] = arg1;
    if(arg1 < settingsTable[OF_Const::customLEDstatic]) {
        ui->customLEDstaticSpinbox->setValue(arg1);
    }

    // show NeoPixel notice if values are updated
    PixelsDiff();

    DiffUpdate();
}


void guiWindow::on_customLEDstaticSpinbox_valueChanged(int arg1)
{
    if(arg1 > settingsTable[OF_Const::customLEDcount]) { ui->customLEDstaticSpinbox->setValue(settingsTable[OF_Const::customLEDcount]); }
    else { settingsTable[OF_Const::customLEDstatic] = arg1; }
    if(OF_Const::customLEDstatic) {
        switch(settingsTable[OF_Const::customLEDstatic]) {
        case 1:
            ui->customLEDstaticBtn1->setEnabled(true);
            ui->customLEDstaticBtn2->setEnabled(false);
            ui->customLEDstaticBtn3->setEnabled(false);
            break;
        case 2:
            ui->customLEDstaticBtn1->setEnabled(true);
            ui->customLEDstaticBtn2->setEnabled(true);
            ui->customLEDstaticBtn3->setEnabled(false);
            break;
        case 3:
            ui->customLEDstaticBtn1->setEnabled(true);
            ui->customLEDstaticBtn2->setEnabled(true);
            ui->customLEDstaticBtn3->setEnabled(true);
            break;
        default:
            ui->customLEDstaticBtn1->setEnabled(false);
            ui->customLEDstaticBtn2->setEnabled(false);
            ui->customLEDstaticBtn3->setEnabled(false);
            break;
        }
    }

    // show NeoPixel notice if values are updated
    PixelsDiff();

    DiffUpdate();
}


void guiWindow::on_customLEDstaticBtn1_clicked()
{
    QColor colorPick = QColorDialog::getColor(settingsTable[OF_Const::customLEDcolor1]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        settingsTable[OF_Const::customLEDcolor1] = packedColor;
        ui->customLEDstaticBtn1->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}


void guiWindow::on_customLEDstaticBtn2_clicked()
{
    QColor colorPick = QColorDialog::getColor(settingsTable[OF_Const::customLEDcolor2]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        settingsTable[OF_Const::customLEDcolor2] = packedColor;
        ui->customLEDstaticBtn2->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}


void guiWindow::on_customLEDstaticBtn3_clicked()
{
    QColor colorPick = QColorDialog::getColor(settingsTable[OF_Const::customLEDcolor3]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        settingsTable[OF_Const::customLEDcolor3] = packedColor;
        ui->customLEDstaticBtn3->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}

// TODO: cali should use a fullscreen window depicting target graphics w/ hidden cursor. This should be its own method and activated when "Cali:" is detected in the serial stream.
// TODO TODO: move this to appcali subwindow
void guiWindow::on_calib1Btn_clicked()
{
    serialPort.write("XC1C");
    if(serialPort.waitForBytesWritten(1000))
        QMessageBox::information(this, "Calibrating Profile 1",
                                       "Aim the gun at the cursor in the center of the display and pull the trigger, then shoot at the four edges of the display that the mouse moves to.\n"
                                       "You can exit without saving changes by pressing either Button A/B/C.\n\n"
                                       "After the final center target, verify that the new calibration is to your liking; press the trigger to confirm, Button A/B to restart calibration, or Button C to exit calibration without any changes.");
}


void guiWindow::on_calib2Btn_clicked()
{
    serialPort.write("XC2C");
    if(serialPort.waitForBytesWritten(1000))
        QMessageBox::information(this,  "Calibrating Profile 2",
                                        "Aim the gun at the cursor in the center of the display and pull the trigger, then shoot at the four edges of the display that the mouse moves to.\n"
                                        "You can exit without saving changes by pressing either Button A/B/C.\n\n"
                                        "After the final center target, verify that the new calibration is to your liking; press the trigger to confirm, Button A/B to restart calibration, or Button C to exit calibration without any changes.");
}


void guiWindow::on_calib3Btn_clicked()
{
    serialPort.write("XC3C");
    if(serialPort.waitForBytesWritten(1000))
        QMessageBox::information(this,  "Calibrating Profile 3",
                                        "Aim the gun at the cursor in the center of the display and pull the trigger, then shoot at the four edges of the display that the mouse moves to.\n"
                                        "You can exit without saving changes by pressing either Button A/B/C.\n\n"
                                        "After the final center target, verify that the new calibration is to your liking; press the trigger to confirm, Button A/B to restart calibration, or Button C to exit calibration without any changes.");
}


void guiWindow::on_calib4Btn_clicked()
{
    serialPort.write("XC4C");
    if(serialPort.waitForBytesWritten(1000))
        QMessageBox::information(this,  "Calibrating Profile 4",
                                        "Aim the gun at the cursor in the center of the display and pull the trigger, then shoot at the four edges of the display that the mouse moves to.\n"
                                        "You can exit without saving changes by pressing either Button A/B/C.\n\n"
                                        "After the final center target, verify that the new calibration is to your liking; press the trigger to confirm, Button A/B to restart calibration, or Button C to exit calibration without any changes.");
}

// WARNING: make sure "serialActive" is set ON for important operations, or this will eat the fucker
// TODO: move to appserial
void guiWindow::serialPort_readyRead()
{
    if(!serialActive) {
        while(!serialPort.atEnd()) {
            QString idleBuffer = serialPort.readLine();

            if(idleBuffer.contains("Pressed:")) {
                testLabel[idleBuffer.trimmed().rightRef(2).toInt()-1]->setStyleSheet("background-color: #FF0000; font: bold");
            } else if(idleBuffer.contains("Released:")) {
                testLabel[idleBuffer.trimmed().rightRef(2).toInt()-1]->setStyleSheet("");
            } else if(idleBuffer.contains("Temperature:")) {
                uint8_t temp = idleBuffer.trimmed().rightRef(2).toInt();

                testLabel[14]->setText(QString("Temp: %1°C").arg(temp));

                if(temp > tempShutoff) {        testLabel[14]->setStyleSheet("color: white;      background-color: #FF0000; font: bold"); }
                else if(temp > tempWarning) {   testLabel[14]->setStyleSheet("color: light-gray; background-color: #EABD2B; font: bold"); }
                else {                          testLabel[14]->setStyleSheet("color: black;      background-color: #11D00A; font: bold"); }
            } else if(idleBuffer.contains("Analog:")) {
                // TODO: perhaps we should be using a small box area with a glyph depicting the aStick's coords instead of only showing cardinal directionality?
                uint8_t analogDir = idleBuffer.trimmed().rightRef(1).toInt();
                if(analogDir) {
                    switch(analogDir) {
                        case 1: testLabel[15]->setText("Analog 🡹"); break;
                        case 2: testLabel[15]->setText("Analog 🡼"); break;
                        case 3: testLabel[15]->setText("Analog 🡸"); break;
                        case 4: testLabel[15]->setText("Analog 🡿"); break;
                        case 5: testLabel[15]->setText("Analog 🡻"); break;
                        case 6: testLabel[15]->setText("Analog 🡾"); break;
                        case 7: testLabel[15]->setText("Analog 🡺"); break;
                        case 8: testLabel[15]->setText("Analog 🡽"); break;
                    }
                    testLabel[15]->setStyleSheet("background-color: #FF0000; font: bold");
                } else {
                    testLabel[15]->setText("Analog");
                    testLabel[15]->setStyleSheet("");
                }
            } else if(idleBuffer.contains("Profile: ")) {
                uint8_t selection = idleBuffer.trimmed().rightRef(1).toInt();

                if(selection != board.selectedProfile) {
                    board.selectedProfile = selection;
                    selectedProfile[selection]->setChecked(true);
                }

                DiffUpdate();

            } else if(idleBuffer.contains("UpdatedProf: ")) {
                uint8_t selection = idleBuffer.trimmed().rightRef(1).toInt();

                if(selection != board.selectedProfile) {
                    selectedProfile[selection]->setChecked(true);
                }

                board.selectedProfile = selection;

                serialPort.waitForReadyRead(2000);
                topOffset[selection]->setText(serialPort.readLine().trimmed());
                profilesTable[selection].topOffset = topOffset[selection]->text().toInt();
                serialPort.waitForReadyRead(2000);
                bottomOffset[selection]->setText(serialPort.readLine().trimmed());
                profilesTable[selection].bottomOffset = bottomOffset[selection]->text().toInt();
                serialPort.waitForReadyRead(2000);
                leftOffset[selection]->setText(serialPort.readLine().trimmed());
                profilesTable[selection].leftOffset = leftOffset[selection]->text().toInt();
                serialPort.waitForReadyRead(2000);
                rightOffset[selection]->setText(serialPort.readLine().trimmed());
                profilesTable[selection].rightOffset = rightOffset[selection]->text().toInt();
                serialPort.waitForReadyRead(2000);
                TLled[selection]->setText(serialPort.readLine().trimmed());
                profilesTable[selection].TLled = TLled[selection]->text().toFloat();
                serialPort.waitForReadyRead(2000);
                TRled[selection]->setText(serialPort.readLine().trimmed());
                profilesTable[selection].TRled = TRled[selection]->text().toFloat();

                DiffUpdate();
            }
        }
    } else if(testMode) {
        QString testBuffer = serialPort.readLine();
        if(testBuffer.contains(',')) {
            QStringList coordsList = testBuffer.remove("\r\n").split(',', Qt::SkipEmptyParts);

            testPointTL.setRect(coordsList[0].toInt()-25, coordsList[1].toInt()-25, 50, 50);
            testPointTR.setRect(coordsList[2].toInt()-25, coordsList[3].toInt()-25, 50, 50);
            testPointBL.setRect(coordsList[4].toInt()-25, coordsList[5].toInt()-25, 50, 50);
            testPointBR.setRect(coordsList[6].toInt()-25, coordsList[7].toInt()-25, 50, 50);
            testPointMed.setRect(coordsList[8].toInt()-25,coordsList[9].toInt()-25, 50, 50);
            testPointD.setRect(coordsList[10].toInt()-25, coordsList[11].toInt()-25, 50, 50);

            QPolygonF poly;
            poly << QPointF(coordsList[0].toInt(), coordsList[1].toInt()) << QPointF(coordsList[2].toInt(), coordsList[3].toInt()) << QPointF(coordsList[6].toInt(), coordsList[7].toInt()) << QPointF(coordsList[4].toInt(), coordsList[5].toInt()) << QPointF(coordsList[0].toInt(), coordsList[1].toInt());
            testBox.setPolygon(poly);
        }
    }
}


void guiWindow::on_rumbleTestBtn_clicked()
{
    serialPort.write("Xtr");
    if(!serialPort.waitForBytesWritten(1000)) QMessageBox::critical(this, "Lost connection!", "Somehow this happened I guess???");
    else ui->statusBar->showMessage("Sent a rumble test pulse.", 2500);
}


void guiWindow::on_solenoidTestBtn_clicked()
{
    serialPort.write("Xts");
    if(!serialPort.waitForBytesWritten(1000)) QMessageBox::critical(this, "Lost connection!", "Somehow this happened I guess???");
    else ui->statusBar->showMessage("Sent a solenoid test pulse.", 2500);
}


void guiWindow::on_redLedTestBtn_clicked()
{
    serialPort.write("XtR");
    if(!serialPort.waitForBytesWritten(1000)) QMessageBox::critical(this, "Lost connection!", "Somehow this happened I guess???");
    else ui->statusBar->showMessage("Set LED to Red.", 2500);
}


void guiWindow::on_greenLedTestBtn_clicked()
{
    serialPort.write("XtG");
    if(!serialPort.waitForBytesWritten(1000)) QMessageBox::critical(this, "Lost connection!", "Somehow this happened I guess???");
    else ui->statusBar->showMessage("Set LED to Green.", 2500);
}


void guiWindow::on_blueLedTestBtn_clicked()
{
    serialPort.write("XtB");
    if(!serialPort.waitForBytesWritten(1000)) QMessageBox::critical(this, "Lost connection!", "Somehow this happened I guess???");
    else ui->statusBar->showMessage("Set LED to Blue.", 2500);
}


void guiWindow::on_testBtn_clicked()
{
    if(serialPort.isOpen()) {
        // Pre-emptively put a sock in the readyRead signal
        serialActive = true;
        aliveTimer->stop();
        serialPort.write("XT");
        serialPort.waitForBytesWritten(1000);
        serialPort.waitForReadyRead(1000);
        if(serialPort.readLine().trimmed() == "Entering Test Mode...") {
            testMode = true;
            ui->testView->setEnabled(true);
            ui->buttonsTestArea->setEnabled(false);
            ui->testBtn->setText("Disable IR Test Mode");
            ui->confirmButton->setEnabled(false);
            ui->confirmButton->setText("[Disabled while in Test Mode]");
            ui->pinsTab->setEnabled(false);
            ui->settingsTab->setEnabled(false);
            ui->profilesTab->setEnabled(false);
            ui->feedbackTestsBox->setEnabled(false);
            ui->dangerZoneBox->setEnabled(false);
        } else {
            testMode = false;
            ui->testView->setEnabled(false);
            ui->buttonsTestArea->setEnabled(true);
            ui->testBtn->setText("Enable IR Test Mode");
            ui->pinsTab->setEnabled(true);
            ui->settingsTab->setEnabled(true);
            ui->profilesTab->setEnabled(true);
            ui->feedbackTestsBox->setEnabled(true);
            ui->dangerZoneBox->setEnabled(true);
            DiffUpdate();
            serialActive = false;
            aliveTimer->start(ALIVE_TIMER);
        }
    }
}


void guiWindow::on_clearEepromBtn_clicked()
{
    QMessageBox messageBox;
    messageBox.setText("Really delete saved data?");
    messageBox.setInformativeText("This operation will delete all saved data, including:\n\n - Calibration Profiles\n - Toggles\n - Settings\n - Custom Identifiers\n\nAre you sure about this?");
    messageBox.setWindowTitle("Delete Confirmation");
    messageBox.setIcon(QMessageBox::Warning);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::Yes);
    int value = messageBox.exec();
    if(value == QMessageBox::Yes) {
        if(serialPort.isOpen()) {
            serialActive = true;
            // clear the buffer if anything's been sent.
            while(!serialPort.atEnd()) {
                serialPort.readLine();
            }
            serialPort.write("Xc");
            serialPort.waitForBytesWritten(2000);
            if(serialPort.waitForReadyRead(5000)) {
                QString buffer = serialPort.readLine();
                if(buffer.trimmed() == "Cleared! Please reset the board.") {
                    serialPort.write("XE");
                    serialPort.waitForBytesWritten(2000);
                    serialPort.close();
                    serialActive = false;
                    ui->comPortSelector->setCurrentIndex(0);
                    QMessageBox::information(this, "Cleared storage.",
                                                   "Please unplug the board and reinsert it into the PC.");
                }
            }
        }
    } else ui->statusBar->showMessage("Clear operation canceled.", 3000);
}


void guiWindow::on_baudResetBtn_clicked()
{
    // No need for workarounds, bootloader reset is in the firmware now.
    serialActive = true;
    serialPort.write("Xxx");
    serialPort.waitForBytesWritten(1000);
    serialPort.close();

/* test stuff for potential app FW update functionality
    // At least on my system, the Bootloader device takes ~7s to appear
    QThread::msleep(7000);
    // Class-ify this function, maybe.
    QString picoPath;
    foreach(const QStorageInfo &storageDevices, QStorageInfo::mountedVolumes()) {
        if(storageDevices.isValid() && storageDevices.isReady() && storageDevices.displayName() == "RPI-RP2") {
            picoPath = storageDevices.device();
            qDebug() << "Found a Pico bootloader!";
            break;
        } else {
            qDebug() << "nope";
        }
    }
    qDebug() << picoPath;
    // QFile::copy("file", picoPath+"file");
*/
    ui->statusBar->showMessage("Board reset to bootloader.", 5000);
    ui->comPortSelector->setCurrentIndex(0);
    serialActive = false;
}

void guiWindow::on_actionAbout_UI_triggered()
{
    QDialog *about = new QDialog;
    Ui::aboutDialog aboutDialog;
    aboutDialog.setupUi(about);
    about->setFixedSize(450, 300);
    about->setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::WindowCloseButtonHint);
    about->show();
}

void guiWindow::on_actionOpenFIRE_Documentation_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/TeamOpenFIRE/OpenFIRE-Firmware/blob/OpenFIRE-dev/SamcoEnhanced/README.md"));
}


void guiWindow::on_actionOpenFIRE_Serial_Usage_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/TeamOpenFIRE/OpenFIRE-Firmware/wiki"));
}

