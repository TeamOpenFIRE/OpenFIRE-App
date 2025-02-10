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

#ifndef APPMAINWINDOW_H
#define APPMAINWINDOW_H

// Amount of profiles to read in (TODO: could just be made a flexible number)
#define PROFILES_COUNT 4

#include "constants.h"
#include "../boards/OpenFIREshared.h"
#include <QMainWindow>
#include <QSerialPort>
#include <QGraphicsItem>
#include <QPen>
#include <QTimer>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSvgWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class guiWindow;
}
QT_END_NAMESPACE

class guiWindow : public QMainWindow
{
    Q_OBJECT

public:
    guiWindow(QWidget *parent = nullptr);
    ~guiWindow();

    QSerialPort serialPort;

    bool serialActive = false;

private slots:
    void aliveTimer_timeout();

    void on_comPortSelector_currentIndexChanged(int index);

    void on_confirmButton_clicked();

    void serialPort_readyRead();

    void pinBoxes_currentIndexChanged(int index);

    void renameBoxes_clicked();

    void colorBoxes_clicked();

    void layoutBoxes_activated(int arg1);

    void irBoxes_activated(int index);

    void runModeBoxes_activated(int index);

    void on_customPinsEnabled_stateChanged(int arg1);

    void on_presetsBox_currentIndexChanged(int index);

    void on_rumbleTestBtn_clicked();

    void on_solenoidTestBtn_clicked();

    void on_baudResetBtn_clicked();

    void on_rumbleToggle_stateChanged(int arg1);

    void on_solenoidToggle_stateChanged(int arg1);

    void on_autofireToggle_stateChanged(int arg1);

    void on_simplePauseToggle_stateChanged(int arg1);

    void on_holdToPauseToggle_stateChanged(int arg1);

    void on_commonAnodeToggle_stateChanged(int arg1);

    void on_lowButtonsToggle_stateChanged(int arg1);

    void on_rumbleFFToggle_stateChanged(int arg1);

    void on_rumbleIntensityBox_valueChanged(int arg1);

    void on_rumbleLengthBox_valueChanged(int arg1);

    void on_holdToPauseLengthBox_valueChanged(int arg1);

    void on_solenoidNormalIntervalBox_valueChanged(int arg1);

    void on_solenoidFastIntervalBox_valueChanged(int arg1);

    void on_solenoidHoldLengthBox_valueChanged(int arg1);

    void on_autofireWaitFactorBox_valueChanged(int arg1);

    void on_productIdInput_textEdited(const QString &arg1);

    void on_productNameInput_textEdited(const QString &arg1);

    void on_neopixelStrandLengthBox_valueChanged(int arg1);

    void on_clearEepromBtn_clicked();

    void on_productIdInput_textChanged(const QString &arg1);

    void on_testBtn_clicked();

    void selectedProfile_isChecked(bool isChecked);

    void on_calib1Btn_clicked();

    void on_calib2Btn_clicked();

    void on_calib3Btn_clicked();

    void on_calib4Btn_clicked();

    void on_actionAbout_UI_triggered();

    void on_customLEDstaticSpinbox_valueChanged(int arg1);

    void on_customLEDstaticBtn1_clicked();

    void on_customLEDstaticBtn2_clicked();

    void on_customLEDstaticBtn3_clicked();

    void on_tinyUSBLayoutToggle_stateChanged(int arg1);

    void on_tUSB_p1_toggled(bool checked);

    void on_tUSB_p2_toggled(bool checked);

    void on_tUSB_p3_toggled(bool checked);

    void on_tUSB_p4_toggled(bool checked);

    void on_redLedTestBtn_clicked();

    void on_greenLedTestBtn_clicked();

    void on_blueLedTestBtn_clicked();

    void on_actionOpenFIRE_Documentation_triggered();

    void on_actionOpenFIRE_Serial_Usage_triggered();

private:
    Ui::guiWindow *ui;

    // Submethod that fills contents of boxes with OF_Const::valuesNamesList
    void BoxesFill();

    // what does this do again? lol
    void BoxesUpdate();

    // Updates board view labels/prettifies labels
    void LabelsUpdate();

    // Checks for differences in current staging settings, enables "send to board" button
    void DiffUpdate();
    // The same, but for NeoPixels specifically
    void PixelsDiff();

    // Search ports (TODO: move to appserial)
    void PortsSearch();

    void SelectionUpdate(uint8_t newSelection);

    // TODO: move to appserial
    bool SerialInit(int portNum);
    void SerialLoad();
    void SyncSettings();
    QString PrettifyName();

    // ^^^---Methods---^^^
    //
    // vvv---Internal Values---vvv

    // List of serial port objects that were found in PortsSearch()
    QList<QSerialPortInfo> serialFoundList;
    // Extracted COM paths, as provided from serialFoundList
    QStringList usbName;

    // Tracks the amount of differences between current config and loaded config.
    // Resets after every call to DiffUpdate()
    uint8_t settingsDiff;

    // Current array of booleans, meant to be used as a bitmask
    bool boolSettings[OF_Const::boolTypesCount];
    // Array of booleans, as loaded from the gun firmware
    bool boolSettings_orig[OF_Const::boolTypesCount];

    // Current table of tunable settings
    uint32_t settingsTable[OF_Const::settingsTypesCount];
    // Table of tunables, as loaded from gun firmware
    uint32_t settingsTable_orig[OF_Const::settingsTypesCount];

    // TODO: add this to settingsTable (5.1?)
    uint8_t tempWarning = 35;
    uint8_t tempShutoff = 42;

    // Indexed array map of the current physical layout of the board;
    // Also doubles as combobox sanity check.
    // Key = pin number, Value = pin function
    // Values: -2 = N/A, -1 = reserved, 0 = available, unused
    //QMap<uint8_t, int8_t> currentPins;

    // Indicator if the test window is activated (to block potentially sending noise)
    bool testMode = false;

    // Timer that probes the board if it's still plugged in
    QTimer *aliveTimer;
    // For AliveTimer that probes the board if it's still plugged in
    bool boardIsAlive = false;

    // ^^^---Internal Values---^^^
    //
    // vvv---GUI Objects---vvv

    // Currently loaded board object
    boardInfo_s board;

    // Currently loaded board's TinyUSB identifier info
    tinyUSBtable_s tinyUSBtable;
    // TinyUSB ident, as loaded from the board
    tinyUSBtable_s tinyUSBtable_orig;

    // Current calibration profiles
    QVector<profilesTable_s> profilesTable;
    // Calibration profiles, as loaded from the board
    QVector<profilesTable_s> profilesTable_orig;

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

    // Always remember to nullptr your fresh pointers, kids!
    // or else release mode undefined behavior will bite your ass :)
    QVBoxLayout *PinsCenter = nullptr;
    QGridLayout *PinsCenterSub = nullptr;
    QGridLayout *PinsLeft = nullptr;
    QGridLayout *PinsRight = nullptr;

    QComboBox *pinBoxes[30] = {nullptr};
    QLabel *pinLabel[30] = {nullptr};
    QWidget *padding[30] = {nullptr};

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

    QSvgWidget *centerPic = nullptr;
    QGraphicsScene *testScene = nullptr;
#define ALIVE_TIMER 5000

    // Test Mode screen points & colors
    QGraphicsEllipseItem testPointTL;
    QGraphicsEllipseItem testPointTR;
    QGraphicsEllipseItem testPointBL;
    QGraphicsEllipseItem testPointBR;
    QGraphicsEllipseItem testPointMed;
    QGraphicsEllipseItem testPointD;
    QGraphicsPolygonItem testBox;

    QPen testPointTLPen;
    QPen testPointTRPen;
    QPen testPointBLPen;
    QPen testPointBRPen;
    QPen testPointMedPen;
    QPen testPointDPen;
};
#endif // GUIWINDOW_H
