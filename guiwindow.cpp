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

#include "guiwindow.h"
#include "constants.h"
#include "ui_guiwindow.h"
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

// Currently loaded board object
boardInfo_s board;

// Currently loaded board's TinyUSB identifier info
tinyUSBtable_s tinyUSBtable;
// TinyUSB ident, as loaded from the board
tinyUSBtable_s tinyUSBtable_orig;

#define PROFILES_COUNT 4
// Current calibration profiles
QVector<profilesTable_s> profilesTable(PROFILES_COUNT);
// Calibration profiles, as loaded from the board
QVector<profilesTable_s> profilesTable_orig(PROFILES_COUNT);

// Map of what inputs are put where,
// Key = button/output, Value = pin number occupying, if any.
// Value of -1 means unmapped.
// Key order based on boardInputs_e, minus 1
// Map functions used in deduplication
QMap<uint8_t, int8_t> inputsMap;
// Inputs map, as loaded from the board
QMap<uint8_t, int8_t> inputsMap_orig;

// ^^^-----Typedefs up there:----^^^
//
// vvv---UI Objects down here:---vvv

// Guess I'll have to do the dynamic layout spawning/destroying stuff to make things work.
// How else do I hide things lol?
QVBoxLayout *PinsCenter;
QGridLayout *PinsCenterSub;
QGridLayout *PinsLeft;
QGridLayout *PinsRight;

QComboBox *pinBoxes[30];
QLabel *pinLabel[30];
QWidget *padding[30];

// buttons in the test screen
QLabel *testLabel[16];

QRadioButton *selectedProfile[PROFILES_COUNT];
QLabel *topOffset[PROFILES_COUNT];
QLabel *bottomOffset[PROFILES_COUNT];
QLabel *leftOffset[PROFILES_COUNT];
QLabel *rightOffset[PROFILES_COUNT];
QLabel *TLled[PROFILES_COUNT];
QLabel *TRled[PROFILES_COUNT];
QComboBox *irSens[PROFILES_COUNT];
QComboBox *runMode[PROFILES_COUNT];
QComboBox *layoutMode[PROFILES_COUNT];
QPushButton *color[PROFILES_COUNT];
QPushButton *renameBtn[PROFILES_COUNT];

QSvgWidget *centerPic;
QGraphicsScene *testScene;
#define ALIVE_TIMER 5000

//
// ^^^-------GLOBAL VARS UP THERE----------^^^
//
// vvv-------GUI METHODS DOWN HERE---------vvv
//

void guiWindow::PortsSearch()
{
    serialFoundList = QSerialPortInfo::availablePorts();
    if(serialFoundList.isEmpty()) {
        //statusBar()->showMessage("FATAL: No COM devices detected!");
        PopupWindow("No devices detected!", "Is the microcontroller board currently running OpenFIRE and is currently plugged in? Make sure it's connected and recognized by the PC.\n\nThis app will now close.", "ERROR", 4);
        exit(1);
    } else {
        // Yeah, sue me, we reading this backwards to make stack management easier.
        for(int i = serialFoundList.length() - 1; i >= 0; --i) {
            if(serialFoundList[i].vendorIdentifier() == 0xF143) {
                usbName.prepend(serialFoundList[i].systemLocation());
                qDebug() << "Found device @" << serialFoundList[i].systemLocation();
            } else {
                qDebug() << "Deleting dummy device" << serialFoundList[i].systemLocation();
                serialFoundList.removeAt(i);
            }
        }
        if(!usbName.length()) {
            PopupWindow("No OpenFIRE devices detected!", "Is the microcontroller board currently running OpenFIRE and is currently plugged in? Make sure it's connected and recognized by the PC.\n\nThis app will now close.", "ERROR", 4);
            exit(1);
        }
    }
}

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
            PopupWindow("User doesn't have serial permissions!", QString("Currently, your user is not allowed to have access to serial devices.\n\nTo add yourself to the right group, run this command in a terminal and then re-login to your session: \n\nsudo usermod -aG dialout %1").arg(qEnvironmentVariable("USER")), "Permission error", 2);
            exit(0);
        }
    } else {
        PopupWindow("Running as root is not allowed!", "Please run the OpenFIRE app as a normal user.", "ERROR", 4);
        exit(2);
    }
#endif

    connect(&serialPort, &QSerialPort::readyRead, this, &guiWindow::serialPort_readyRead);

    // just to be sure, init the inputsMap hashes
    for(uint8_t i = 0; i < boardInputsCount-1; i++) {
        inputsMap[i] = -1;
        inputsMap_orig[i] = -1;
    }

    // sending all these children to die upon comPortSelector->on_currentIndexChanged
    // (which gets fired immediately after ui->comPortSelector->addItems).
    PinsCenter = new QVBoxLayout();
    PinsCenterSub = new QGridLayout();
    PinsLeft = new QGridLayout();
    PinsRight = new QGridLayout();

    ui->PinsTopHalf->addLayout(PinsLeft);
    ui->PinsTopHalf->addLayout(PinsCenter);
    ui->PinsTopHalf->addLayout(PinsRight);

    for(uint8_t i = 0; i < 30; i++) {
        pinBoxes[i] = new QComboBox();
        connect(pinBoxes[i], SIGNAL(activated(int)), this, SLOT(pinBoxes_activated(int)));
        pinLabel[i] = new QLabel();
        pinLabel[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        padding[i] = new QWidget();
        padding[i]->setMinimumHeight(25);
    }

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
        if(i == 14) {
            testLabel[i]->setText(valuesNameList[tempPin]);
        } else if(i == 15) {
            testLabel[i]->setText("Analog Stick");
        } else {
            testLabel[i]->setText(valuesNameList[i+1]);
        }
        testLabel[i]->setEnabled(false);
        testLabel[i]->setAlignment(Qt::AlignCenter);
        testLabel[i]->setFrameStyle(QFrame::Box | QFrame::Raised);
        if(i == 15) {
            ui->buttonsTestLayout->addWidget(testLabel[i], 3, 3, 1, 1);
        } else if(i == 14) {
            ui->buttonsTestLayout->addWidget(testLabel[i], 3, 1, 1, 1);
        } else if(i > 9) {
            ui->buttonsTestLayout->addWidget(testLabel[i], 2, i-10, 1, 1);
        } else if(i > 4) {
            ui->buttonsTestLayout->addWidget(testLabel[i], 1, i-5, 1, 1);
        } else {
            ui->buttonsTestLayout->addWidget(testLabel[i], 0, i, 1, 1);
        }
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


void guiWindow::PopupWindow(QString errorTitle, QString errorMessage, QString windowTitle, int errorType)
{
    QMessageBox messageBox;
    messageBox.setText(errorTitle);
    messageBox.setInformativeText(errorMessage);
    messageBox.setWindowTitle(windowTitle);
    switch(errorType) {
    case 0:
        // lol nothing here
        break;
    case 1:
        messageBox.setIcon(QMessageBox::Question);
        break;
    case 2:
        messageBox.setIcon(QMessageBox::Information);
        break;
    case 3:
        messageBox.setIcon(QMessageBox::Warning);
        break;
    case 4:
        messageBox.setIcon(QMessageBox::Critical);
        break;
    }
    messageBox.exec();
    // TODO: maybe we should be using Serial Port errors instead of assuming,
    // but for now just clear it here for cleanliness.
    serialPort.clearError();
}


void guiWindow::SerialLoad()
{
    serialActive = true;
    serialPort.write("Xlb");
    if(serialPort.waitForBytesWritten(2000)) {
        if(serialPort.waitForReadyRead(2000)) {
            // booleans
            QString bufStr = serialPort.readLine().trimmed();
            QStringList buffer = bufStr.split(',');
            for(uint8_t i = 0; i < boolTypesCount; i++) {
                boolSettings[i] = buffer[i].toInt();
                boolSettings_orig[i] = boolSettings[i];
            }

            // pins
            if(boolSettings[customPins]) {
                serialPort.write("Xlp");
                serialPort.waitForBytesWritten(2000);
                serialPort.waitForReadyRead(2000);
                bufStr = serialPort.readLine().trimmed();
                buffer = bufStr.split(',');
                for(uint8_t i = 0; i < boardInputsCount-1; i++) {
                    inputsMap_orig[i] = buffer[i].toInt();
                }
                inputsMap = inputsMap_orig;
            }

            // settings
            serialPort.write("Xls");
            serialPort.waitForBytesWritten(2000);
            serialPort.waitForReadyRead(2000);
            bufStr = serialPort.readLine().trimmed();
            buffer = bufStr.split(',');
            for(uint8_t i = 0; i < settingsTypesCount; i++) {
                settingsTable[i] = buffer[i].toInt();
                settingsTable_orig[i] = settingsTable[i];
            }

            // profiles
            for(uint8_t i = 0; i < PROFILES_COUNT; i++) {
                QString genString = QString("XlP%1").arg(i);
                serialPort.write(genString.toLocal8Bit());
                serialPort.waitForBytesWritten(2000);
                serialPort.waitForReadyRead(2000);
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
                selectedProfile[i]->setText(buffer[10]), profilesTable[i].profName = buffer[10], profilesTable_orig[i].profName = profilesTable[i].profName;
            }
            serialActive = false;
        } else {
            PopupWindow("Data hasn't arrived!", "Device was detected, but settings request wasn't received in time!\nThis can happen if the app was closed in the middle of an operation.\n\nTry selecting the device again.", "Sync Error!", 4);
            //qDebug() << "Didn't receive any data in time! Dammit Seong, you jiggled the cable too much again!";
        }
    } else {
        qDebug() << "Couldn't send any data in time! Does the port even exist??? Fucking dammit Seong!?!?!?";
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
                QString bufStr = serialPort.readLine().trimmed();
                QStringList buffer = bufStr.split(',');
                if(buffer[0].contains("OpenFIRE")) {
                    qDebug() << "OpenFIRE gun detected!";
                    board.versionNumber = buffer[1];
                    qDebug() << "Version number:" << board.versionNumber;
                    board.versionCodename = buffer[2];
                    qDebug() << "Version codename:" << board.versionCodename;
                    if(buffer[3] == "rpipico") {
                        board.type = rpipico;
                    } else if(buffer[3] == "rpipicow") {
                        board.type = rpipicow;
                    } else if(buffer[3] == "adafruitItsyRP2040") {
                        board.type = adafruitItsyRP2040;
                    } else if(buffer[3] == "adafruitKB2040") {
                        board.type = adafruitKB2040;
                    } else if(buffer[3] == "arduinoNanoRP2040") {
                        board.type = arduinoNanoRP2040;
                    } else if(buffer[3] == "waveshareZero") {
                        board.type = waveshareZero;
                    } else if(buffer[3] == "vccgndYD") {
                        board.type = vccgndYD;
                    } else {
                        board.type = generic;
                    }
                    board.selectedProfile = buffer[4].toInt();
                    board.previousProfile = board.selectedProfile;
                    selectedProfile[board.selectedProfile]->setChecked(true);
                    serialPort.write("Xli");
                    serialPort.waitForReadyRead(1000);
                    bufStr = serialPort.readLine().trimmed();
                    buffer = bufStr.split(',');
                    tinyUSBtable.tinyUSBid = buffer[0];
                    tinyUSBtable_orig.tinyUSBid = tinyUSBtable.tinyUSBid;
                    if(buffer[1] == "SERIALREADERR01") {
                        tinyUSBtable.tinyUSBname = "";
                    } else {
                        tinyUSBtable.tinyUSBname = buffer[1];
                    }
                    tinyUSBtable_orig.tinyUSBname = tinyUSBtable.tinyUSBname;
                    SerialLoad();
                    return true;
                } else if(buffer[0].contains("Device not available")) {
                    PopupWindow("Camera not available!", "Device was detected, but data received indicates that the camera is in a bad state.\nThis can happen if the camera wires are crossed (data wire to clock pin, clock wire to data pin).\n\nThe camera must be removed or resoldered to resolve this.", "Device Error!", 3);
                    return false;
                } else {
                    qDebug() << "Port did not respond with expected response! Seong fucked this up again.";
                    return false;
                }
            } else {
                PopupWindow("Data hasn't arrived! (Stale state?)", "Device was detected, but initial settings request wasn't received in time!\nThis can happen if the app was unexpectedly closed and the gun is in a stale docked state.\n\nTry selecting the device again.", "Sync Error!", 3);
                qDebug() << "Didn't receive any data in time! Dammit Seong, you jiggled the cable too much again!";
                return false;
            }
        } else {
            qDebug() << "Couldn't send any data in time! Does the port even exist??? Fucking dammit Seong!?!?!?";
            return false;
        }
    } else {
        PopupWindow("Serial port is blocked!", "This usually indicates that the port is being used by something else, e.g. Arduino IDE's serial monitor, or another command line app (stty, screen).\n\nPlease close the offending application and try selecting this port again.", "Port In Use!", 3);
        return false;
    }
}


void guiWindow::BoxesUpdate()
{
    if(boolSettings[customPins]) {
        // if the custom pins setting *grabbed from the gun* has been set
        if(boolSettings_orig[customPins]) {
            // clear map
            currentPins.clear();
            // set or clear the local pins mapping
            for(uint8_t i = 0; i < 30; i++) {
                currentPins[i] = btnUnmapped;
            }
            // (re)-copy pins settings grabbed from the gun to the app catalog
            inputsMap = inputsMap_orig;
        // else, if the board *was using default maps* before switching to custom
        } else {
            for(uint8_t i = 0; i < 30; i++) {
                if(currentPins[i] > btnUnmapped) {
                    inputsMap[currentPins[i]-1] = i;
                }
            }
        }
        // enable pinboxes
        for(uint8_t i = 0; i < 30; i++) {
            pinBoxes[i]->setEnabled(true);
        }
        // copy OF's native inputs map layout to app's current pins layout, copy to pinboxes.
        for(uint8_t i = 0; i < boardInputsCount-1; i++) {
            if(inputsMap.value(i) >= 0) {
                currentPins[inputsMap.value(i)] = i+1;
                pinBoxes[inputsMap.value(i)]->setCurrentIndex(currentPins[inputsMap.value(i)]);
            }
        }
        return;
    } else {
        switch(board.type) {
        // Copy preloaded values to current pins map based on board.
        // pico and w are the same physical board, so why need a new layout for it?
        case rpipico:
        case rpipicow:
            for(uint8_t i = 0; i < 30; i++) { currentPins[i] = rpipicoLayout[i].pinAssignment; }
            break;
        case adafruitItsyRP2040:
            for(uint8_t i = 0; i < 30; i++) { currentPins[i] = adafruitItsyRP2040Layout[i].pinAssignment; }
            break;
        case adafruitKB2040:
            for(uint8_t i = 0; i < 30; i++) { currentPins[i] = adafruitKB2040Layout[i].pinAssignment; }
            break;
        case arduinoNanoRP2040:
            for(uint8_t i = 0; i < 30; i++) { currentPins[i] = arduinoNanoRP2040Layout[i].pinAssignment; }
            break;
        case waveshareZero:
            for(uint8_t i = 0; i < 30; i++) { currentPins[i] = waveshareZeroLayout[i].pinAssignment; }
            break;
        }

        // assign pinboxes from custom pins map
        for(uint8_t i = 0; i < 30; i++) {
            pinBoxes[i]->setCurrentIndex(currentPins[i]);
            pinBoxes[i]->setEnabled(false);
        }
        // convert app's current pins map (each pin = 1 function) to OF's native input map layout (each function = 1 pin)
        for(uint8_t i = 0; i < 30; i++) {
            if(currentPins[i] > btnUnmapped) {
                inputsMap[currentPins[i]-1] = i;
            }
        }
    }
}


void guiWindow::DiffUpdate()
{
    settingsDiff = 0;
    if(boolSettings_orig[customPins] != boolSettings[customPins]) {
        settingsDiff++;
    }
    if(boolSettings[customPins]) {
        if(inputsMap_orig != inputsMap) {
            settingsDiff++;
        }
    }
    for(uint8_t i = 1; i < boolTypesCount; i++) {
        if(boolSettings_orig[i] != boolSettings[i]) {
            settingsDiff++;
        }
    }
    for(uint8_t i = 0; i < settingsTypesCount; i++) {
        if(settingsTable_orig[i] != settingsTable[i]) {
            settingsDiff++;
        }
    }
    if(tinyUSBtable_orig.tinyUSBid != tinyUSBtable.tinyUSBid) {
        settingsDiff++;
    }
    if(tinyUSBtable_orig.tinyUSBname != tinyUSBtable.tinyUSBname) {
        settingsDiff++;
    }
    if(board.selectedProfile != board.previousProfile) {
        settingsDiff++;
    }
    for(uint8_t i = 0; i < PROFILES_COUNT; i++) {
        if(profilesTable_orig[i].profName != profilesTable[i].profName) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].topOffset != profilesTable[i].topOffset) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].bottomOffset != profilesTable[i].bottomOffset) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].leftOffset != profilesTable[i].leftOffset) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].rightOffset != profilesTable[i].rightOffset) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].TLled != profilesTable[i].TLled) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].TRled != profilesTable[i].TRled) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].irSensitivity != profilesTable[i].irSensitivity) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].runMode != profilesTable[i].runMode) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].layoutType != profilesTable[i].layoutType) {
            settingsDiff++;
        }
        if(profilesTable_orig[i].color != profilesTable[i].color) {
            settingsDiff++;
        }
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
    for(uint8_t i = 0; i < boolTypesCount; i++) {
        boolSettings_orig[i] = boolSettings[i];
    }
    if(boolSettings_orig[customPins]) {
        inputsMap_orig = inputsMap;
    } else {
        for(uint8_t i = 0; i < boardInputsCount-1; i++)
        inputsMap_orig[i] = -1;
    }
    for(uint8_t i = 0; i < settingsTypesCount; i++) {
        settingsTable_orig[i] = settingsTable[i];
    }
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


QString PrettifyName()
{
    QString name;

    if(!tinyUSBtable.tinyUSBname.isEmpty()) {
        name = tinyUSBtable.tinyUSBname;
    } else {
        name = "Unnamed Device";
    }

    // append name of board to gun name string.
    switch(board.type) {
    case nothing:
        name = "";
        break;
    case rpipico:
        name = name + " | Raspberry Pi Pico";
        break;
    case rpipicow:
        name = name + " | Raspberry Pi Pico W";
        break;
    case adafruitItsyRP2040:
        name = name + " | Adafruit ItsyBitsy RP2040";
        break;
    case adafruitKB2040:
        name = name + " | Adafruit KB2040";
        break;
    case arduinoNanoRP2040:
        name = name + " | Arduino Nano RP2040 Connect";
        break;
    case waveshareZero:
        name = name + " | Waveshare RP2040 Zero";
        break;
    case generic:
        name = name + " | Generic RP2040 Board";
        break;
    }

    return name;
}


void guiWindow::PixelsDiff()
{
    if(settingsTable[customLEDcount] == settingsTable_orig[customLEDcount] &&
        settingsTable[customLEDstatic] == settingsTable_orig[customLEDstatic] &&
        settingsTable[customLEDcolor1] == settingsTable_orig[customLEDcolor1] &&
        settingsTable[customLEDcolor2] == settingsTable_orig[customLEDcolor2] &&
        settingsTable[customLEDcolor3] == settingsTable_orig[customLEDcolor3]) {
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
            for(uint8_t i = 0; i < boolTypesCount; i++) {
                serialQueue.append(QString("Xm.0.%1.%2").arg(i).arg(boolSettings[i]));
            }

            if(boolSettings[customPins]) {
                for(uint8_t i = 0; i < boardInputsCount-1; i++) {
                    serialQueue.append(QString("Xm.1.%1.%2").arg(i).arg(inputsMap.value(i)));
                }
            }

            for(uint8_t i = 0; i < settingsTypesCount; i++) {
                serialQueue.append(QString("Xm.2.%1.%2").arg(i).arg(settingsTable[i]));
            }

            serialQueue.append(QString("Xm.3.0.%1").arg(tinyUSBtable.tinyUSBid));
            if(!tinyUSBtable.tinyUSBname.isEmpty()) {
                serialQueue.append(QString("Xm.3.1.%1").arg(tinyUSBtable.tinyUSBname));
            }
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
    // Indiscriminately clears the board layout views.
    // yes, every time. goddammit QT.
    // fuck it, it works until QT provides a better mechanism to remove widgets without deleting them.
    if(pinBoxes[0]->count() > 0) {
        for(uint8_t i = 0; i < 30; i++) {
            pinBoxes[i]->clear();
            delete pinBoxes[i];
            delete padding[i];
            delete pinLabel[i];
        }
        delete centerPic;
    }

    delete PinsCenter;
    delete PinsLeft;
    delete PinsRight;

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
        connect(pinBoxes[i], SIGNAL(activated(int)), this, SLOT(pinBoxes_activated(int)));
        padding[i] = new QWidget();
        padding[i]->setMinimumHeight(25);
        // I2C channel coloring
        if(i & 0b0000010) { pinLabel[i] = new QLabel(QString("<font color=#FF8800>«GPIO%1»</font>").arg(i)); }
        else { pinLabel[i] = new QLabel(QString("<font color=#0099FF>«GPIO%1»</font>").arg(i)); }
        pinLabel[i]->setEnabled(false);
        pinLabel[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        pinLabel[i]->setToolTip(QString("GPIO Pin number %1\n\nBlue pin numbers are members of I2C0\nOrange are members of I2C1").arg(i));
    }

    if(index > 0) {
        qDebug() << "COM port set to" << ui->comPortSelector->currentIndex();
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
        // try to init serial port
        // if returns false, it failed, so just turn the index back to initial.
        if(!SerialInit(index - 1)) {
            ui->comPortSelector->setCurrentIndex(0);
            aliveTimer->stop();
        // else, serial port is online! What do we got?
        } else {
            aliveTimer->start(ALIVE_TIMER);
            ui->versionLabel->setText(QString("v%1 - \"%2\"").arg(board.versionNumber, board.versionCodename));
            BoxesFill();
            LabelsUpdate();

            switch(board.type) {
                case rpipico:
                {
                    centerPic = new QSvgWidget(":/boardPics/pico.svg");
                    QSvgRenderer *picRenderer = centerPic->renderer();
                    picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);
                    ui->boardLabel->setText(PrettifyName());

                    // left side
                    PinsLeft->addWidget(padding[0],    0,  0);   // padding
                    PinsLeft->addWidget(pinBoxes[0],   1,  0), PinsLeft->addWidget(pinLabel[0],  1,  1);
                    PinsLeft->addWidget(pinBoxes[1],   2,  0), PinsLeft->addWidget(pinLabel[1],  2,  1);
                    PinsLeft->addWidget(padding[1],    3,  0);   // gnd
                    PinsLeft->addWidget(pinBoxes[2],   4,  0), PinsLeft->addWidget(pinLabel[2],  4,  1);
                    PinsLeft->addWidget(pinBoxes[3],   5,  0), PinsLeft->addWidget(pinLabel[3],  5,  1);
                    PinsLeft->addWidget(pinBoxes[4],   6,  0), PinsLeft->addWidget(pinLabel[4],  6,  1);
                    PinsLeft->addWidget(pinBoxes[5],   7,  0), PinsLeft->addWidget(pinLabel[5],  7,  1);
                    PinsLeft->addWidget(padding[2],    8,  0);   // gnd
                    PinsLeft->addWidget(pinBoxes[6],   9,  0), PinsLeft->addWidget(pinLabel[6],  9,  1);
                    PinsLeft->addWidget(pinBoxes[7],   10, 0), PinsLeft->addWidget(pinLabel[7],  10, 1);
                    PinsLeft->addWidget(pinBoxes[8],   11, 0), PinsLeft->addWidget(pinLabel[8],  11, 1);
                    PinsLeft->addWidget(pinBoxes[9],   12, 0), PinsLeft->addWidget(pinLabel[9],  12, 1);
                    PinsLeft->addWidget(padding[3],    13, 0);   // gnd
                    PinsLeft->addWidget(pinBoxes[10],  14, 0), PinsLeft->addWidget(pinLabel[10], 14, 1);
                    PinsLeft->addWidget(pinBoxes[11],  15, 0), PinsLeft->addWidget(pinLabel[11], 15, 1);
                    PinsLeft->addWidget(pinBoxes[12],  16, 0), PinsLeft->addWidget(pinLabel[12], 16, 1);
                    PinsLeft->addWidget(pinBoxes[13],  17, 0), PinsLeft->addWidget(pinLabel[13], 17, 1);
                    PinsLeft->addWidget(padding[4],    18, 0);   // gnd
                    PinsLeft->addWidget(pinBoxes[14],  19, 0), PinsLeft->addWidget(pinLabel[14], 19, 1);
                    PinsLeft->addWidget(pinBoxes[15],  20, 0), PinsLeft->addWidget(pinLabel[15], 20, 1);

                    // right side
                    PinsRight->addWidget(padding[5],   0,  1);   // padding
                    PinsRight->addWidget(padding[6],   1,  1);   // VBUS
                    PinsRight->addWidget(padding[7],   2,  1);   // VSYS
                    PinsRight->addWidget(padding[8],   3,  1);   // gnd
                    PinsRight->addWidget(padding[9],   4,  1);   // 3V3 EN
                    PinsRight->addWidget(padding[10],  5,  1);   // 3V3 OUT
                    PinsRight->addWidget(padding[11],  6,  1);   // ADC VREF
                    PinsRight->addWidget(pinBoxes[28], 7,  1), PinsRight->addWidget(pinLabel[28], 7,  0);
                    PinsRight->addWidget(padding[12],  8,  1);   // gnd
                    PinsRight->addWidget(pinBoxes[27], 9,  1), PinsRight->addWidget(pinLabel[27], 9,  0);
                    PinsRight->addWidget(pinBoxes[26], 10, 1), PinsRight->addWidget(pinLabel[26], 10, 0);
                    PinsRight->addWidget(padding[13],  11, 1);   // RUN
                    PinsRight->addWidget(pinBoxes[22], 12, 1), PinsRight->addWidget(pinLabel[22], 12, 0);
                    PinsRight->addWidget(padding[14],  13, 1);   // gnd
                    PinsRight->addWidget(pinBoxes[21], 14, 1), PinsRight->addWidget(pinLabel[21], 14, 0);
                    PinsRight->addWidget(pinBoxes[20], 15, 1), PinsRight->addWidget(pinLabel[20], 15, 0);
                    PinsRight->addWidget(pinBoxes[19], 16, 1), PinsRight->addWidget(pinLabel[19], 16, 0);
                    PinsRight->addWidget(pinBoxes[18], 17, 1), PinsRight->addWidget(pinLabel[18], 17, 0);
                    PinsRight->addWidget(padding[17],  18, 1);   // gnd
                    PinsRight->addWidget(pinBoxes[17], 19, 1), PinsRight->addWidget(pinLabel[17], 19, 0);
                    PinsRight->addWidget(pinBoxes[16], 20, 1), PinsRight->addWidget(pinLabel[16], 20, 0);

                    // center
                    PinsCenter->addWidget(centerPic);
                    centerPic->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
                    break;
                }
                case rpipicow:
                {
                    centerPic = new QSvgWidget(":/boardPics/picow.svg");
                    QSvgRenderer *picRenderer = centerPic->renderer();
                    picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);
                    ui->boardLabel->setText(PrettifyName());

                    // left side
                    PinsLeft->addWidget(padding[0],    0,  0);   // padding
                    PinsLeft->addWidget(pinBoxes[0],   1,  0), PinsLeft->addWidget(pinLabel[0],  1,  1);
                    PinsLeft->addWidget(pinBoxes[1],   2,  0), PinsLeft->addWidget(pinLabel[1],  2,  1);
                    PinsLeft->addWidget(padding[1],    3,  0);   // gnd
                    PinsLeft->addWidget(pinBoxes[2],   4,  0), PinsLeft->addWidget(pinLabel[2],  4,  1);
                    PinsLeft->addWidget(pinBoxes[3],   5,  0), PinsLeft->addWidget(pinLabel[3],  5,  1);
                    PinsLeft->addWidget(pinBoxes[4],   6,  0), PinsLeft->addWidget(pinLabel[4],  6,  1);
                    PinsLeft->addWidget(pinBoxes[5],   7,  0), PinsLeft->addWidget(pinLabel[5],  7,  1);
                    PinsLeft->addWidget(padding[2],    8,  0);   // gnd
                    PinsLeft->addWidget(pinBoxes[6],   9,  0), PinsLeft->addWidget(pinLabel[6],  9,  1);
                    PinsLeft->addWidget(pinBoxes[7],   10, 0), PinsLeft->addWidget(pinLabel[7],  10, 1);
                    PinsLeft->addWidget(pinBoxes[8],   11, 0), PinsLeft->addWidget(pinLabel[8],  11, 1);
                    PinsLeft->addWidget(pinBoxes[9],   12, 0), PinsLeft->addWidget(pinLabel[9],  12, 1);
                    PinsLeft->addWidget(padding[3],    13, 0);   // gnd
                    PinsLeft->addWidget(pinBoxes[10],  14, 0), PinsLeft->addWidget(pinLabel[10], 14, 1);
                    PinsLeft->addWidget(pinBoxes[11],  15, 0), PinsLeft->addWidget(pinLabel[11], 15, 1);
                    PinsLeft->addWidget(pinBoxes[12],  16, 0), PinsLeft->addWidget(pinLabel[12], 16, 1);
                    PinsLeft->addWidget(pinBoxes[13],  17, 0), PinsLeft->addWidget(pinLabel[13], 17, 1);
                    PinsLeft->addWidget(padding[4],    18, 0);   // gnd
                    PinsLeft->addWidget(pinBoxes[14],  19, 0), PinsLeft->addWidget(pinLabel[14], 19, 1);
                    PinsLeft->addWidget(pinBoxes[15],  20, 0), PinsLeft->addWidget(pinLabel[15], 20, 1);

                    // right side
                    PinsRight->addWidget(padding[5],   0,  1);   // padding
                    PinsRight->addWidget(padding[6],   1,  1);   // VBUS
                    PinsRight->addWidget(padding[7],   2,  1);   // VSYS
                    PinsRight->addWidget(padding[8],   3,  1);   // gnd
                    PinsRight->addWidget(padding[9],   4,  1);   // 3V3 EN
                    PinsRight->addWidget(padding[10],  5,  1);   // 3V3 OUT
                    PinsRight->addWidget(padding[11],  6,  1);   // ADC VREF
                    PinsRight->addWidget(pinBoxes[28], 7,  1), PinsRight->addWidget(pinLabel[28], 7,  0);
                    PinsRight->addWidget(padding[12],  8,  1);   // gnd
                    PinsRight->addWidget(pinBoxes[27], 9,  1), PinsRight->addWidget(pinLabel[27], 9,  0);
                    PinsRight->addWidget(pinBoxes[26], 10, 1), PinsRight->addWidget(pinLabel[26], 10, 0);
                    PinsRight->addWidget(padding[13],  11, 1);   // RUN
                    PinsRight->addWidget(pinBoxes[22], 12, 1), PinsRight->addWidget(pinLabel[22], 12, 0);
                    PinsRight->addWidget(padding[14],  13, 1);   // gnd
                    PinsRight->addWidget(pinBoxes[21], 14, 1), PinsRight->addWidget(pinLabel[21], 14, 0);
                    PinsRight->addWidget(pinBoxes[20], 15, 1), PinsRight->addWidget(pinLabel[20], 15, 0);
                    PinsRight->addWidget(pinBoxes[19], 16, 1), PinsRight->addWidget(pinLabel[19], 16, 0);
                    PinsRight->addWidget(pinBoxes[18], 17, 1), PinsRight->addWidget(pinLabel[18], 17, 0);
                    PinsRight->addWidget(padding[17],  18, 1);   // gnd
                    PinsRight->addWidget(pinBoxes[17], 19, 1), PinsRight->addWidget(pinLabel[17], 19, 0);
                    PinsRight->addWidget(pinBoxes[16], 20, 1), PinsRight->addWidget(pinLabel[16], 20, 0);

                    // center
                    PinsCenter->addWidget(centerPic);
                    centerPic->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
                    break;
                }
                case adafruitItsyRP2040:
                {
                    centerPic = new QSvgWidget(":/boardPics/adafruitItsy2040.svg");
                    QSvgRenderer *picRenderer = centerPic->renderer();
                    picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);
                    ui->boardLabel->setText(PrettifyName());

                    // left side
                    PinsLeft->addWidget(padding[0],    0,  0);   // reset
                    PinsLeft->addWidget(padding[1],    1,  0);   // 3v3_1
                    PinsLeft->addWidget(padding[3],    2,  0);   // 3v3_2
                    PinsLeft->addWidget(padding[4],    3,  0);   // VHi
                    PinsLeft->addWidget(pinBoxes[26],  4,  0), PinsLeft->addWidget(pinLabel[26], 4,  1);
                    PinsLeft->addWidget(pinBoxes[27],  5,  0), PinsLeft->addWidget(pinLabel[27], 5,  1);
                    PinsLeft->addWidget(pinBoxes[28],  6,  0), PinsLeft->addWidget(pinLabel[28], 6,  1);
                    PinsLeft->addWidget(pinBoxes[29],  7,  0), PinsLeft->addWidget(pinLabel[29], 7,  1);
                    PinsLeft->addWidget(pinBoxes[24],  8,  0), PinsLeft->addWidget(pinLabel[24], 8,  1);
                    PinsLeft->addWidget(pinBoxes[25],  9,  0), PinsLeft->addWidget(pinLabel[25], 9,  1);
                    PinsLeft->addWidget(pinBoxes[18],  10, 0), PinsLeft->addWidget(pinLabel[18], 10, 1);
                    PinsLeft->addWidget(pinBoxes[19],  11, 0), PinsLeft->addWidget(pinLabel[19], 11, 1);
                    PinsLeft->addWidget(pinBoxes[20],  12, 0), PinsLeft->addWidget(pinLabel[20], 12, 1);
                    PinsLeft->addWidget(pinBoxes[12],  13, 0), PinsLeft->addWidget(pinLabel[12], 13, 1);
                    PinsLeft->addWidget(padding[5],    14, 0);   // bottom padding
                    PinsLeft->addWidget(padding[6],    14, 0);

                    // right side
                    PinsRight->addWidget(padding[8],   0,  1);   // battery
                    PinsRight->addWidget(padding[9],   1,  1);   // gnd
                    PinsRight->addWidget(padding[10],  2,  1);   // USB power in
                    PinsRight->addWidget(pinBoxes[11], 3,  1), PinsRight->addWidget(pinLabel[11], 3,  0);
                    PinsRight->addWidget(pinBoxes[10], 4,  1), PinsRight->addWidget(pinLabel[10], 4,  0);
                    PinsRight->addWidget(pinBoxes[9],  5,  1), PinsRight->addWidget(pinLabel[9],  5,  0);
                    PinsRight->addWidget(pinBoxes[8],  6,  1), PinsRight->addWidget(pinLabel[8],  6,  0);
                    PinsRight->addWidget(pinBoxes[7],  7,  1), PinsRight->addWidget(pinLabel[7],  7,  0);
                    PinsRight->addWidget(pinBoxes[6],  8,  1), PinsRight->addWidget(pinLabel[6],  8,  0);
                    PinsRight->addWidget(padding[11],  9,  1);   // 5!
                    PinsRight->addWidget(pinBoxes[3],  10, 1), PinsRight->addWidget(pinLabel[3],  10, 0);
                    PinsRight->addWidget(pinBoxes[2],  11, 1), PinsRight->addWidget(pinLabel[2],  11, 0);
                    PinsRight->addWidget(pinBoxes[0],  12, 1), PinsRight->addWidget(pinLabel[0],  12, 0);
                    PinsRight->addWidget(pinBoxes[1],  13, 1), PinsRight->addWidget(pinLabel[1],  13, 0);
                    PinsRight->addWidget(padding[12],  14, 1);   // bottom padding
                    PinsRight->addWidget(padding[13],  15, 1);

                    // center
                    PinsCenter->addWidget(centerPic);
                    PinsCenter->addLayout(PinsCenterSub);
                    centerPic->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
                    PinsCenterSub->addWidget(pinBoxes[4], 1, 3), PinsCenterSub->addWidget(pinLabel[4], 0, 3);
                    PinsCenterSub->addWidget(pinBoxes[5], 1, 2), PinsCenterSub->addWidget(pinLabel[5], 0, 2);
                    break;
                }
                case adafruitKB2040:
                {
                    centerPic = new QSvgWidget(":/boardPics/adafruitKB2040.svg");
                    QSvgRenderer *picRenderer = centerPic->renderer();
                    picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);
                    ui->boardLabel->setText(PrettifyName());

                    // left side
                    PinsLeft->addWidget(padding[0],    0,  0);   // padding
                    PinsLeft->addWidget(padding[1],    1,  0);   // D+
                    PinsLeft->addWidget(pinBoxes[0],   2,  0), PinsLeft->addWidget(pinLabel[0],   2,  1);
                    PinsLeft->addWidget(pinBoxes[1],   3,  0), PinsLeft->addWidget(pinLabel[1],   3,  1);
                    PinsLeft->addWidget(padding[2],    4,  0);   // gnd
                    PinsLeft->addWidget(padding[3],    5,  0);   // gnd
                    PinsLeft->addWidget(pinBoxes[2],   6,  0), PinsLeft->addWidget(pinLabel[2],   6,  1);
                    PinsLeft->addWidget(pinBoxes[3],   7,  0), PinsLeft->addWidget(pinLabel[3],   7,  1);
                    PinsLeft->addWidget(pinBoxes[4],   8,  0), PinsLeft->addWidget(pinLabel[4],   8,  1);
                    PinsLeft->addWidget(pinBoxes[5],   9,  0), PinsLeft->addWidget(pinLabel[5],   9,  1);
                    PinsLeft->addWidget(pinBoxes[6],   10, 0), PinsLeft->addWidget(pinLabel[6],   10, 1);
                    PinsLeft->addWidget(pinBoxes[7],   11, 0), PinsLeft->addWidget(pinLabel[7],   11, 1);
                    PinsLeft->addWidget(pinBoxes[8],   12, 0), PinsLeft->addWidget(pinLabel[8],   12, 1);
                    PinsLeft->addWidget(pinBoxes[9],   13, 0), PinsLeft->addWidget(pinLabel[9],   13, 1);

                    // right side
                    PinsRight->addWidget(padding[4],   0,  1);   // padding
                    PinsRight->addWidget(padding[5],   1,  1);   // D-
                    PinsRight->addWidget(padding[6],   2,  1);   // RAW
                    PinsRight->addWidget(padding[7],   3,  1);   // gnd
                    PinsRight->addWidget(padding[8],   4,  1);   // reset
                    PinsRight->addWidget(padding[9],   5,  1);   // 3.3v
                    PinsRight->addWidget(pinBoxes[29], 6,  1), PinsRight->addWidget(pinLabel[29], 6,  0);
                    PinsRight->addWidget(pinBoxes[28], 7,  1), PinsRight->addWidget(pinLabel[28], 7,  0);
                    PinsRight->addWidget(pinBoxes[27], 8,  1), PinsRight->addWidget(pinLabel[27], 8,  0);
                    PinsRight->addWidget(pinBoxes[26], 9,  1), PinsRight->addWidget(pinLabel[26], 9,  0);
                    PinsRight->addWidget(pinBoxes[18], 10, 1), PinsRight->addWidget(pinLabel[18], 10, 0);
                    PinsRight->addWidget(pinBoxes[20], 11, 1), PinsRight->addWidget(pinLabel[20], 11, 0);
                    PinsRight->addWidget(pinBoxes[19], 12, 1), PinsRight->addWidget(pinLabel[19], 12, 0);
                    PinsRight->addWidget(pinBoxes[10], 13, 1), PinsRight->addWidget(pinLabel[10], 13, 0);

                    // center
                    PinsCenter->addWidget(centerPic);
                    centerPic->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
                    break;
                }
                case arduinoNanoRP2040:
                {
                    centerPic = new QSvgWidget(":/boardPics/arduinoNano2040.svg");
                    QSvgRenderer *picRenderer = centerPic->renderer();
                    picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);
                    PinsCenter->addWidget(centerPic);
                    ui->boardLabel->setText(PrettifyName());

                    // left side
                    PinsLeft->addWidget(padding[0],    0,  0);   // top padding
                    PinsLeft->addWidget(padding[1],    1,  0);
                    PinsLeft->addWidget(padding[2],    2,  0);
                    PinsLeft->addWidget(pinBoxes[6],   3,  0), PinsLeft->addWidget(pinLabel[6],   3,  1);
                    PinsLeft->addWidget(padding[3],    4,  0);   // 3V3 Out
                    PinsLeft->addWidget(padding[4],    5,  0);   // AREF
                    PinsLeft->addWidget(pinBoxes[26],  6,  0), PinsLeft->addWidget(pinLabel[26],  6,  1);
                    PinsLeft->addWidget(pinBoxes[27],  7,  0), PinsLeft->addWidget(pinLabel[27],  7,  1);
                    PinsLeft->addWidget(pinBoxes[28],  8,  0), PinsLeft->addWidget(pinLabel[28],  8,  1);
                    PinsLeft->addWidget(pinBoxes[29],  9,  0), PinsLeft->addWidget(pinLabel[29],  9,  1);
                    PinsLeft->addWidget(pinBoxes[12],  10, 0), PinsLeft->addWidget(pinLabel[12],  10, 1);
                    PinsLeft->addWidget(pinBoxes[13],  11, 0), PinsLeft->addWidget(pinLabel[13],  11, 1);
                    PinsLeft->addWidget(padding[5],    12, 0);   // A6 - unused
                    PinsLeft->addWidget(padding[6],    13, 0);   // A7 - unused
                    PinsLeft->addWidget(padding[7],    14, 0);   // 5V OUT
                    PinsLeft->addWidget(padding[8],    15, 0);   // REC?
                    PinsLeft->addWidget(padding[9],    16, 0);   // gnd
                    PinsLeft->addWidget(padding[10],   17, 0);   // 5V IN
                    PinsLeft->addWidget(padding[11],   18, 0);   // bottom padding
                    PinsLeft->addWidget(padding[12],   19, 0);

                    // right side
                    PinsRight->addWidget(padding[13],  0,  1);   // top padding
                    PinsRight->addWidget(padding[14],  1,  1);   // top padding
                    PinsRight->addWidget(padding[15],  2,  1);   // top padding
                    PinsRight->addWidget(pinBoxes[4],  3,  1), PinsRight->addWidget(pinLabel[4],  3,  0);
                    PinsRight->addWidget(pinBoxes[7],  4,  1), PinsRight->addWidget(pinLabel[7],  4,  0);
                    PinsRight->addWidget(pinBoxes[5],  5,  1), PinsRight->addWidget(pinLabel[5],  5,  0);
                    PinsRight->addWidget(pinBoxes[21], 6,  1), PinsRight->addWidget(pinLabel[21], 6,  0);
                    PinsRight->addWidget(pinBoxes[20], 7,  1), PinsRight->addWidget(pinLabel[20], 7,  0);
                    PinsRight->addWidget(pinBoxes[19], 8,  1), PinsRight->addWidget(pinLabel[19], 8,  0);
                    PinsRight->addWidget(pinBoxes[18], 9,  1), PinsRight->addWidget(pinLabel[18], 9,  0);
                    PinsRight->addWidget(pinBoxes[17], 10, 1), PinsRight->addWidget(pinLabel[17], 10, 0);
                    PinsRight->addWidget(pinBoxes[16], 11, 1), PinsRight->addWidget(pinLabel[16], 11, 0);
                    PinsRight->addWidget(pinBoxes[15], 12, 1), PinsRight->addWidget(pinLabel[15], 12, 0);
                    PinsRight->addWidget(pinBoxes[25], 13, 1), PinsRight->addWidget(pinLabel[25], 13, 0);
                    PinsRight->addWidget(padding[16],  14, 1);   // gnd
                    PinsRight->addWidget(padding[17],  15, 1);   // RESET
                    PinsRight->addWidget(pinBoxes[1],  16, 1), PinsRight->addWidget(pinLabel[1],  16, 0);
                    PinsRight->addWidget(pinBoxes[0],  17, 1), PinsRight->addWidget(pinLabel[0],  17, 0);
                    PinsRight->addWidget(padding[18],  18, 1);   // bottom padding
                    PinsRight->addWidget(padding[19],  19, 1);

                    // center
                    PinsCenter->addWidget(centerPic);
                    break;
                }
                case waveshareZero:
                {
                    centerPic = new QSvgWidget(":/boardPics/waveshareZero.svg");
                    QSvgRenderer *picRenderer = centerPic->renderer();
                    picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);
                    ui->boardLabel->setText(PrettifyName());

                    // left side
                    PinsLeft->addWidget(padding[0],   0,  0);    // 5V OUT
                    PinsLeft->addWidget(padding[1],   1,  0);    // gnd
                    PinsLeft->addWidget(padding[2],   2,  0);    // 3V3 OUT
                    PinsLeft->addWidget(pinBoxes[29], 3,  0),  PinsLeft->addWidget(pinLabel[29], 3,  1);
                    PinsLeft->addWidget(pinBoxes[28], 4,  0),  PinsLeft->addWidget(pinLabel[28], 4,  1);
                    PinsLeft->addWidget(pinBoxes[27], 5,  0),  PinsLeft->addWidget(pinLabel[27], 5,  1);
                    PinsLeft->addWidget(pinBoxes[26], 6,  0),  PinsLeft->addWidget(pinLabel[26], 6,  1);
                    PinsLeft->addWidget(pinBoxes[15], 7,  0),  PinsLeft->addWidget(pinLabel[15], 7,  1);
                    PinsLeft->addWidget(pinBoxes[14], 8,  0),  PinsLeft->addWidget(pinLabel[14], 8,  1);
                    PinsLeft->addWidget(pinBoxes[13], 9,  0),  PinsLeft->addWidget(pinLabel[13], 9,  1);
                    PinsLeft->addWidget(pinBoxes[12], 10, 0),  PinsLeft->addWidget(pinLabel[12], 10, 1);

                    // right side
                    PinsRight->addWidget(padding[3],  0,  1);    // padding
                    PinsRight->addWidget(pinBoxes[0], 1,  1),  PinsRight->addWidget(pinLabel[0], 1,  0);
                    PinsRight->addWidget(pinBoxes[1], 2,  1),  PinsRight->addWidget(pinLabel[1], 2,  0);
                    PinsRight->addWidget(pinBoxes[2], 3,  1),  PinsRight->addWidget(pinLabel[2], 3,  0);
                    PinsRight->addWidget(pinBoxes[3], 4,  1),  PinsRight->addWidget(pinLabel[3], 4,  0);
                    PinsRight->addWidget(pinBoxes[4], 5,  1),  PinsRight->addWidget(pinLabel[4], 5,  0);
                    PinsRight->addWidget(pinBoxes[5], 6,  1),  PinsRight->addWidget(pinLabel[5], 6,  0);
                    PinsRight->addWidget(pinBoxes[6], 7,  1),  PinsRight->addWidget(pinLabel[6], 7,  0);
                    PinsRight->addWidget(pinBoxes[7], 8,  1),  PinsRight->addWidget(pinLabel[7], 8,  0);
                    PinsRight->addWidget(pinBoxes[8], 9,  1),  PinsRight->addWidget(pinLabel[8], 9,  0);
                    PinsRight->addWidget(pinBoxes[9], 10, 1),  PinsRight->addWidget(pinLabel[9], 10, 0);

                    // center
                    PinsCenter->addWidget(centerPic);
                    PinsCenter->addLayout(PinsCenterSub);
                    centerPic->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
                    PinsCenterSub->addWidget(pinBoxes[10], 1, 3), PinsCenterSub->addWidget(pinLabel[10], 0, 3);
                    PinsCenterSub->addWidget(pinBoxes[11], 1, 2), PinsCenterSub->addWidget(pinLabel[11], 0, 2);
                    break;
                }
                case generic:
                {
                    centerPic = new QSvgWidget(":/boardPics/unknown.svg");
                    QSvgRenderer *picRenderer = centerPic->renderer();
                    picRenderer->setAspectRatioMode(Qt::KeepAspectRatio);
                    ui->boardLabel->setText(PrettifyName());

                    // left side
                    PinsLeft->addWidget(padding[0],    0,  0);   // padding
                    PinsLeft->addWidget(pinBoxes[0],   1,  0), PinsLeft->addWidget(pinLabel[0],  1,  1);
                    PinsLeft->addWidget(pinBoxes[1],   2,  0), PinsLeft->addWidget(pinLabel[1],  2,  1);
                    PinsLeft->addWidget(padding[1],    3,  0);   // gnd
                    PinsLeft->addWidget(pinBoxes[2],   4,  0), PinsLeft->addWidget(pinLabel[2],  4,  1);
                    PinsLeft->addWidget(pinBoxes[3],   5,  0), PinsLeft->addWidget(pinLabel[3],  5,  1);
                    PinsLeft->addWidget(pinBoxes[4],   6,  0), PinsLeft->addWidget(pinLabel[4],  6,  1);
                    PinsLeft->addWidget(pinBoxes[5],   7,  0), PinsLeft->addWidget(pinLabel[5],  7,  1);
                    PinsLeft->addWidget(padding[2],    8,  0);   // gnd
                    PinsLeft->addWidget(pinBoxes[6],   9,  0), PinsLeft->addWidget(pinLabel[6],  9,  1);
                    PinsLeft->addWidget(pinBoxes[7],   10, 0), PinsLeft->addWidget(pinLabel[7],  10, 1);
                    PinsLeft->addWidget(pinBoxes[8],   11, 0), PinsLeft->addWidget(pinLabel[8],  11, 1);
                    PinsLeft->addWidget(pinBoxes[9],   12, 0), PinsLeft->addWidget(pinLabel[9],  12, 1);
                    PinsLeft->addWidget(padding[3],    13, 0);   // gnd
                    PinsLeft->addWidget(pinBoxes[10],  14, 0), PinsLeft->addWidget(pinLabel[10], 14, 1);
                    PinsLeft->addWidget(pinBoxes[11],  15, 0), PinsLeft->addWidget(pinLabel[11], 15, 1);
                    PinsLeft->addWidget(pinBoxes[12],  16, 0), PinsLeft->addWidget(pinLabel[12], 16, 1);
                    PinsLeft->addWidget(pinBoxes[13],  17, 0), PinsLeft->addWidget(pinLabel[13], 17, 1);
                    PinsLeft->addWidget(padding[4],    18, 0);   // gnd
                    PinsLeft->addWidget(pinBoxes[14],  19, 0), PinsLeft->addWidget(pinLabel[14], 19, 1);
                    PinsLeft->addWidget(pinBoxes[15],  20, 0), PinsLeft->addWidget(pinLabel[15], 20, 1);

                    // right side
                    PinsRight->addWidget(padding[5],   0,  1);   // padding
                    PinsRight->addWidget(padding[6],   1,  1);
                    PinsRight->addWidget(padding[7],   2,  1);
                    PinsRight->addWidget(padding[8],   3,  1);   // gnd
                    PinsRight->addWidget(padding[9],   4,  1);
                    PinsRight->addWidget(padding[10],  5,  1);
                    PinsRight->addWidget(padding[11],  6,  1);
                    PinsRight->addWidget(pinBoxes[28], 7,  1), PinsRight->addWidget(pinLabel[28], 7,  0);
                    PinsRight->addWidget(padding[12],  8,  1);   // gnd
                    PinsRight->addWidget(pinBoxes[27], 9,  1), PinsRight->addWidget(pinLabel[27], 9,  0);
                    PinsRight->addWidget(pinBoxes[26], 10, 1), PinsRight->addWidget(pinLabel[26], 10, 0);
                    PinsRight->addWidget(padding[13],  11, 1);
                    PinsRight->addWidget(pinBoxes[22], 12, 1), PinsRight->addWidget(pinLabel[22], 12, 0);
                    PinsRight->addWidget(padding[14],  13, 1);   // gnd
                    PinsRight->addWidget(pinBoxes[21], 14, 1), PinsRight->addWidget(pinLabel[21], 14, 0);
                    PinsRight->addWidget(pinBoxes[20], 15, 1), PinsRight->addWidget(pinLabel[20], 15, 0);
                    PinsRight->addWidget(pinBoxes[19], 16, 1), PinsRight->addWidget(pinLabel[19], 16, 0);
                    PinsRight->addWidget(pinBoxes[18], 17, 1), PinsRight->addWidget(pinLabel[18], 17, 0);
                    PinsRight->addWidget(padding[17],  18, 1);   // gnd
                    PinsRight->addWidget(pinBoxes[17], 19, 1), PinsRight->addWidget(pinLabel[17], 19, 0);
                    PinsRight->addWidget(pinBoxes[16], 20, 1), PinsRight->addWidget(pinLabel[16], 20, 0);

                    // center
                    PinsCenter->addWidget(centerPic);
                    centerPic->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
                    break;
                }
            }

            ui->tabWidget->setEnabled(true);
            ui->customPinsEnabled->setChecked(boolSettings[customPins]);

            if(inputsMap[rumblePin-1] >= 0) { ui->rumbleToggle->setEnabled(true), ui->rumbleFFToggle->setEnabled(true); } else { ui->rumbleToggle->setEnabled(false), ui->rumbleFFToggle->setEnabled(false); }
            ui->rumbleToggle->setChecked(boolSettings[rumble]);

            if(inputsMap[solenoidPin-1] >= 0) { ui->solenoidToggle->setEnabled(true); } else { ui->solenoidToggle->setEnabled(false); }
            ui->solenoidToggle->setChecked(boolSettings[solenoid]);

            if((boolSettings[rumble] && boolSettings[rumbleFF]) || boolSettings[solenoid]) { ui->autofireToggle->setEnabled(true); } else { ui->autofireToggle->setEnabled(false); }
            ui->autofireToggle->setChecked(boolSettings[autofire]);

            ui->simplePauseToggle->setChecked(boolSettings[simplePause]);
            ui->holdToPauseToggle->setChecked(boolSettings[holdToPause]);

            if(inputsMap[ledR-1] >= 0 && inputsMap[ledG-1] >= 0 && inputsMap[ledB-1] >= 0) { ui->commonAnodeToggle->setEnabled(true); } else { ui->commonAnodeToggle->setEnabled(false); }
            ui->commonAnodeToggle->setChecked(boolSettings[commonAnode]);
            ui->lowButtonsToggle->setChecked(boolSettings[lowButtonsMode]);
            ui->rumbleFFToggle->setChecked(boolSettings[rumbleFF]);
            ui->rumbleIntensityBox->setEnabled(boolSettings[rumble]),          ui->rumbleIntensityBox->setValue(settingsTable[rumbleStrength]);
            ui->rumbleLengthBox->setEnabled(boolSettings[rumble]),             ui->rumbleLengthBox->setValue(settingsTable[rumbleInterval]);
            ui->holdToPauseLengthBox->setEnabled(boolSettings[holdToPause]),   ui->holdToPauseLengthBox->setValue(settingsTable[holdToPauseLength]);
            ui->solenoidNormalIntervalBox->setEnabled(boolSettings[solenoid]), ui->solenoidNormalIntervalBox->setValue(settingsTable[solenoidNormalInterval]);
            ui->solenoidFastIntervalBox->setEnabled(boolSettings[solenoid]),   ui->solenoidFastIntervalBox->setValue(settingsTable[solenoidFastInterval]);
            ui->solenoidHoldLengthBox->setEnabled(boolSettings[solenoid]),     ui->solenoidHoldLengthBox->setValue(settingsTable[solenoidHoldLength]);
            ui->autofireWaitFactorBox->setEnabled(boolSettings[autofire]),     ui->autofireWaitFactorBox->setValue(settingsTable[autofireWaitFactor]);
            ui->productIdInput->setText(tinyUSBtable.tinyUSBid);
            ui->productNameInput->setText(tinyUSBtable.tinyUSBname);
            if(inputsMap[neoPixel-1] >= 0) { ui->neopixelGroupBox->setEnabled(true); } else { ui->neopixelGroupBox->setEnabled(false); }
            ui->neopixelStrandLengthBox->setValue(settingsTable[customLEDcount]);
            ui->customLEDstaticSpinbox->setValue(settingsTable[customLEDstatic]);
            ui->customLEDstaticBtn1->setStyleSheet(QString("background-color: #%1").arg(settingsTable[customLEDcolor1], 6, 16, QLatin1Char('0')));
            ui->customLEDstaticBtn2->setStyleSheet(QString("background-color: #%1").arg(settingsTable[customLEDcolor2], 6, 16, QLatin1Char('0')));
            ui->customLEDstaticBtn3->setStyleSheet(QString("background-color: #%1").arg(settingsTable[customLEDcolor3], 6, 16, QLatin1Char('0')));

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
    for(uint8_t i = 0; i < 30; i++) {
        pinBoxes[i]->addItems(valuesNameList);
        // clear out analog options for digital pins (< GPIO26)
        if(i < 26) {
            pinBoxes[i]->removeItem(tempPin);
            pinBoxes[i]->removeItem(analogY);
            pinBoxes[i]->removeItem(analogX);
        }
        // filter out SCL/SDA if possible.
        if(i & 1) {
            pinBoxes[i]->removeItem(camSDA);
            pinBoxes[i]->insertSeparator(camSDA);
            pinBoxes[i]->removeItem(periphSDA);
            pinBoxes[i]->insertSeparator(periphSDA);
        } else {
            pinBoxes[i]->removeItem(camSCL);
            pinBoxes[i]->insertSeparator(camSCL);
            pinBoxes[i]->removeItem(periphSCL);
            pinBoxes[i]->insertSeparator(periphSCL);
        }
    }
    ui->presetsBox->clear();
    if(boardCustomPresetsCount[board.type]) {
        ui->presetsBox->setHidden(false);
        switch(board.type) {
        case rpipico:
        case rpipicow:
            ui->presetsBox->setEnabled(true);
            ui->presetsBox->addItems(rpipicoPresetsList);
            break;
        case adafruitItsyRP2040:
            ui->presetsBox->setEnabled(true);
            ui->presetsBox->addItems(adafruitItsyBitsyRP2040PresetsList);
            break;
        default:
            ui->presetsBox->setEnabled(false);
        }
    } else {
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
            if(inputsMap[i] >= 0) {
                testLabel[i]->setText(valuesNameList[i+1]);
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText(valuesNameList[i+1] + " (N/C)");
                testLabel[i]->setEnabled(false);
            }
        } else if(i == 14) {
            if(inputsMap[tempPin-1] >= 0) {
                testLabel[i]->setText("Temp Read...");
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText("Temp (N/C)");
                testLabel[i]->setEnabled(false);
            }
            testLabel[i]->setStyleSheet("");
        } else if(i == 15) {
            if(inputsMap[analogX-1] >=0 && inputsMap[analogY-1] >= 0) {
                testLabel[i]->setText("Analog");
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText("Analog (N/C)");
                testLabel[i]->setEnabled(false);
            }
            testLabel[i]->setStyleSheet("");
        }
    }
    if(inputsMap[ledR-1] >= 0) { ui->redLedTestBtn->setEnabled(true); } else { ui->redLedTestBtn->setEnabled(false); }
    if(inputsMap[ledG-1] >= 0) { ui->greenLedTestBtn->setEnabled(true); } else { ui->greenLedTestBtn->setEnabled(false); }
    if(inputsMap[ledB-1] >= 0) { ui->blueLedTestBtn->setEnabled(true); } else { ui->blueLedTestBtn->setEnabled(false); }
}

void guiWindow::pinBoxes_activated(int index)
{
    // Demultiplexing to figure out which "pin" this combobox that's calling correlates to.
    uint8_t pin;
    QObject* obj = sender();
    for(uint8_t i = 0;;i++) {
        if(obj == pinBoxes[i]) {
                pin = i;
                break;
            }
    }

    if(ui->presetsBox->currentIndex() > -1) {
        ui->presetsBox->setCurrentIndex(-1);
    }

    // if it's being set to 0 (unmapped), unmap this pin without question.
    if(!index) {
        inputsMap[currentPins.value(pin) - 1] = -1;
        currentPins[pin] = btnUnmapped;
    // else, it's a function, so check for duplicates
    } else if(currentPins[pin] != index) {
        int8_t btnRequest = index - 1;

        // Scorched Earth approach, clear anything that matches so that it's unmapped.
        inputsMap[btnRequest] = -1;
        // only reset if current pin was already mapped.
        if(currentPins.value(pin) > 0) {
            inputsMap[currentPins.value(pin) - 1] = -1;
        }
        // if function is I2C, check for other things
        if(index == camSDA) {
            // if it's mapped, check that cam clock pin isn't mapped to the opposite I2C channel
            if(inputsMap[camSCL-1] > -1 && (pin & 0b00000010) != (inputsMap[camSCL-1] & 0b00000010)) {
                // channels mismatched, unmap the other pin
                currentPins[inputsMap[camSCL-1]] = btnUnmapped;
                pinBoxes[inputsMap[camSCL-1]]->setCurrentIndex(btnUnmapped);
                ui->statusBar->showMessage("Camera pins are not on the same I2C channel! Please check camera pin mappings.", 10000);
            // check that peripheral data isn't mapped to this I2C channel
            } else if(inputsMap[periphSDA-1] > -1 && (pin & 0b00000010) == (inputsMap[periphSDA-1] & 0b00000010)) {
                // channels matched, unmap peripheral data
                currentPins[inputsMap[periphSDA-1]] = btnUnmapped;
                pinBoxes[inputsMap[periphSDA-1]]->setCurrentIndex(btnUnmapped);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Peripheral Data.", 10000);
            }
        } else if(index == camSCL) {
            // if it's mapped, check that current cam clock pin isn't mapped to the opposite I2C channel
            if(inputsMap[camSDA-1] > -1 && (pin & 0b00000010) != (inputsMap[camSDA-1] & 0b00000010)) {
                // channels mismatched, unmap the other pin
                currentPins[inputsMap[camSDA-1]] = btnUnmapped;
                pinBoxes[inputsMap[camSDA-1]]->setCurrentIndex(btnUnmapped);
                ui->statusBar->showMessage("Camera pins are not on the same I2C channel! Please check camera pin mappings.", 10000);
            // check that peripheral clock isn't mapped to this I2C channel
            } else if(inputsMap[periphSCL-1] > -1 && (pin & 0b00000010) == (inputsMap[periphSCL-1] & 0b00000010)) {
                // channels matched, unmap peripheral clock
                currentPins[inputsMap[periphSCL-1]] = btnUnmapped;
                pinBoxes[inputsMap[periphSCL-1]]->setCurrentIndex(btnUnmapped);
                ui->statusBar->showMessage("Camera and Peripheral Clock pins clashed! Please remap Peripheral Clock.", 10000);
            }
        } else if(index == periphSDA) {
            // if it's mapped, check that current peripheral clock pin isn't mapped to the opposite I2C channel
            if(inputsMap[periphSCL-1] > -1 && (pin & 0b00000010) != (inputsMap[periphSCL-1] & 0b00000010)) {
                // channels mismatched, unmap the other pin
                currentPins[inputsMap[periphSCL-1]] = btnUnmapped;
                pinBoxes[inputsMap[periphSCL-1]]->setCurrentIndex(btnUnmapped);
                ui->statusBar->showMessage("Peripheral pins are not on the same I2C channel! Please check peripheral pin mappings.", 10000);
            // check that cam data isn't mapped to this I2C channel
            } else if(inputsMap[camSDA-1] > -1 && (pin & 0b00000010) == (inputsMap[camSDA-1] & 0b00000010)) {
                // channels matched, unmap cam data
                currentPins[inputsMap[camSDA-1]] = btnUnmapped;
                pinBoxes[inputsMap[camSDA-1]]->setCurrentIndex(btnUnmapped);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Camera Data.", 10000);
            }
        } else if(index == periphSCL) {
            // if it's mapped, check that current peripheral data pin isn't mapped to the opposite I2C channel
            if(inputsMap[periphSDA-1] > -1 && (pin & 0b00000010) != (inputsMap[periphSDA-1] & 0b00000010)) {
                // channels mismatched, unmap the other pin
                currentPins[inputsMap[periphSDA-1]] = btnUnmapped;
                pinBoxes[inputsMap[periphSDA-1]]->setCurrentIndex(btnUnmapped);
                ui->statusBar->showMessage("Peripheral pins are not on the same I2C channel! Please check peripheral pin mappings.", 10000);
            // check that cam clock isn't mapped to this I2C channel
            } else if(inputsMap[camSCL-1] > -1 && (pin & 0b00000010) == (inputsMap[camSCL-1] & 0b00000010)) {
                // channels matched, unmap cam clock
                currentPins[inputsMap[camSCL-1]] = btnUnmapped;
                pinBoxes[inputsMap[camSCL-1]]->setCurrentIndex(btnUnmapped);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Camera Clock.", 10000);
            }
        }
        QList<uint8_t> foundList = currentPins.keys(index);
        for(uint8_t i = 0; i < foundList.length(); i++) {
            currentPins[foundList[i]] = btnUnmapped;
            pinBoxes[foundList[i]]->setCurrentIndex(btnUnmapped);
        }
        // Then map the thing.
        currentPins[pin] = index;
        inputsMap[btnRequest] = pin;
    }

    // update settings panel to reflect pins map changes
    if(inputsMap[rumblePin-1] >= 0) { ui->rumbleToggle->setEnabled(true), ui->rumbleFFToggle->setEnabled(true); } else { ui->rumbleToggle->setChecked(false), ui->rumbleToggle->setEnabled(false), ui->rumbleFFToggle->setChecked(false), ui->rumbleFFToggle->setEnabled(false); }
    if(inputsMap[solenoidPin-1] >= 0) { ui->solenoidToggle->setEnabled(true); } else { ui->solenoidToggle->setChecked(false), ui->solenoidToggle->setEnabled(false); }
    if(inputsMap[neoPixel-1] >= 0) { ui->neopixelGroupBox->setEnabled(true); } else { ui->neopixelGroupBox->setEnabled(false); }
    if(inputsMap[ledR-1] >= 0 && inputsMap[ledG-1] >= 0 && inputsMap[ledB-1] >= 0) { ui->commonAnodeToggle->setEnabled(true); } else { ui->commonAnodeToggle->setEnabled(false); }

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
    // Demultiplexing to figure out which box we're using.
    uint8_t slot;
    QObject* obj = sender();
    for(uint8_t i = 0;;i++) {
        if(obj == renameBtn[i]) {
            slot = i;
            break;
        }
    }

    // TODO: limit character length in the text dialog - for now, just use up to 15 characters.
    QString newLabel = QInputDialog::getText(this, "Input Name", QString("Set name for profile %1").arg(slot+1));
    if(!newLabel.isEmpty()) {
        selectedProfile[slot]->setText(newLabel.left(15));
        profilesTable[slot].profName = newLabel.left(15);
    }
    DiffUpdate();
}


void guiWindow::colorBoxes_clicked()
{
    // Demultiplexing to figure out which box we're using.
    uint8_t slot;
    QObject* obj = sender();
    for(uint8_t i = 0;;i++) {
        if(obj == color[i]) {
            slot = i;
            break;
        }
    }

    QColor colorPick = QColorDialog::getColor(profilesTable[slot].color);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        profilesTable[slot].color = packedColor;
        color[slot]->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));
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
    boolSettings[customPins] = arg1;
    BoxesUpdate();
    if(inputsMap[solenoidPin-1] >= 0) {
        ui->solenoidToggle->setEnabled(true);
    } else {
        ui->solenoidToggle->setEnabled(false);
        ui->solenoidToggle->setChecked(false);
    }
    if(inputsMap[rumblePin-1] >= 0) {
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
        if(!ui->customPinsEnabled->isChecked()) { ui->customPinsEnabled->setChecked(true); }
        for(uint8_t i = 0; i < 30; i++) {
            pinBoxes[i]->setCurrentIndex(btnUnmapped);
            currentPins[i] = btnUnmapped;
        }
        for(uint8_t i = 0; i < boardInputsCount-1; i++) {
            switch(board.type) {
            case rpipico:
            case rpipicow:
                if(rpipicoPresets[index][i] > -1) {
                    pinBoxes[rpipicoPresets[index][i]]->setCurrentIndex(i+1);
                    currentPins[rpipicoPresets[index][i]] = i+1;
                }
                inputsMap[i] = rpipicoPresets[index][i];
                break;
            case adafruitItsyRP2040:
                if(adafruitItsyBitsyRP2040Presets[index][i] > -1) {
                    pinBoxes[adafruitItsyBitsyRP2040Presets[index][i]]->setCurrentIndex(i+1);
                    currentPins[adafruitItsyBitsyRP2040Presets[index][i]] = i+1;
                }
                inputsMap[i] = adafruitItsyBitsyRP2040Presets[index][i];
                break;
            default:
                // lol wut
                break;
            }
        }
        DiffUpdate();
    }
}


void guiWindow::on_rumbleToggle_stateChanged(int arg1)
{
    boolSettings[rumble] = arg1;
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
    if(!(arg1 && boolSettings[rumbleFF]) && !boolSettings[solenoid]) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_solenoidToggle_stateChanged(int arg1)
{
    boolSettings[solenoid] = arg1;
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
    if(!arg1 && !(boolSettings[rumble] && boolSettings[rumbleFF])) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_autofireToggle_stateChanged(int arg1)
{
    boolSettings[autofire] = arg1;
    if(arg1) { ui->autofireWaitFactorBox->setEnabled(true); } else { ui->autofireWaitFactorBox->setEnabled(false); }
    DiffUpdate();
}


void guiWindow::on_simplePauseToggle_stateChanged(int arg1)
{
    boolSettings[simplePause] = arg1;
    DiffUpdate();
}


void guiWindow::on_holdToPauseToggle_stateChanged(int arg1)
{
    boolSettings[holdToPause] = arg1;
    if(arg1) { ui->holdToPauseLengthBox->setEnabled(true); }
    else { ui->holdToPauseLengthBox->setEnabled(false); }
    DiffUpdate();
}


void guiWindow::on_commonAnodeToggle_stateChanged(int arg1)
{
    boolSettings[commonAnode] = arg1;
    DiffUpdate();
}


void guiWindow::on_lowButtonsToggle_stateChanged(int arg1)
{
    boolSettings[lowButtonsMode] = arg1;
    DiffUpdate();
}


void guiWindow::on_rumbleFFToggle_stateChanged(int arg1)
{
    boolSettings[rumbleFF] = arg1;
    if(arg1) { ui->solenoidToggle->setChecked(false); }
    if(!(arg1 && boolSettings[rumble]) && !boolSettings[solenoid]) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_rumbleIntensityBox_valueChanged(int arg1)
{
    settingsTable[rumbleStrength] = arg1;
    DiffUpdate();
}


void guiWindow::on_rumbleLengthBox_valueChanged(int arg1)
{
    settingsTable[rumbleInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_holdToPauseLengthBox_valueChanged(int arg1)
{
    settingsTable[holdToPauseLength] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidNormalIntervalBox_valueChanged(int arg1)
{
    settingsTable[solenoidNormalInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidFastIntervalBox_valueChanged(int arg1)
{
    settingsTable[solenoidFastInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidHoldLengthBox_valueChanged(int arg1)
{
    settingsTable[solenoidHoldLength] = arg1;
    DiffUpdate();
}


void guiWindow::on_autofireWaitFactorBox_valueChanged(int arg1)
{
    settingsTable[autofireWaitFactor] = arg1;
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
    settingsTable[customLEDcount] = arg1;
    if(arg1 < settingsTable[customLEDstatic]) {
        ui->customLEDstaticSpinbox->setValue(arg1);
    }

    // show NeoPixel notice if values are updated
    PixelsDiff();

    DiffUpdate();
}


void guiWindow::on_customLEDstaticSpinbox_valueChanged(int arg1)
{
    if(arg1 > settingsTable[customLEDcount]) { ui->customLEDstaticSpinbox->setValue(settingsTable[customLEDcount]); }
    else { settingsTable[customLEDstatic] = arg1; }
    if(customLEDstatic) {
        switch(settingsTable[customLEDstatic]) {
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
    QColor colorPick = QColorDialog::getColor(settingsTable[customLEDcolor1]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        settingsTable[customLEDcolor1] = packedColor;
        ui->customLEDstaticBtn1->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}


void guiWindow::on_customLEDstaticBtn2_clicked()
{
    QColor colorPick = QColorDialog::getColor(settingsTable[customLEDcolor2]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        settingsTable[customLEDcolor2] = packedColor;
        ui->customLEDstaticBtn2->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}


void guiWindow::on_customLEDstaticBtn3_clicked()
{
    QColor colorPick = QColorDialog::getColor(settingsTable[customLEDcolor3]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        settingsTable[customLEDcolor3] = packedColor;
        ui->customLEDstaticBtn3->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}

// TODO: cali should use a fullscreen window depicting target graphics w/ hidden cursor. This should be its own method and activated when "Cali:" is detected in the serial stream.
void guiWindow::on_calib1Btn_clicked()
{
    serialPort.write("XC1C");
    if(serialPort.waitForBytesWritten(1000)) {
        PopupWindow("Calibrating Profile 1.", "Aim the gun at the cursor in the center of the display and pull the trigger, then shoot at the four edges of the display that the mouse moves to.\nYou can exit without saving changes by pressing either Button A/B/C.\n\nAfter the final center target, verify that the new calibration is to your liking; press the trigger to confirm, Button A/B to restart calibration, or Button C to exit calibration without any changes.", "Calibration", 2);
    }
}


void guiWindow::on_calib2Btn_clicked()
{
    serialPort.write("XC2C");
    if(serialPort.waitForBytesWritten(1000)) {
        PopupWindow("Calibrating Profile 2.", "Aim the gun at the cursor in the center of the display and pull the trigger, then shoot at the four edges of the display that the mouse moves to.\nYou can exit without saving changes by pressing either Button A/B/C.\n\nAfter the final center target, verify that the new calibration is to your liking; press the trigger to confirm, Button A/B to restart calibration, or Button C to exit calibration without any changes.", "Calibration", 2);
    }
}


void guiWindow::on_calib3Btn_clicked()
{
    serialPort.write("XC3C");
    if(serialPort.waitForBytesWritten(1000)) {
        PopupWindow("Calibrating Profile 3.", "Aim the gun at the cursor in the center of the display and pull the trigger, then shoot at the four edges of the display that the mouse moves to.\nYou can exit without saving changes by pressing either Button A/B/C.\n\nAfter the final center target, verify that the new calibration is to your liking; press the trigger to confirm, Button A/B to restart calibration, or Button C to exit calibration without any changes.", "Calibration", 2);
    }
}


void guiWindow::on_calib4Btn_clicked()
{
    serialPort.write("XC4C");
    if(serialPort.waitForBytesWritten(1000)) {
        PopupWindow("Calibrating Profile 4.", "Aim the gun at the cursor in the center of the display and pull the trigger, then shoot at the four edges of the display that the mouse moves to.\nYou can exit without saving changes by pressing either Button A/B/C.\n\nAfter the final center target, verify that the new calibration is to your liking; press the trigger to confirm, Button A/B to restart calibration, or Button C to exit calibration without any changes.", "Calibration", 2);
    }
}

// WARNING: make sure "serialActive" is set ON for important operations, or this will eat the fucker
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
    if(!serialPort.waitForBytesWritten(1000)) { PopupWindow("Lost connection!", "Somehow this happened I guess???", "Oops!", 4); }
    else { ui->statusBar->showMessage("Sent a rumble test pulse.", 2500); }
}


void guiWindow::on_solenoidTestBtn_clicked()
{
    serialPort.write("Xts");
    if(!serialPort.waitForBytesWritten(1000)) {
        PopupWindow("Lost connection!", "Somehow this happened I guess???", "Oops!", 4);
    } else {
        ui->statusBar->showMessage("Sent a solenoid test pulse.", 2500);
    }
}


void guiWindow::on_redLedTestBtn_clicked()
{
    serialPort.write("XtR");
    if(!serialPort.waitForBytesWritten(1000)) {
        PopupWindow("Lost connection!", "Somehow this happened I guess???", "Oops!", 4);
    } else {
        ui->statusBar->showMessage("Set LED to Red.", 2500);
    }
}


void guiWindow::on_greenLedTestBtn_clicked()
{
    serialPort.write("XtG");
    if(!serialPort.waitForBytesWritten(1000)) {
        PopupWindow("Lost connection!", "Somehow this happened I guess???", "Oops!", 4);
    } else {
        ui->statusBar->showMessage("Set LED to Green.", 2500);
    }
}


void guiWindow::on_blueLedTestBtn_clicked()
{
    serialPort.write("XtB");
    if(!serialPort.waitForBytesWritten(1000)) {
        PopupWindow("Lost connection!", "Somehow this happened I guess???", "Oops!", 4);
    } else {
        ui->statusBar->showMessage("Set LED to Blue.", 2500);
    }
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
                    PopupWindow("Cleared storage.", "Please unplug the board and reinsert it into the PC.", "Clear Finished", 1);
                }
            }
        }
    } else {
        //qDebug() << "Clear operation canceled.";
        ui->statusBar->showMessage("Clear operation canceled.", 3000);
    }
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

