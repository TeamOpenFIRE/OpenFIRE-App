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
#include "appabout.h"
#include "constants.h"
#include "ui_appmainwindow.h"

#include <QGraphicsScene>
#include <QMessageBox>
#include <QSvgRenderer>
#include <QSerialPortInfo>
#include <QProgressBar>
#include <QProcess>
//#include <QStorageInfo>
#include <QDebug>
#include <QFileDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>

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

    // Kickstart serial thread properly
    connect(&serial.serialWorker, &SerialThreadWorker::Worker_StartupFinished, this, &guiWindow::Worker_Started);
    emit serial.operate();

#if defined(OFAPP_VERSION) & defined(OFAPP_CODENAME)
    this->setWindowTitle("OpenFIRE App - " + OFAPP_CODENAME + " [v" + OFAPP_VERSION + ']');
#else
    this->setWindowTitle("OpenFIRE App - Tokinomiya [v3.0-dev]");
#endif // OFAPP_VERSION

    // get all fixed interactable elements marked to use event filter for hover stuff:
    for(const auto child : this->findChildren<QPushButton*>())
        if(!child->property("trackable").isNull()) child->installEventFilter(this);

    for(const auto child : this->findChildren<QCheckBox*>())
        if(!child->property("trackable").isNull()) child->installEventFilter(this);

    for(const auto child : this->findChildren<QLineEdit*>())
        if(!child->property("trackable").isNull()) child->installEventFilter(this);

    for(const auto child : this->findChildren<QSpinBox*>())
        if(!child->property("trackable").isNull()) child->installEventFilter(this);

    for(const auto child : this->findChildren<QComboBox*>())
        if(!child->property("trackable").isNull()) child->installEventFilter(this);

    for(const auto child : this->findChildren<QRadioButton*>())
        if(!child->property("trackable").isNull()) child->installEventFilter(this);

    // Connect boards view "custom layouts" actions to the button
    ui->customLayoutToolBtn->addActions({ui->actionImport_Custom_Layout, ui->actionExport_Custom_Layout});

    // Add center board pic above the Sub Pins layout
    ui->PinsCenter->insertWidget(0, &boardPic, 1);
    boardPic.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Setup test screen buttons
    for(int i = 0; i < 16; i++) {
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
        if(i == 15)      ui->buttonsTestLayout->addWidget(testLabel[i], 3, 3);
        // temp sensor
        else if(i == 14) ui->buttonsTestLayout->addWidget(testLabel[i], 3, 1);
        // third/second/first row of buttons
        else if(i > 9)   ui->buttonsTestLayout->addWidget(testLabel[i], 2, i-10);
        else if(i > 4)   ui->buttonsTestLayout->addWidget(testLabel[i], 1, i-5);
        else             ui->buttonsTestLayout->addWidget(testLabel[i], 0, i);
    }

    ui->buttonsTestLayout->setRowMinimumHeight(0, 32);
    ui->buttonsTestLayout->setRowMinimumHeight(1, 32);
    ui->buttonsTestLayout->setRowMinimumHeight(2, 32);
    ui->buttonsTestLayout->setRowMinimumHeight(3, 32);

    // hiding tUSB elements by default since this can't be done from the off
    ui->tUSBLayoutAdvanced->setVisible(false);

    // set hidden by default until a board with presets is loaded
    ui->presetsBox->setHidden(true);

    aliveTimer = new QTimer();
    connect(aliveTimer, &QTimer::timeout, this, &guiWindow::aliveTimer_timeout);

    statusBar()->showMessage("Welcome to the OpenFIRE app!", 3000);

    statusProgressBar = new QProgressBar();
    statusProgressBar->setVisible(false);
    ui->statusBar->addPermanentWidget(statusProgressBar);

    // light mode styling adjustments:
    if(this->palette().window().color().value() > this->palette().text().color().value()) {
        ui->settingsDescBox->setStyleSheet("QGroupBox::title { color: #909000 }");
        ui->settingsDescText->setStyleSheet("color: doubledarkgray");
        ui->profilesDescBox->setStyleSheet("QGroupBox::title { color: #909000 }");
        ui->profilesDescText->setStyleSheet("color: doubledarkgray");
    }
}

guiWindow::~guiWindow()
{
    if(ui->comPortSelector->currentIndex() > 0) {
        statusBar()->showMessage("Sending undock request to board...");
        emit serial.Disconnect();
    }

    serial.SerialEnd();

    delete ui;
}


bool guiWindow::eventFilter(QObject* object, QEvent* event)
{
    if(event->type() == QEvent::Enter) {
        switch(object->property("trackable").toInt()) {
        case App_Const::trackPinbox:
        {
            // Copy and modify board pic array to change opacity of selected pin element, if existing.
            highlightBoardPic = origBoardPicFile;
            highlightBoardPic.replace(QString("id=\"OF_pin%1\"\nstyle=\"opacity:0").arg(object->property("slot").toInt()),
                                      QString("id=\"OF_pin%1\"\nstyle=\"opacity:1").arg(object->property("slot").toInt()));

            boardPic.load(highlightBoardPic.toLocal8Bit());
            boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
            break;
        }
        case App_Const::trackSettingsItem:
            ui->settingsDescBox->setTitle(object->property("accessibleName").toString());
            ui->settingsDescText->setText(object->property("whatsThis").toString());
            break;
        case App_Const::trackProfileItem:
            ui->profilesDescBox->setTitle(object->property("accessibleName").toString());
            ui->profilesDescText->setText(object->property("whatsThis").toString());
            break;
        case App_Const::trackTestItem:
            break;
        }
    } else if(event->type() == QEvent::Leave) {
        if(object->property("trackable").toInt() == App_Const::trackPinbox) {
            boardPic.load(origBoardPicFile);
            boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
        }
    }

    // disable QComboBox scroll events (to prevent accidental pinbox index changing)
    if(!(event->type() == QEvent::Wheel && object->inherits("QComboBox")))
        return QWidget::eventFilter(object, event);
    else return true;
}


void guiWindow::Worker_Started()
{
    // Connect up Serial thread signals to the main app window
    connect(serial.serialWorker.port, &QSerialPort::readyRead, this, &guiWindow::serialPort_readyRead);
    connect(&serial.serialWorker, &SerialThreadWorker::SearchPorts_Result, this, &guiWindow::serialPort_handleResult);
    connect(&serial.serialWorker, &SerialThreadWorker::GetSettings_Result, this, &guiWindow::serialPort_handleResult);
    connect(&serial.serialWorker, &SerialThreadWorker::OneShot_Result,     this, &guiWindow::serialPort_handleResult);
    connect(&serial.serialWorker, &SerialThreadWorker::Commit_Result,      this, &guiWindow::serialPort_handleResult);
    connect(&serial.serialWorker, &SerialThreadWorker::Disconnect_Result,  this, &guiWindow::serialPort_handleResult);
    connect(&serial.serialWorker, &SerialThreadWorker::Serial_SetProgressRange, this, &guiWindow::serialPort_progressSet);
    connect(&serial.serialWorker, &SerialThreadWorker::Serial_ProgressUpdate, this, &guiWindow::serialPort_progressUpdate);

    serial.SearchPorts();

    aliveTimer->start(ALIVE_TIMER);
}


void guiWindow::BoxesUpdate()
{
    // enabling custom pins
    if(App_Const::boolSettings[OF_Const::customPins]) {
        // enable pinboxes
        for(int i = 0; i < pinBoxes.count(); i++)
            pinBoxes.at(i)->setEnabled(true);

        // if the custom pins setting *grabbed from the gun* has been set
        if(App_Const::boolSettings_orig[OF_Const::customPins]) {
            // reset pinboxes
            for(int i = 0; i < pinBoxes.count(); i++)
                pinBoxes.at(i)->setCurrentIndex(OF_Const::btnUnmapped+1);

            // set pinboxes to copied values (pinbox index is off by 1)
            for(int i = 0; i < App_Const::inputsMap_orig.count(); i++)
                if(App_Const::inputsMap_orig.value(i) > OF_Const::btnUnmapped && App_Const::inputsMap_orig.value(i) < pinBoxes.count())
                    pinBoxes.at(App_Const::inputsMap_orig.value(i))->setCurrentIndex(i+1);

        // else, if the board *was using default maps* before switching to custom (no need to re-set pinboxes)
        } else {
            // copy original map, which clears this map (as boards using defaults comes with no actual map instated)
            App_Const::inputsMap = App_Const::inputsMap_orig;

            // copy presets to inputs map
            if(OF_Const::boardsPresetsMap.count(App_Const::board.boardType.toStdString())) {
                for(int i = 0; i < PINS_COUNT; i++) {
                    if(OF_Const::boardsPresetsMap.at(App_Const::board.boardType.toStdString()).pin[i] > OF_Const::btnUnmapped) {
                        App_Const::inputsMap[OF_Const::boardsPresetsMap.at(App_Const::board.boardType.toStdString()).pin[i]] = i;
                    }
                }
            }
        }

        return;

    // disabling custom pins, reset to presets
    } else {
        // reset inputs map, as it's not even referenced when custom pins are disabled
        for(int i = 0; i < PINS_COUNT; i++)
            pinBoxes.at(i)->setEnabled(false), pinBoxes.at(i)->setCurrentIndex(OF_Const::btnUnmapped+1);

        // copy preset layout to pinboxes
        if(OF_Const::boardsPresetsMap.count(App_Const::board.boardType.toStdString()))
            for(int i = 0; i < PINS_COUNT; i++)
                pinBoxes.at(i)->setCurrentIndex(OF_Const::boardsPresetsMap.at(App_Const::board.boardType.toStdString()).pin[i]+1);

        // generics don't come with mappings
        else for(int i = 0; i < PINS_COUNT; i++)
            pinBoxes.at(i)->setCurrentIndex(OF_Const::btnUnmapped+1);

        return;
    }
}


void guiWindow::DiffUpdate()
{
    int settingsDiff = 0;

    if(App_Const::boolSettings_orig[OF_Const::customPins] != App_Const::boolSettings[OF_Const::customPins])
        settingsDiff++;

    if(App_Const::boolSettings[OF_Const::customPins])
        if(App_Const::inputsMap_orig != App_Const::inputsMap)
            settingsDiff++;

    for(uint8_t i = 1; i < OF_Const::boolTypesCount; i++)
        if(App_Const::boolSettings_orig[i] != App_Const::boolSettings[i])
            settingsDiff++;

    for(uint8_t i = 0; i < OF_Const::settingsTypesCount; i++)
        if(App_Const::settingsTable_orig[i] != App_Const::settingsTable[i])
            settingsDiff++;

    if(App_Const::tinyUSBtable_orig.tinyUSBid != App_Const::tinyUSBtable.tinyUSBid)
        settingsDiff++;

    if(App_Const::tinyUSBtable_orig.tinyUSBname != App_Const::tinyUSBtable.tinyUSBname)
        settingsDiff++;

    if(App_Const::board.selectedProfile != App_Const::board.previousProfile)
        settingsDiff++;

    for(uint8_t i = 0; i < PROFILES_COUNT; i++) {
        if(App_Const::profilesTable_orig[i].profName != App_Const::profilesTable[i].profName)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].topOffset != App_Const::profilesTable[i].topOffset)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].bottomOffset != App_Const::profilesTable[i].bottomOffset)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].leftOffset != App_Const::profilesTable[i].leftOffset)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].rightOffset != App_Const::profilesTable[i].rightOffset)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].TLled != App_Const::profilesTable[i].TLled)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].TRled != App_Const::profilesTable[i].TRled)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].irSensitivity != App_Const::profilesTable[i].irSensitivity)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].runMode != App_Const::profilesTable[i].runMode)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].layoutType != App_Const::profilesTable[i].layoutType)
            settingsDiff++;

        if(App_Const::profilesTable_orig[i].color != App_Const::profilesTable[i].color)
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


QString guiWindow::PrettifyName(QString name)
{
    if(name.isEmpty())
        name = "Unnamed Device";

    // append name of board to gun name string.
    if(OF_Const::boardNames.contains(App_Const::board.boardType.toStdString()))
         return name + " | " + OF_Const::boardNames[App_Const::board.boardType.toStdString()];
    else return name + " | " + OF_Const::boardNames["generic"];
}


void guiWindow::PixelsDiff()
{
    if( App_Const::settingsTable[OF_Const::customLEDcount]  == App_Const::settingsTable_orig[OF_Const::customLEDcount]  &&
        App_Const::settingsTable[OF_Const::customLEDstatic] == App_Const::settingsTable_orig[OF_Const::customLEDstatic] &&
        App_Const::settingsTable[OF_Const::customLEDcolor1] == App_Const::settingsTable_orig[OF_Const::customLEDcolor1] &&
        App_Const::settingsTable[OF_Const::customLEDcolor2] == App_Const::settingsTable_orig[OF_Const::customLEDcolor2] &&
        App_Const::settingsTable[OF_Const::customLEDcolor3] == App_Const::settingsTable_orig[OF_Const::customLEDcolor3]) {
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
        serialActive = true;

        ui->tabWidget->setEnabled(false);
        ui->comPortSelector->setEnabled(false);
        ui->confirmButton->setEnabled(false);

        emit serial.CommitSettings();
    } else { statusBar()->showMessage("Save operation canceled.", 3000); }
}


void guiWindow::aliveTimer_timeout()
{
    emit serial.SearchPorts();
}


void guiWindow::on_comPortSelector_currentTextChanged(const QString &text)
{
    // Clear stale states if any
    if(testMode)
        if(caliWindow != nullptr)
            caliWindow->Shutdown();

    // unmount old board if mounted
    emit serial.Disconnect();

    if(ui->comPortSelector->currentIndex() > 0) {
        printf("COM port set to %d\n", ui->comPortSelector->currentIndex());

        // try to init serial port
        // if returns false, it failed, so just turn the index back to initial.

        emit serial.GetSettings(ui->comPortSelector->currentText());
    } else {
        ui->boardLabel->clear();
        ui->versionLabel->clear();

        emit serial.Disconnect();
    }
}


// Only runs either on initial load or save
void guiWindow::LabelsUpdate()
{
    // because App_Const::inputsMap uses pin no. starting from 0
    for(uint8_t i = 0; i < 16; i++) {
        if(i < 14) {
            if(App_Const::inputsMap.value(i) >= 0) {
                testLabel[i]->setText(OF_Const::valuesNameList[i+1]);
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText(OF_Const::valuesNameList[i+1] + " (N/C)");
                testLabel[i]->setEnabled(false);
            }
        } else if(i == 14) {
            if(App_Const::inputsMap.value(OF_Const::tempPin) >= 0) {
                testLabel[i]->setText("Temp Read...");
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText("Temp (N/C)");
                testLabel[i]->setEnabled(false);
            }
            testLabel[i]->setStyleSheet("");
        } else if(i == 15) {
            if(App_Const::inputsMap.value(OF_Const::analogX) >=0 && App_Const::inputsMap.value(OF_Const::analogY) >= 0) {
                testLabel[i]->setText("Analog");
                testLabel[i]->setEnabled(true);
            } else {
                testLabel[i]->setText("Analog (N/C)");
                testLabel[i]->setEnabled(false);
            }
            testLabel[i]->setStyleSheet("");
        }
    }
    if(App_Const::inputsMap.value(OF_Const::ledR) >= 0) ui->redLedTestBtn->setEnabled(true);   else ui->redLedTestBtn->setEnabled(false);
    if(App_Const::inputsMap.value(OF_Const::ledG) >= 0) ui->greenLedTestBtn->setEnabled(true); else ui->greenLedTestBtn->setEnabled(false);
    if(App_Const::inputsMap.value(OF_Const::ledB) >= 0) ui->blueLedTestBtn->setEnabled(true);  else ui->blueLedTestBtn->setEnabled(false);

    ui->boardLabel->setText(PrettifyName(App_Const::tinyUSBtable.tinyUSBname));
}

void guiWindow::pinBoxes_currentIndexChanged(int index)
{
    // using comboboxes' "slot" property to figure caller,
    // and "prevMapping" to get previous index, as this method immediately overwrites what it was mapped to.
    // always remember to sync the change to "prevMapping" property at the end of its logic path!

    /*
    if(index >= 0 && index <= App_Const::inputsMap.size()) {
        //printf("Requesting pinbox %d to set to %s\n", sender()->property("slot").toInt(), OF_Const::valuesNameList.at(index).toLocal8Bit().constData());
    } else printf("Oops! Seems like pinbox %d is trying to set itself to index %d, which is out of range!\n", sender()->property("slot").toInt(), index);
    //*/

    // reset presets box, as it's no longer accurate for this layout
    if(ui->presetsBox->currentIndex() > -1)
        ui->presetsBox->setCurrentIndex(-1);

    // if it's being set to 0 (unmapped), unmap this pin without question.
    if(index <= 0) {
        if(sender()->property("prevMapping").toInt() > OF_Const::btnUnmapped+1) {
            App_Const::inputsMap[sender()->property("prevMapping").toInt()-1] = OF_Const::btnUnmapped;
            sender()->setProperty("prevMapping", OF_Const::btnUnmapped+1);
        }

    // else, it's a function, so check for duplicates
    } else if(sender()->property("prevMapping").toInt() != index) {
        int8_t btnRequest = index - 1;

        // unmap pinbox's previous function, if mapped to any
        if(sender()->property("prevMapping").toInt() > OF_Const::btnUnmapped+1)
            App_Const::inputsMap[sender()->property("prevMapping").toInt()-1] = OF_Const::btnUnmapped;

        // Remove whatever pin mapping that this function belonged to, if it was mapped
        // (making sure we don't disable the pin trying to be set)
        if(App_Const::inputsMap.value(btnRequest)  > OF_Const::btnUnmapped &&
           App_Const::inputsMap.value(btnRequest) != sender()->property("slot").toInt())
            pinBoxes.at(App_Const::inputsMap.value(btnRequest))->setCurrentIndex(OF_Const::btnUnmapped+1);

        // if function is I2C, check for other things
        if(btnRequest == OF_Const::camSDA) {
            // if it's mapped, check that cam clock pin isn't mapped to the opposite I2C channel
            if(App_Const::inputsMap.value(OF_Const::camSCL) > OF_Const::btnUnmapped &&
               (sender()->property("slot").toInt() & 0b00000010) != (App_Const::inputsMap.value(OF_Const::camSCL) & 0b00000010)) {
                // channels mismatched, unmap the other pin
                pinBoxes.at(App_Const::inputsMap.value(OF_Const::camSCL))->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera pins are not on the same I2C channel! Please check camera pin mappings.", 10000);
            // check that peripheral data isn't mapped to this I2C channel
            } else if(App_Const::inputsMap.value(OF_Const::periphSDA) > OF_Const::btnUnmapped &&
                      (sender()->property("slot").toInt() & 0b00000010) == (App_Const::inputsMap.value(OF_Const::periphSDA) & 0b00000010)) {
                // channels matched, unmap peripheral data
                pinBoxes.at(App_Const::inputsMap.value(OF_Const::periphSDA))->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Peripheral Data.", 10000);
            }
        } else if(btnRequest == OF_Const::camSCL) {
            // if it's mapped, check that cam clock pin isn't mapped to the opposite I2C channel
            if(App_Const::inputsMap.value(OF_Const::camSDA) > OF_Const::btnUnmapped &&
               (sender()->property("slot").toInt() & 0b00000010) != (App_Const::inputsMap.value(OF_Const::camSDA) & 0b00000010)) {
                // channels mismatched, unmap the other pin
                pinBoxes.at(App_Const::inputsMap.value(OF_Const::camSDA))->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera pins are not on the same I2C channel! Please check camera pin mappings.", 10000);
            // check that peripheral clock isn't mapped to this I2C channel
            } else if(App_Const::inputsMap.value(OF_Const::periphSCL) > OF_Const::btnUnmapped &&
                      (sender()->property("slot").toInt() & 0b00000010) == (App_Const::inputsMap.value(OF_Const::periphSCL) & 0b00000010)) {
                // channels matched, unmap peripheral clock
                pinBoxes.at(App_Const::inputsMap.value(OF_Const::periphSCL))->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera and Peripheral Clock pins clashed! Please remap Peripheral Clock.", 10000);
            }
        } else if(btnRequest == OF_Const::periphSDA) {
            // if it's mapped, check that current peripheral clock pin isn't mapped to the opposite I2C channel
            if(App_Const::inputsMap.value(OF_Const::periphSCL) > OF_Const::btnUnmapped &&
               (sender()->property("slot").toInt() & 0b00000010) != (App_Const::inputsMap.value(OF_Const::periphSCL) & 0b00000010)) {
                // channels mismatched, unmap the other pin
                pinBoxes.at(App_Const::inputsMap.value(OF_Const::periphSCL))->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Peripheral pins are not on the same I2C channel! Please check peripheral pin mappings.", 10000);
            // check that cam data isn't mapped to this I2C channel
            } else if(App_Const::inputsMap.value(OF_Const::camSDA) > OF_Const::btnUnmapped &&
                      (sender()->property("slot").toInt() & 0b00000010) == (App_Const::inputsMap.value(OF_Const::camSDA) & 0b00000010)) {
                // channels matched, unmap cam data
                pinBoxes.at(App_Const::inputsMap.value(OF_Const::camSDA))->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Camera Data.", 10000);
            }
        } else if(btnRequest == OF_Const::periphSCL) {
            // if it's mapped, check that current peripheral data pin isn't mapped to the opposite I2C channel
            if(App_Const::inputsMap.value(OF_Const::periphSDA) > OF_Const::btnUnmapped &&
               (sender()->property("slot").toInt() & 0b00000010) != (App_Const::inputsMap.value(OF_Const::periphSDA) & 0b00000010)) {
                // channels mismatched, unmap the other pin
                pinBoxes.at(App_Const::inputsMap.value(OF_Const::periphSDA))->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Peripheral pins are not on the same I2C channel! Please check peripheral pin mappings.", 10000);
            // check that cam clock isn't mapped to this I2C channel
            } else if(App_Const::inputsMap.value(OF_Const::camSCL) > OF_Const::btnUnmapped &&
                      (sender()->property("slot").toInt() & 0b00000010) == (App_Const::inputsMap.value(OF_Const::camSCL) & 0b00000010)) {
                // channels matched, unmap cam clock
                pinBoxes.at(App_Const::inputsMap.value(OF_Const::camSCL))->setCurrentIndex(OF_Const::btnUnmapped+1);
                ui->statusBar->showMessage("Camera and Peripheral Data pins clashed! Please remap Camera Clock.", 10000);
            }
        }

        // Then map the thing, and sync this change to the property value
        App_Const::inputsMap[btnRequest] = sender()->property("slot").toInt();
        sender()->setProperty("prevMapping", index);
    }

    // update settings panel to reflect pins map changes and prevent illegal values/combinations
    if(App_Const::inputsMap.value(OF_Const::rumblePin) >= 0)
        ui->rumbleToggle->setEnabled(true), ui->rumbleFFToggle->setEnabled(true);
    else {
        ui->rumbleToggle->setChecked(false),   ui->rumbleToggle->setEnabled(false),
        ui->rumbleFFToggle->setChecked(false), ui->rumbleFFToggle->setEnabled(false);
    }

    if(App_Const::inputsMap.value(OF_Const::solenoidPin) >= 0)
        ui->solenoidToggle->setEnabled(true);
    else ui->solenoidToggle->setChecked(false), ui->solenoidToggle->setEnabled(false);

    if(App_Const::inputsMap.value(OF_Const::neoPixel) >= 0)
        ui->neopixelGroupBox->setEnabled(true);
    else ui->neopixelGroupBox->setEnabled(false);

    if(App_Const::inputsMap.value(OF_Const::ledR) >= 0 && App_Const::inputsMap.value(OF_Const::ledG) >= 0 && App_Const::inputsMap.value(OF_Const::ledB) >= 0)
        ui->commonAnodeToggle->setEnabled(true);
    else ui->commonAnodeToggle->setEnabled(false);

    DiffUpdate();
}

void guiWindow::profileBoxes_activated(int index)
{
    switch(sender()->property("type").toInt()) {
    case App_Const::pBoxIRsens:
        App_Const::profilesTable[sender()->property("slot").toInt()].irSensitivity = index;
        break;
    case App_Const::pBoxRunMode:
        App_Const::profilesTable[sender()->property("slot").toInt()].runMode = index;
        break;
    case App_Const::pBoxLayout:
        App_Const::profilesTable[sender()->property("slot").toInt()].layoutType = index;
        break;
    default:
        break;
    }

    DiffUpdate();
}


void guiWindow::renameBoxes_clicked()
{
    // TODO: limit character length in the text dialog - for now, just use up to 15 characters.
    QString newLabel = QInputDialog::getText(this,
                                             "Input Name",
                                             QString("Set name for Calibration Profile %1").arg(sender()->property("slot").toInt()+1));

    if(!newLabel.isEmpty()) {
        selectedProfile[sender()->property("slot").toInt()]->setText(QString("%1. %2").arg(sender()->property("slot").toInt()).arg(newLabel.left(15)));
        App_Const::profilesTable[sender()->property("slot").toInt()].profName = newLabel.left(15);
    }

    DiffUpdate();
}


void guiWindow::colorBoxes_clicked()
{
    QColor colorPick = QColorDialog::getColor(App_Const::profilesTable[sender()->property("slot").toInt()].color);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        App_Const::profilesTable[sender()->property("slot").toInt()].color = packedColor;
        color[sender()->property("slot").toInt()]->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));
        DiffUpdate();
    }
}


void guiWindow::on_customPinsEnabled_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::customPins] = arg1;
    BoxesUpdate();

    if(arg1)
        ui->customLayoutToolBtn->setEnabled(true);
    else ui->customLayoutToolBtn->setEnabled(false);

    if(App_Const::inputsMap.value(OF_Const::solenoidPin) > OF_Const::btnUnmapped) {
        ui->solenoidToggle->setEnabled(true);
    } else {
        ui->solenoidToggle->setEnabled(false);
        ui->solenoidToggle->setChecked(false);
    }

    if(App_Const::inputsMap.value(OF_Const::rumblePin) > OF_Const::btnUnmapped) {
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
            pinBoxes.at(i)->setCurrentIndex(OF_Const::btnUnmapped+1);

        // set pinboxes to alt preset values (and let the index changed signal handle the rest)
        QList<OF_Const::boardAltPresetsMap_t> altPresets = OF_Const::boardsAltPresets.values(App_Const::board.boardType.toStdString());
        for(int i = 0; i < PINS_COUNT; i++)
            pinBoxes.at(i)->setCurrentIndex(altPresets.at(index).pin[i]+1);

        DiffUpdate();
    }
}


void guiWindow::on_rumbleToggle_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::rumble] = arg1;
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
    if(!(arg1 && App_Const::boolSettings[OF_Const::rumbleFF]) && !App_Const::boolSettings[OF_Const::solenoid]) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_solenoidToggle_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::solenoid] = arg1;
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
    if(!arg1 && !(App_Const::boolSettings[OF_Const::rumble] && App_Const::boolSettings[OF_Const::rumbleFF])) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_autofireToggle_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::autofire] = arg1;
    if(arg1) { ui->autofireWaitFactorBox->setEnabled(true); } else { ui->autofireWaitFactorBox->setEnabled(false); }
    DiffUpdate();
}


void guiWindow::on_simplePauseToggle_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::simplePause] = arg1;
    DiffUpdate();
}


void guiWindow::on_holdToPauseToggle_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::holdToPause] = arg1;
    if(arg1) { ui->holdToPauseLengthBox->setEnabled(true); }
    else { ui->holdToPauseLengthBox->setEnabled(false); }
    DiffUpdate();
}


void guiWindow::on_commonAnodeToggle_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::commonAnode] = arg1;
    DiffUpdate();
}


void guiWindow::on_lowButtonsToggle_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::lowButtonsMode] = arg1;
    DiffUpdate();
}


void guiWindow::on_rumbleFFToggle_stateChanged(int arg1)
{
    App_Const::boolSettings[OF_Const::rumbleFF] = arg1;
    if(arg1) { ui->solenoidToggle->setChecked(false); }
    if(!(arg1 && App_Const::boolSettings[OF_Const::rumble]) && !App_Const::boolSettings[OF_Const::solenoid]) {
        ui->autofireToggle->setChecked(false);
        ui->autofireToggle->setEnabled(false);
    } else {
        ui->autofireToggle->setEnabled(true);
    }
    DiffUpdate();
}


void guiWindow::on_rumbleIntensityBox_valueChanged(int arg1)
{
    App_Const::settingsTable[OF_Const::rumbleStrength] = arg1;
    DiffUpdate();
}


void guiWindow::on_rumbleLengthBox_valueChanged(int arg1)
{
    App_Const::settingsTable[OF_Const::rumbleInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_holdToPauseLengthBox_valueChanged(int arg1)
{
    App_Const::settingsTable[OF_Const::holdToPauseLength] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidNormalIntervalBox_valueChanged(int arg1)
{
    App_Const::settingsTable[OF_Const::solenoidNormalInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidFastIntervalBox_valueChanged(int arg1)
{
    App_Const::settingsTable[OF_Const::solenoidFastInterval] = arg1;
    DiffUpdate();
}


void guiWindow::on_solenoidHoldLengthBox_valueChanged(int arg1)
{
    App_Const::settingsTable[OF_Const::solenoidHoldLength] = arg1;
    DiffUpdate();
}


void guiWindow::on_autofireWaitFactorBox_valueChanged(int arg1)
{
    App_Const::settingsTable[OF_Const::autofireWaitFactor] = arg1;
    DiffUpdate();
}


void guiWindow::on_tUSB_p1_toggled(bool checked)
{
    if(checked) {
        App_Const::tinyUSBtable.tinyUSBid = "1";
        App_Const::tinyUSBtable.tinyUSBname = "FIRECon P1";
        ui->productIdInput->setValue(App_Const::tinyUSBtable.tinyUSBid.toInt());
        ui->productNameInput->setText(App_Const::tinyUSBtable.tinyUSBname);

        DiffUpdate();
    }
}


void guiWindow::on_tUSB_p2_toggled(bool checked)
{
    if(checked) {
        App_Const::tinyUSBtable.tinyUSBid = "2";
        App_Const::tinyUSBtable.tinyUSBname = "FIRECon P2";
        ui->productIdInput->setValue(App_Const::tinyUSBtable.tinyUSBid.toInt());
        ui->productNameInput->setText(App_Const::tinyUSBtable.tinyUSBname);

        DiffUpdate();
    }
}


void guiWindow::on_tUSB_p3_toggled(bool checked)
{
    if(checked) {
        App_Const::tinyUSBtable.tinyUSBid = "3";
        App_Const::tinyUSBtable.tinyUSBname = "FIRECon P3";
        ui->productIdInput->setValue(App_Const::tinyUSBtable.tinyUSBid.toInt());
        ui->productNameInput->setText(App_Const::tinyUSBtable.tinyUSBname);

        DiffUpdate();
    }
}


void guiWindow::on_tUSB_p4_toggled(bool checked)
{
    if(checked) {
        App_Const::tinyUSBtable.tinyUSBid = "4";
        App_Const::tinyUSBtable.tinyUSBname = "FIRECon P4";
        ui->productIdInput->setValue(App_Const::tinyUSBtable.tinyUSBid.toInt());
        ui->productNameInput->setText(App_Const::tinyUSBtable.tinyUSBname);

        DiffUpdate();
    }
}


void guiWindow::on_productIdInput_valueChanged(int arg1)
{
    App_Const::tinyUSBtable.tinyUSBid = QString::number(arg1);
    if(ui->productNameInput->text().isEmpty()) {
        switch(arg1) {
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
    bool badInput = false;
    // Very unga-bunga way of doing this.
    // if someone is aware of a validator for this, feel free to replace this.
    for(int i = 0; i < arg1.length(); i++) {
        if(arg1.at(i).unicode() > 255) {
            badInput = true;
            break;
        }
    }

    if(badInput) {
        ui->productNameInput->setText(App_Const::tinyUSBtable.tinyUSBname);
        ui->productNameInput->setStyleSheet("color: red");
    } else {
        if(!ui->productNameInput->styleSheet().isEmpty())
            ui->productNameInput->setStyleSheet("");
        App_Const::tinyUSBtable.tinyUSBname = arg1;
        DiffUpdate();
    }
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
        if(sender()->property("slot").toInt() != App_Const::board.selectedProfile) {
            emit serial.OneShotSend("XC" + QString::number(sender()->property("slot").toInt()+1));
            App_Const::board.selectedProfile = sender()->property("slot").toInt();
            DiffUpdate();
        }
    }
}


void guiWindow::on_neopixelStrandLengthBox_valueChanged(int arg1)
{
    App_Const::settingsTable[OF_Const::customLEDcount] = arg1;
    if(arg1 < App_Const::settingsTable[OF_Const::customLEDstatic]) {
        ui->customLEDstaticSpinbox->setValue(arg1);
    }

    // show NeoPixel notice if values are updated
    PixelsDiff();

    DiffUpdate();
}


void guiWindow::on_customLEDstaticSpinbox_valueChanged(int arg1)
{
    if(arg1 > App_Const::settingsTable[OF_Const::customLEDcount]) { ui->customLEDstaticSpinbox->setValue(App_Const::settingsTable[OF_Const::customLEDcount]); }
    else { App_Const::settingsTable[OF_Const::customLEDstatic] = arg1; }
    if(OF_Const::customLEDstatic) {
        switch(App_Const::settingsTable[OF_Const::customLEDstatic]) {
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
    QColor colorPick = QColorDialog::getColor(App_Const::settingsTable[OF_Const::customLEDcolor1]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        App_Const::settingsTable[OF_Const::customLEDcolor1] = packedColor;
        ui->customLEDstaticBtn1->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}


void guiWindow::on_customLEDstaticBtn2_clicked()
{
    QColor colorPick = QColorDialog::getColor(App_Const::settingsTable[OF_Const::customLEDcolor2]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        App_Const::settingsTable[OF_Const::customLEDcolor2] = packedColor;
        ui->customLEDstaticBtn2->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}


void guiWindow::on_customLEDstaticBtn3_clicked()
{
    QColor colorPick = QColorDialog::getColor(App_Const::settingsTable[OF_Const::customLEDcolor3]);
    if(colorPick.isValid()) {
        int *red = new int;
        int *green = new int;
        int *blue = new int;
        colorPick.getRgb(red, green, blue);
        uint32_t packedColor = 0;
        packedColor |= *red << 16;
        packedColor |= *green << 8;
        packedColor |= *blue;
        App_Const::settingsTable[OF_Const::customLEDcolor3] = packedColor;
        ui->customLEDstaticBtn3->setStyleSheet(QString("background-color: #%1").arg(packedColor, 6, 16, QLatin1Char('0')));

        // show NeoPixel notice if values are updated
        PixelsDiff();

        DiffUpdate();
    }
}

void guiWindow::caliBtns_clicked()
{
    caliWindow = new AppCaliWindow(nullptr, AppCaliWindow::modeCalibrate);
    caliWindow->setProperty("profile", sender()->property("slot").toInt());
    connect(caliWindow, &AppCaliWindow::WindowExiting,      this, &guiWindow::CaliWindowExiting);
    connect(caliWindow, &AppCaliWindow::CaliRequestToExit,  this, &guiWindow::CaliWindowRequestedExit);

    caliWindow->showFullScreen();

    emit serial.OneShotSend(QString("XC%1CI%2L%3").arg(sender()->property("slot").toInt()+1)
                                                  .arg(App_Const::profilesTable.at(sender()->property("slot").toInt()).irSensitivity)
                                                  .arg(App_Const::profilesTable.at(sender()->property("slot").toInt()).layoutType)
                                                  .toLocal8Bit());
}


// WARNING: make sure "serialActive" is set ON for important operations, or this will eat the fucker
// TODO: move to appserial
void guiWindow::serialPort_readyRead()
{
    debugWindow.AppendText(serial.serialWorker.port->peek(serial.serialWorker.port->bytesAvailable()));

    if(!serialActive) {
        while(!serial.serialWorker.port->atEnd()) {
            QString idleBuffer = serial.serialWorker.port->readLine();

            if(idleBuffer.contains("Pressed:")) {
                int btn = idleBuffer.mid(idleBuffer.indexOf(' ')).trimmed().toInt();
                if(btn < 16)
                    testLabel[btn]->setStyleSheet("background-color: #FF0000; font: bold");
            }

            else if(idleBuffer.contains("Released:")) {
                int btn = idleBuffer.mid(idleBuffer.indexOf(' ')).trimmed().toInt();
                if(btn < 16)
                    testLabel[btn]->setStyleSheet("");
            }

            else if(idleBuffer.contains("Temperature:")) {
                unsigned int temp = idleBuffer.mid(idleBuffer.indexOf(' ')).trimmed().toInt();

                testLabel[14]->setText(QString("Temp: %1°C").arg(temp));

                if(temp > tempShutoff) {        testLabel[14]->setStyleSheet("color: white;      background-color: #FF0000; font: bold"); }
                else if(temp > tempWarning) {   testLabel[14]->setStyleSheet("color: light-gray; background-color: #EABD2B; font: bold"); }
                else {                          testLabel[14]->setStyleSheet("color: black;      background-color: #11D00A; font: bold"); }
            }

            else if(idleBuffer.contains("Analog:")) {
                // TODO: perhaps we should be using a small box area with a glyph depicting the aStick's coords instead of only showing cardinal directionality?
                uint8_t analogDir = idleBuffer.mid(idleBuffer.indexOf(' ')).trimmed().toInt();

                // analog stick moved
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
                // no analog direction
                } else {
                    testLabel[15]->setText("Analog");
                    testLabel[15]->setStyleSheet("");
                }
            }

            else if(idleBuffer.contains("Profile: ")) {
                uint8_t selection = idleBuffer.mid(idleBuffer.indexOf(' ')).trimmed().toInt();

                if(selection != App_Const::board.selectedProfile) {
                    App_Const::board.selectedProfile = selection;
                    selectedProfile[selection]->setChecked(true);
                }

                DiffUpdate();
            }

            else if(idleBuffer.contains("CalStage:")) {
                if(caliWindow != nullptr)
                    if(caliWindow->GetWindowMode() == AppCaliWindow::modeCalibrate)
                        caliWindow->CaliModeSet(idleBuffer.mid(idleBuffer.indexOf(' ')).trimmed().toInt());
            }

            else if(idleBuffer.contains("CalUpd:")) {
                if(caliWindow != nullptr)
                    if(caliWindow->GetWindowMode() == AppCaliWindow::modeCalibrate)
                        caliWindow->CaliModeTextUpdate(idleBuffer.mid(idleBuffer.indexOf(' ')).trimmed());
            }

            else if(idleBuffer.contains("Entering Test Mode...")) {
                if(caliWindow != nullptr)
                    caliWindow->Shutdown();

                caliWindow = new AppCaliWindow(nullptr, AppCaliWindow::modeIRTest);
                connect(caliWindow, &AppCaliWindow::WindowExiting, this, &guiWindow::CaliWindowExiting);

                caliWindow->showFullScreen();

                testMode = true;

                ui->buttonsTestArea->setEnabled(false);
                ui->confirmButton->setEnabled(false);
                ui->confirmButton->setText("[Disabled while in Test Mode]");
                ui->pinsTab->setEnabled(false);
                ui->settingsTab->setEnabled(false);
                ui->profilesTab->setEnabled(false);
                ui->feedbackTestsBox->setEnabled(false);
                ui->dangerZoneBox->setEnabled(false);
            }

            else if(idleBuffer.contains("Cleared! Please reset the board.")) {
                ui->comPortSelector->setCurrentIndex(0);
                QMessageBox::information(this, "Successfully reset board settings",
                                         "Please unplug the board and reinsert it into the PC.");
            }
        }

    } else if(testMode) {
        QString testBuffer = serial.serialWorker.port->readLine();

        if(testBuffer.contains(','))
            if(caliWindow != nullptr)
                if(caliWindow->GetWindowMode() == AppCaliWindow::modeIRTest)
                    caliWindow->TestModeDraw(testBuffer.remove("\r\n").split(',', Qt::SkipEmptyParts));
    }
}


void guiWindow::serialPort_handleResult(const int &type, const bool &success)
{
    switch(type) {
    case SerialThreadWorker::Serial_SearchPorts:
        if(success) {
            // ports have changed, update COM ports list
            // if comPort only has "Nothing", safe to add items
            if(ui->comPortSelector->count() == 0) {
                if(serial.serialWorker.currentPorts.count()) {
                    ui->comPortSelector->addItem("[Select a device]");
                    ui->comPortSelector->setCurrentIndex(0);
                    for(const auto port : serial.serialWorker.currentPorts)
                        ui->comPortSelector->addItem(port.portName());
                }
            // if comPort is filled
            // TODO: find some way to add new items without removing
            } else {
                if(serial.serialWorker.currentPorts.count()) {
                    // if no active comPort
                    if(ui->comPortSelector->currentIndex() <= 0) {
                        while(ui->comPortSelector->count() > 1)
                            ui->comPortSelector->removeItem(1);

                        for(const auto port : serial.serialWorker.currentPorts)
                            ui->comPortSelector->addItem(port.portName());
                    // if comPort is active
                    } else {
                        // remove all other comPorts
                        int i = 1;
                        while(ui->comPortSelector->count() > 2) {
                            if(i == ui->comPortSelector->currentIndex())
                                i++;
                            else ui->comPortSelector->removeItem(i);
                        }

                        // check if current comPort is still in devices list
                        // TODO: probably a better way of doing this, meh
                        bool inList = false;
                        for(const auto port : serial.serialWorker.currentPorts)
                            if(ui->comPortSelector->currentText() == port.portName())
                                inList = true;
                        if(!inList) {
                            statusBar()->showMessage("Current board has been disconnected.");
                            ui->comPortSelector->removeItem(1);
                        }

                        // append new items to list
                        for(const auto port : serial.serialWorker.currentPorts)
                            ui->comPortSelector->addItem(port.portName());
                    }
                // if ports list is cleared, assume no board can be connected.
                } else {
                    if(ui->comPortSelector->currentIndex() > 0)
                        statusBar()->showMessage("Current board has been disconnected.");
                    ui->comPortSelector->clear();
                }
            }
        }
        break;
    case SerialThreadWorker::Serial_GetSettings:
        if(success) {
            for(int i = 0; i < topOffset.count(); i++) {
                delete topOffset.at(i);
                delete bottomOffset.at(i);
                delete leftOffset.at(i);
                delete rightOffset.at(i);
                delete TLled.at(i);
                delete TRled.at(i);
                delete renameBtn.at(i);
                delete selectedProfile.at(i);
                delete irSens.at(i);
                delete runMode.at(i);
                delete layoutMode.at(i);
                delete color.at(i);
                delete caliBtn.at(i);
            }

            topOffset.clear();
            bottomOffset.clear();
            leftOffset.clear();
            rightOffset.clear();
            TLled.clear();
            TRled.clear();
            renameBtn.clear();
            selectedProfile.clear();
            irSens.clear();
            runMode.clear();
            layoutMode.clear();
            color.clear();
            caliBtn.clear();

            int caliBtnRow;
            for(uint8_t i = 0; i < App_Const::profilesTable.size(); i++) {
                caliBtnRow = i/4;

                // create new assets for this profile
                renameBtn << new QPushButton();
                renameBtn.at(i)->setFlat(true);
                renameBtn.at(i)->setFixedWidth(20);
                renameBtn.at(i)->setIcon(QIcon(":/icon/edit.png"));
                renameBtn.at(i)->installEventFilter(this);
                renameBtn.at(i)->setProperty("slot", i);
                renameBtn.at(i)->setProperty("trackable", App_Const::trackProfileItem);
                renameBtn.at(i)->setAccessibleName(QString("Rename Profile %1").arg(i+1));
                renameBtn.at(i)->setWhatsThis("<p>Click to rename this Calibration Profile.</p>"
                                              "<p>Aside from differentiating between different profiles for different displays, "
                                              "Cali Profile names are displayed in Pause Mode when using a compatible <i>I2C Display.</i></p>");
                connect(renameBtn.at(i), &QPushButton::clicked, this, &guiWindow::renameBoxes_clicked);

                selectedProfile << new QRadioButton(QString("%1. %2").arg(i+1).arg(App_Const::profilesTable.at(i).profName));
                if(i == App_Const::board.selectedProfile)
                    selectedProfile.at(i)->setChecked(true);
                selectedProfile.at(i)->setFont(QFont("Monospace"));
                selectedProfile.at(i)->setProperty("slot", i);
                connect(selectedProfile.at(i), &QRadioButton::toggled, this, &guiWindow::selectedProfile_isChecked);

                topOffset       << new QLabel(QString("%1").arg(App_Const::profilesTable.at(i).topOffset      ));
                bottomOffset    << new QLabel(QString("%1").arg(App_Const::profilesTable.at(i).bottomOffset   ));
                leftOffset      << new QLabel(QString("%1").arg(App_Const::profilesTable.at(i).leftOffset     ));
                rightOffset     << new QLabel(QString("%1").arg(App_Const::profilesTable.at(i).rightOffset    ));
                TLled           << new QLabel(QString("%1").arg(App_Const::profilesTable.at(i).TLled          ));
                TRled           << new QLabel(QString("%1").arg(App_Const::profilesTable.at(i).TRled          ));

                irSens << new QComboBox();
                irSens.at(i)->addItems({"Default", "Higher", "Highest"});
                irSens.at(i)->setCurrentIndex(App_Const::profilesTable.at(i).irSensitivity);
                irSens.at(i)->installEventFilter(this);
                irSens.at(i)->setProperty("slot", i);
                irSens.at(i)->setProperty("type", App_Const::pBoxIRsens);
                irSens.at(i)->setProperty("trackable", App_Const::trackProfileItem);
                irSens.at(i)->setAccessibleName(QString("Camera Sensitivity for Profile %1").arg(i+1));
                irSens.at(i)->setWhatsThis("<p>This setting determines the sensitivity of the IR Camera for this Calibration Profile.</p>"
                                           "<p>If the camera seems to have trouble picking up IR emitters (and is causing coarse cursor movement), "
                                           "adjusting this setting higher might fix issues with tracking.<br>"
                                           "Conversely, setting sensitivity too high may cause indirect IR sources "
                                           "(such as sunlight or IR bouncing off of reflective surfaces) "
                                           "to be picked up instead, causing the cursor to jitter or erratically jump across the screen.</p>");
                connect(irSens.at(i), SIGNAL(activated(int)), this, SLOT(profileBoxes_activated(int)));

                runMode << new QComboBox();
                runMode.at(i)->addItems({"Normal", "1-Frame Avg", "2-Frame Avg"});
                runMode.at(i)->setCurrentIndex(App_Const::profilesTable.at(i).runMode);
                runMode.at(i)->installEventFilter(this);
                runMode.at(i)->setProperty("slot", i);
                runMode.at(i)->setProperty("type", App_Const::pBoxRunMode);
                runMode.at(i)->setProperty("trackable", App_Const::trackProfileItem);
                runMode.at(i)->setAccessibleName(QString("Camera Position Averaging Mode for Profile %1").arg(i+1));
                runMode.at(i)->setWhatsThis("<p>This setting determines the cursor Averaging Mode for this Calibration Profile.</p>"
                                            "<p>The movement of the aiming cursor can be smoothed out by averaging a select number of frames, "
                                            "at the cost of a small increase in latency; conversely, disabling this position averaging can "
                                            "reduce latency, at the cost of some added jitter in mouse movement.</p>"
                                            "<p>The default is <b>1-Frame Avg</b>, which should be the preferred balance for most people.</p>");
                connect(runMode.at(i), SIGNAL(activated(int)), this, SLOT(profileBoxes_activated(int)));

                layoutMode << new QComboBox();
                layoutMode.at(i)->addItems({"Square", "Diamond"});
                layoutMode.at(i)->setCurrentIndex(App_Const::profilesTable.at(i).layoutType);
                layoutMode.at(i)->installEventFilter(this);
                layoutMode.at(i)->setProperty("slot", i);
                layoutMode.at(i)->setProperty("type", App_Const::pBoxLayout);
                layoutMode.at(i)->setProperty("trackable", App_Const::trackProfileItem);
                layoutMode.at(i)->setAccessibleName(QString("IR Emitter Layout for Profile %1").arg(i+1));
                layoutMode.at(i)->setWhatsThis("<p>This setting determines the IR Layout to be used with this Calibration Profile.</p>"
                                               "<p>Each Cali Profile can be set to use either the <i>Square Layout,</i> "
                                               "which uses two pairs of emitters on the top and bottom, and <i>Diamond Layout,</i> "
                                               "which uses one emitter at the center of each side of the display.</p>"
                                               "<p><i>Square Layout</i> generally has much higher accuracy at any angle and allows for "
                                               "playing closer to the screen or using external Fish Eye lenses without viewport distortion, "
                                               "while <i>Diamond Layout</i> is for screen compatibility with certain legacy lightgun systems "
                                               "(allowing OpenFIRE guns to play with such other lightgun systems on the same display).</p>"
                                               "<p>If unsure, use <b>Square Layout</b> "
                                               "(unless you also use a different brand of lightgun that needs a diamond IR layout to function).</p>");
                connect(layoutMode.at(i), SIGNAL(activated(int)), this, SLOT(profileBoxes_activated(int)));

                color << new QPushButton();
                color.at(i)->setFixedWidth(32);
                color.at(i)->setStyleSheet(QString("background-color: #%1").arg(App_Const::profilesTable.at(i).color, 6, 16, QLatin1Char('0')));
                color.at(i)->installEventFilter(this);
                color.at(i)->setProperty("slot", i);
                color.at(i)->setProperty("trackable", App_Const::trackProfileItem);
                color.at(i)->setAccessibleName(QString("Profile Menu Color for Cali Profile %1").arg(i+1));
                color.at(i)->setWhatsThis("<p>Open a window to select the color used to represent this profile in <i>Pause Mode.</i></p>"
                                          "<p>Each profile can be assigned a color used to identify them when switching profiles on the lightgun itself, "
                                          "which is emitted by a 4-pin RGB LED and/or an active NeoPixel strand.</p>");
                connect(color.at(i), &QPushButton::clicked, this, &guiWindow::colorBoxes_clicked);

                caliBtn << new QPushButton(QString("Calibrate Profile %1").arg(i+1));
                caliBtn.at(i)->installEventFilter(this);
                caliBtn.at(i)->setProperty("slot", i);
                caliBtn.at(i)->setProperty("trackable", App_Const::trackProfileItem);
                caliBtn.at(i)->setAccessibleName(QString("Open Calibration Window for Cali Profile %1").arg(i+1));
                caliBtn.at(i)->setWhatsThis("Click to start the calibration process for this profile.");
                connect(caliBtn.at(i), &QPushButton::clicked, this, &guiWindow::caliBtns_clicked);

                topOffset.at(i)     ->setAlignment(Qt::AlignCenter);
                bottomOffset.at(i)  ->setAlignment(Qt::AlignCenter);
                leftOffset.at(i)    ->setAlignment(Qt::AlignCenter);
                rightOffset.at(i)   ->setAlignment(Qt::AlignCenter);
                TLled.at(i)         ->setAlignment(Qt::AlignCenter);
                TRled.at(i)         ->setAlignment(Qt::AlignCenter);

                ui->profilesArea->addWidget(renameBtn.at(i),       i+1, 0);
                ui->profilesArea->addWidget(selectedProfile.at(i), i+1, 1);
                ui->profilesArea->addWidget(topOffset.at(i),       i+1, 3);
                ui->profilesArea->addWidget(bottomOffset.at(i),    i+1, 5);
                ui->profilesArea->addWidget(leftOffset.at(i),      i+1, 7);
                ui->profilesArea->addWidget(rightOffset.at(i),     i+1, 9);
                ui->profilesArea->addWidget(TLled.at(i),           i+1, 11);
                ui->profilesArea->addWidget(TRled.at(i),           i+1, 13);
                ui->profilesArea->addWidget(irSens.at(i),          i+1, 15);
                ui->profilesArea->addWidget(runMode.at(i),         i+1, 17);
                ui->profilesArea->addWidget(layoutMode.at(i),      i+1, 19);
                ui->profilesArea->addWidget(color.at(i),           i+1, 21);

                ui->caliBtnsLayout->addWidget(caliBtn.at(i), caliBtnRow, i);
            }

            // Clears old board layout items
            if(pinBoxes.count()) {
                for(uint8_t i = 0; i < pinBoxes.count(); i++)
                    delete pinBoxes.at(i);
                for(uint8_t i = 0; i < padding.count(); i++)
                    delete padding.at(i);
                for(uint8_t i = 0; i < pinLabel.count(); i++)
                    delete pinLabel.at(i);

                pinBoxes.clear();
                padding.clear();
                pinLabel.clear();
            }

            for(uint8_t i = 0; i < PINS_COUNT; i++) {
                pinBoxes << new QComboBox();
                pinBoxes.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                pinBoxes.at(i)->setProperty("slot", i);
                pinBoxes.at(i)->setProperty("prevMapping", OF_Const::btnUnmapped+1);
                pinBoxes.at(i)->setProperty("trackable", App_Const::trackPinbox);
                pinBoxes.at(i)->installEventFilter(this);
                // install items
                pinBoxes.at(i)->addItems(OF_Const::valuesNameList);
                // clear out analog options for digital pins (< GPIO26)
                // (entrylist is offset by one, as "Unmapped" == -1 in our enum)
                if(i < 26) {
                    SetComboBoxItemEnabled(pinBoxes.at(i), OF_Const::analogX+1, false);
                    SetComboBoxItemEnabled(pinBoxes.at(i), OF_Const::analogY+1, false);
                    SetComboBoxItemEnabled(pinBoxes.at(i), OF_Const::tempPin+1, false);
                }
                // filter out SCL/SDA if possible.
                if(i & 1) {
                    SetComboBoxItemEnabled(pinBoxes.at(i), OF_Const::camSDA+1,     false);
                    SetComboBoxItemEnabled(pinBoxes.at(i), OF_Const::periphSDA+1,  false);
                } else {
                    SetComboBoxItemEnabled(pinBoxes.at(i), OF_Const::camSCL+1,     false);
                    SetComboBoxItemEnabled(pinBoxes.at(i), OF_Const::periphSCL+1,  false);
                }
                connect(pinBoxes.at(i), SIGNAL(currentIndexChanged(int)), this, SLOT(pinBoxes_currentIndexChanged(int)));

                padding << new QWidget();
                padding.at(i)->setMinimumHeight(25);

                // I2C channel coloring
                if(i & 0b0000010)
                    pinLabel  << new QLabel(QString("<font color=#FF8800>«GPIO%1»</font>").arg(i));
                else pinLabel << new QLabel(QString("<font color=#0099FF>«GPIO%1»</font>").arg(i));

                pinLabel.at(i)->setEnabled(false);
                pinLabel.at(i)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                pinLabel.at(i)->setToolTip(QString("GPIO Pin number %1\n\nBlue pin numbers are members of I2C0\nOrange are members of I2C1").arg(i));
            }

            ui->versionLabel->setText(QString("v%1 - \"%2\"").arg(App_Const::board.versionNumber, App_Const::board.versionCodename));

            // update presets box if this board has any
            ui->presetsBox->clear();

            if(OF_Const::boardsAltPresets.count(App_Const::board.boardType.toStdString())) {
                ui->presetsBox->setHidden(false);
                ui->presetsBox->setEnabled(true);

                QList<OF_Const::boardAltPresetsMap_t> altPresets = OF_Const::boardsAltPresets.values(App_Const::board.boardType.toStdString());
                for(auto &entry : altPresets)
                    ui->presetsBox->addItem(entry.name);
            } else {
                ui->presetsBox->setEnabled(false);
                ui->presetsBox->setHidden(true);
            }

            // set boxes to reflect indexes of inputsMap
            BoxesUpdate();

            LabelsUpdate();

            // Drawing the actual board view page by referencing the board maps data from OpenFIREshared.h
            if(OF_Const::boardsBoxPositions.contains(App_Const::board.boardType.toStdString())) {
                QFile resource(":/boardPics/" + App_Const::board.boardType);
                resource.open(QIODevice::ReadOnly);
                origBoardPicFile = resource.readAll();

                for(int i = 0; i < PINS_COUNT; i++) {
                    if(OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] & OF_Const::posLeft) {
                        ui->PinsLeft->addWidget(pinBoxes.at(i),
                                                OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] ^ OF_Const::posLeft,
                                                0);
                        ui->PinsLeft->addWidget(pinLabel.at(i),
                                                OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] ^ OF_Const::posLeft,
                                                1);
                    } else if(OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] & OF_Const::posRight) {
                        ui->PinsRight->addWidget(pinBoxes.at(i),
                                                 OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] ^ OF_Const::posRight,
                                                 1);
                        ui->PinsRight->addWidget(pinLabel.at(i),
                                                 OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] ^ OF_Const::posRight,
                                                 0);
                    } else if(OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] & OF_Const::posMiddle) {
                        ui->PinsCenterSub->addWidget(pinBoxes.at(i),
                                                     1,
                                                     OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] ^ OF_Const::posMiddle);
                        ui->PinsCenterSub->addWidget(pinLabel.at(i),
                                                     0,
                                                     OF_Const::boardsBoxPositions.value(App_Const::board.boardType.toStdString()).pin[i] ^ OF_Const::posMiddle);
                    }
                }
            } else {
                QFile resource(":/boardPics/generic");
                resource.open(QIODevice::ReadOnly);
                origBoardPicFile = resource.readAll();

                for(int i = 0; i < PINS_COUNT; i++) {
                    if(OF_Const::boardsBoxPositions.value("generic").pin[i] & OF_Const::posLeft) {
                        ui->PinsLeft->addWidget(pinBoxes.at(i),
                                                OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posLeft,
                                                0);
                        ui->PinsLeft->addWidget(pinLabel.at(i),
                                                OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posLeft,
                                                1);
                    } else if(OF_Const::boardsBoxPositions.value("generic").pin[i] & OF_Const::posRight) {
                        ui->PinsRight->addWidget(pinBoxes.at(i),
                                                 OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posRight,
                                                 1);
                        ui->PinsRight->addWidget(pinLabel.at(i),
                                                 OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posRight,
                                                 0);
                    } else if(OF_Const::boardsBoxPositions.value("generic").pin[i] & OF_Const::posMiddle) {
                        ui->PinsCenterSub->addWidget(pinBoxes.at(i),
                                                     1,
                                                     OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posMiddle);
                        ui->PinsCenterSub->addWidget(pinLabel.at(i),
                                                     0,
                                                     OF_Const::boardsBoxPositions.value("generic").pin[i] ^ OF_Const::posMiddle);
                    }
                }
            }

            // aspect ratio hint needs to be set every time a new asset is loaded
            boardPic.load(origBoardPicFile);
            boardPic.renderer()->setAspectRatioMode(Qt::KeepAspectRatio);

            int prevPadCount;
            for(int i = 1, padCount = 0; i < ui->PinsLeft->rowCount(); i++) {
                if(ui->PinsLeft->itemAtPosition(i, 0) == nullptr) {
                    ui->PinsLeft->addWidget(padding.at(padCount), i, 0);
                    padCount++;
                    prevPadCount = padCount;
                }
            }
            for(int i = 1, padCount = prevPadCount; i < ui->PinsRight->rowCount(); i++) {
                if(ui->PinsRight->itemAtPosition(i, 0) == nullptr) {
                    ui->PinsRight->addWidget(padding.at(padCount), i, 0);
                    padCount++;
                }
            }

            ui->tabWidget->setEnabled(true);
            ui->customPinsEnabled->setChecked(App_Const::boolSettings[OF_Const::customPins]);

            if(App_Const::inputsMap.value(OF_Const::rumblePin) >= 0)
                ui->rumbleToggle->setEnabled(true),  ui->rumbleFFToggle->setEnabled(true);
            else ui->rumbleToggle->setEnabled(false), ui->rumbleFFToggle->setEnabled(false);
            ui->rumbleToggle->setChecked(App_Const::boolSettings[OF_Const::rumble]);

            if(App_Const::inputsMap.value(OF_Const::solenoidPin) >= 0)
                ui->solenoidToggle->setEnabled(true);
            else ui->solenoidToggle->setEnabled(false);
            ui->solenoidToggle->setChecked(App_Const::boolSettings[OF_Const::solenoid]);

            if((App_Const::boolSettings[OF_Const::rumble] && App_Const::boolSettings[OF_Const::rumbleFF]) || App_Const::boolSettings[OF_Const::solenoid])
                ui->autofireToggle->setEnabled(true);
            else ui->autofireToggle->setEnabled(false);
            ui->autofireToggle->setChecked(App_Const::boolSettings[OF_Const::autofire]);

            ui->simplePauseToggle->setChecked(App_Const::boolSettings[OF_Const::simplePause]);
            ui->holdToPauseToggle->setChecked(App_Const::boolSettings[OF_Const::holdToPause]);

            if(App_Const::inputsMap.value(OF_Const::ledR) >= 0 && App_Const::inputsMap.value(OF_Const::ledG) >= 0 && App_Const::inputsMap.value(OF_Const::ledB) >= 0)
                ui->commonAnodeToggle->setEnabled(true);
            else ui->commonAnodeToggle->setEnabled(false);
            ui->commonAnodeToggle->setChecked(App_Const::boolSettings[OF_Const::commonAnode]);

            ui->lowButtonsToggle->setChecked(App_Const::boolSettings[OF_Const::lowButtonsMode]);
            ui->rumbleFFToggle->setChecked(App_Const::boolSettings[OF_Const::rumbleFF]);
            ui->rumbleIntensityBox->setEnabled(App_Const::boolSettings[OF_Const::rumble]),          ui->rumbleIntensityBox->setValue(App_Const::settingsTable[OF_Const::rumbleStrength]);
            ui->rumbleLengthBox->setEnabled(App_Const::boolSettings[OF_Const::rumble]),             ui->rumbleLengthBox->setValue(App_Const::settingsTable[OF_Const::rumbleInterval]);
            ui->holdToPauseLengthBox->setEnabled(App_Const::boolSettings[OF_Const::holdToPause]),   ui->holdToPauseLengthBox->setValue(App_Const::settingsTable[OF_Const::holdToPauseLength]);
            ui->solenoidNormalIntervalBox->setEnabled(App_Const::boolSettings[OF_Const::solenoid]), ui->solenoidNormalIntervalBox->setValue(App_Const::settingsTable[OF_Const::solenoidNormalInterval]);
            ui->solenoidFastIntervalBox->setEnabled(App_Const::boolSettings[OF_Const::solenoid]),   ui->solenoidFastIntervalBox->setValue(App_Const::settingsTable[OF_Const::solenoidFastInterval]);
            ui->solenoidHoldLengthBox->setEnabled(App_Const::boolSettings[OF_Const::solenoid]),     ui->solenoidHoldLengthBox->setValue(App_Const::settingsTable[OF_Const::solenoidHoldLength]);
            ui->autofireWaitFactorBox->setEnabled(App_Const::boolSettings[OF_Const::autofire]),     ui->autofireWaitFactorBox->setValue(App_Const::settingsTable[OF_Const::autofireWaitFactor]);

            ui->productIdInput->setValue(App_Const::tinyUSBtable.tinyUSBid.toInt());
            ui->productNameInput->setText(App_Const::tinyUSBtable.tinyUSBname);

            if(App_Const::inputsMap.value(OF_Const::neoPixel) >= 0)
                ui->neopixelGroupBox->setEnabled(true);
            else ui->neopixelGroupBox->setEnabled(false);
            ui->neopixelStrandLengthBox->setValue(App_Const::settingsTable[OF_Const::customLEDcount]);
            ui->customLEDstaticSpinbox->setValue(App_Const::settingsTable[OF_Const::customLEDstatic]);
            ui->customLEDstaticBtn1->setStyleSheet(QString("background-color: #%1").arg(App_Const::settingsTable[OF_Const::customLEDcolor1], 6, 16, QLatin1Char('0')));
            ui->customLEDstaticBtn2->setStyleSheet(QString("background-color: #%1").arg(App_Const::settingsTable[OF_Const::customLEDcolor2], 6, 16, QLatin1Char('0')));
            ui->customLEDstaticBtn3->setStyleSheet(QString("background-color: #%1").arg(App_Const::settingsTable[OF_Const::customLEDcolor3], 6, 16, QLatin1Char('0')));

            switch(App_Const::tinyUSBtable.tinyUSBid.toInt()) {
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
        } else ui->comPortSelector->setCurrentIndex(0);
        serialPort_progressSet(0);
        break;
    case SerialThreadWorker::Serial_OneShot:
        break;
    case SerialThreadWorker::Serial_Sync:
        if(success) {
            statusBar()->showMessage("Sent settings successfully!", 5000);

            // sync settings
            for(int i = 0; i < OF_Const::boolTypesCount; i++)
                App_Const::boolSettings_orig[i] = App_Const::boolSettings[i];

            if(App_Const::boolSettings_orig[OF_Const::customPins])
                App_Const::inputsMap_orig = App_Const::inputsMap;
            else for(int i = 0; i < App_Const::inputsMap.size(); i++)
                    App_Const::inputsMap_orig[i] = -1;

            for(int i = 0; i < OF_Const::settingsTypesCount; i++)
                App_Const::settingsTable_orig[i] = App_Const::settingsTable[i];

            App_Const::tinyUSBtable_orig.tinyUSBid = App_Const::tinyUSBtable.tinyUSBid;
            App_Const::tinyUSBtable_orig.tinyUSBname = App_Const::tinyUSBtable.tinyUSBname;
            App_Const::board.previousProfile = App_Const::board.selectedProfile;

            for(uint8_t i = 0; i < PROFILES_COUNT; i++) {
                App_Const::profilesTable_orig[i].irSensitivity = App_Const::profilesTable[i].irSensitivity;
                App_Const::profilesTable_orig[i].runMode = App_Const::profilesTable[i].runMode;
                App_Const::profilesTable_orig[i].layoutType = App_Const::profilesTable[i].layoutType;
                App_Const::profilesTable_orig[i].color = App_Const::profilesTable[i].color;
                App_Const::profilesTable_orig[i].profName = App_Const::profilesTable[i].profName;
            }

            // Reflect new names in UI
            LabelsUpdate();

            // update (clear) diffs
            PixelsDiff();
            DiffUpdate();
        } else printf("Settings syncing failed!?\n");
        serialPort_progressSet(0);
        ui->tabWidget->setEnabled(true);
        ui->comPortSelector->setEnabled(true);
        serialActive = false;
        break;
    case SerialThreadWorker::Serial_Disconnect:
        // reset stuff
        testLabel[14]->setStyleSheet("");
        testLabel[15]->setStyleSheet("");
        // force disable test mode if it was set
        if(testMode) {
            testMode = false;
            ui->buttonsTestArea->setEnabled(true);
            ui->pinsTab->setEnabled(true);
            ui->settingsTab->setEnabled(true);
            ui->profilesTab->setEnabled(true);
            ui->feedbackTestsBox->setEnabled(true);
            ui->dangerZoneBox->setEnabled(true);
            serialActive = false;
        }
        serialActive = false;
        ui->tabWidget->setEnabled(false);
        break;
    }
}


void guiWindow::serialPort_progressSet(const int &range)
{
    if(range) {
        if(!statusProgressBar->isVisible())
            statusProgressBar->setVisible(true);
        statusProgressBar->setRange(0, range);
    } else {
        statusProgressBar->setVisible(false);
        statusProgressBar->setValue(0);
    }
}


void guiWindow::serialPort_progressUpdate(const int &pos)
{
    if(statusProgressBar->isVisible())
        statusProgressBar->setValue(pos);
}


void guiWindow::on_rumbleTestBtn_clicked()
{
    emit serial.OneShotSend("Xtr");
    ui->statusBar->showMessage("Sent a rumble test pulse.", 2500);
}


void guiWindow::on_solenoidTestBtn_clicked()
{
    emit serial.OneShotSend("Xts");
    ui->statusBar->showMessage("Sent a solenoid test pulse.", 2500);
}


void guiWindow::on_redLedTestBtn_clicked()
{
    emit serial.OneShotSend("XtR");
    ui->statusBar->showMessage("Set LED to Red.", 2500);
}


void guiWindow::on_greenLedTestBtn_clicked()
{
    emit serial.OneShotSend("XtG");
    ui->statusBar->showMessage("Set LED to Green.", 2500);
}


void guiWindow::on_blueLedTestBtn_clicked()
{
    emit serial.OneShotSend("XtB");
    ui->statusBar->showMessage("Set LED to Blue.", 2500);
}


void guiWindow::on_testBtn_clicked()
{
    // Pre-emptively put a sock in the readyRead signal
    serialActive = true;

    emit serial.OneShotSend("XT");
}


void guiWindow::CaliWindowExiting(const int &mode,
                                  const int &topOffsetNew,
                                  const int &bottomOffsetNew,
                                  const int &leftOffsetNew,
                                  const int &rightOffsetNew,
                                  const float &topLeftLedNew,
                                  const float &topRightLedNew)
{
    switch(mode) {
    case AppCaliWindow::modeCalibrate:
    {
        if(topOffsetNew >= 0 &&
            bottomOffsetNew >= 0 &&
            leftOffsetNew >= 0 &&
            rightOffsetNew >= 0 &&
            topLeftLedNew >= 0 && topLeftLedNew < 32768 &&
            topRightLedNew >= 0 && topRightLedNew < 32768) {
            uint8_t selection = caliWindow->property("profile").toInt();

            App_Const::profilesTable[selection].topOffset = topOffsetNew;
            topOffset[selection]->setText(QString::number(topOffsetNew));

            App_Const::profilesTable[selection].bottomOffset = bottomOffsetNew;
            bottomOffset[selection]->setText(QString::number(bottomOffsetNew));

            App_Const::profilesTable[selection].leftOffset = leftOffsetNew;
            leftOffset[selection]->setText(QString::number(leftOffsetNew));

            App_Const::profilesTable[selection].rightOffset = rightOffsetNew;
            rightOffset[selection]->setText(QString::number(rightOffsetNew));

            App_Const::profilesTable[selection].TLled = topLeftLedNew;
            TLled[selection]->setText(QString::number(topLeftLedNew));

            App_Const::profilesTable[selection].TRled = topRightLedNew;
            TRled[selection]->setText(QString::number(topRightLedNew));

            DiffUpdate();
            ui->statusBar->showMessage("Calibration for Profile " + QString::number(selection) + " successful", 5000);
        } else {
            ui->statusBar->showMessage("Calibration failed: invalid results, reverting to original values.", 10000);
        }
        break;
    }
    case AppCaliWindow::modeIRTest:
        emit serial.OneShotSend("XT");

        testMode = false;

        ui->buttonsTestArea->setEnabled(true);
        ui->pinsTab->setEnabled(true);
        ui->settingsTab->setEnabled(true);
        ui->profilesTab->setEnabled(true);
        ui->feedbackTestsBox->setEnabled(true);
        ui->dangerZoneBox->setEnabled(true);

        serialActive = false;
        break;
    case AppCaliWindow::modeAlignment:
    default:
        break;
    }

    // for some reason, caliWindows can leave a lingering pointer???
    // so make sure it's deleted.
    caliWindow->close();
    if(caliWindow != nullptr)
        caliWindow = nullptr;
}


void guiWindow::CaliWindowRequestedExit()
{
    emit serial.OneShotSend("X");
}


void guiWindow::on_clearEepromBtn_clicked()
{
    // Do we really need all this msgbox setup?
    QMessageBox messageBox;
    messageBox.setText("Really delete saved data?");
    messageBox.setInformativeText("This operation will delete all saved data, including:\n\n"
                                  " - Calibration Profiles\n"
                                  " - Toggles\n - Settings\n"
                                  " - Custom Identifiers\n\n"
                                  "Are you sure about this?");
    messageBox.setWindowTitle("Delete Confirmation");
    messageBox.setIcon(QMessageBox::Warning);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::Yes);

    if(messageBox.exec() == QMessageBox::Yes)
        emit serial.OneShotSend("Xc");
    else ui->statusBar->showMessage("Clear operation canceled.", 3000);
}


void guiWindow::on_baudResetBtn_clicked()
{
    // No need for workarounds, bootloader reset is in the firmware now.
    emit serial.OneShotSend("Xxx");

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
}


void guiWindow::on_tabWidget_currentChanged(int index)
{
    switch(index) {
    // settings tab
    case 1:
        ui->settingsDescBox->setTitle("");
        ui->settingsDescText->setText(ui->settingsDescText->whatsThis());
        break;
    // profiles tab
    case 2:
        ui->profilesDescBox->setTitle("");
        ui->profilesDescText->setText(ui->profilesDescText->whatsThis());
        break;
    // test tab (no use yet)
    case 3:
        break;
    // pins tab (doesn't have any)
    case 0:
    default:
        break;
    }
}


void guiWindow::on_actionAbout_UI_triggered()
{
    AppAbout *about = new AppAbout();
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


void guiWindow::on_actionOpen_IR_Emitter_Alignment_Assistant_triggered()
{
    if(caliWindow == nullptr) {
        caliWindow = new AppCaliWindow(nullptr, AppCaliWindow::modeAlignment);
        connect(caliWindow, &AppCaliWindow::WindowExiting, this, &guiWindow::CaliWindowExiting);
        caliWindow->setAttribute(Qt::WA_DeleteOnClose);

        caliWindow->showFullScreen();
    }
}


void guiWindow::on_actionImport_Custom_Layout_triggered()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Save New Layout",
                                                QDir::homePath(),
                                                "OpenFIRE Layout Files (*.ofl)");

    if(!path.isEmpty()) {
        QFile fileIn(path);
        if(fileIn.open(QFile::ReadOnly)) {
            if(fileIn.readLine().trimmed() == App_Const::board.boardType) {
                ui->customPinsEnabled->setChecked(true);
                // clear current mapping
                for(int i = 0; i < PINS_COUNT; i++)
                    pinBoxes.at(i)->setCurrentIndex(OF_Const::btnUnmapped+1);

                // import new maps
                for(int i = 0; i < PINS_COUNT; i++)
                    if(!fileIn.atEnd())
                        pinBoxes.at(i)->setCurrentIndex(fileIn.read(1).toHex().toInt(nullptr, 16));
                    else break;

                fileIn.close();
                ui->statusBar->showMessage("Successfully imported custom layout!", 5000);
            } else QMessageBox::warning(this, "Board Doesn't Match",
                                              "Custom layout file is not compatible with this board.");
        } else QMessageBox::warning(this, "File Read Error",
                                          "Custom layout file could not be read.");
    } else ui->statusBar->showMessage("Canceled custom layout load operation.", 5000);
}


void guiWindow::on_actionExport_Custom_Layout_triggered()
{
    QString path = QFileDialog::getSaveFileName(this,
                                                "Save New Layout",
                                                QDir::homePath(),
                                                "OpenFIRE Layout Files (*.ofl)");

    if(!path.isEmpty()) {
        QFile fileOut(path);
        if(fileOut.open(QFile::WriteOnly)) {
            fileOut.write(QString("%1\n").arg(App_Const::board.boardType).toLocal8Bit());

            for(int i = 0; i < PINS_COUNT; i++)
                fileOut.putChar(pinBoxes.at(i)->currentIndex());

            fileOut.close();
            ui->statusBar->showMessage("Custom layout export successful!", 5000);
        } else QMessageBox::warning(this, "File Write Error",
                                          "Custom layout file could not be written.");
    } else ui->statusBar->showMessage("Canceled custom layout save operation.", 5000);
}

void guiWindow::on_actionDebug_Window_triggered()
{
    debugWindow.show();
}

