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

// Maximum amount of GPIO that the RP2040 microcontroller has available
#define PINS_COUNT 30

// Default maximum amount of profiles to read in (TODO: could just be made a flexible number)
#define PROFILES_COUNT 4

// Interval of the aliveTimer object that probes the board to ensure it's connected
#define ALIVE_TIMER 5000

#include "constants.h"
#include "appcali.h"
#include "../boards/OpenFIREshared.h"
#include <QMainWindow>
#include <QSerialPort>
#include <QGraphicsItem>
#include <QPen>
#include <QTimer>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStandardItemModel>
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

private slots:
    void aliveTimer_timeout();

    void on_comPortSelector_currentIndexChanged(int index);

    void on_confirmButton_clicked();

    void pinBoxes_currentIndexChanged(int index);

    void serialPort_readyRead();

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

    void on_actionOpen_IR_Emitter_Alignment_Assistant_triggered();

    void CaliWindowExiting(const int &);

    void on_actionImport_Custom_Layout_triggered();

    void on_actionExport_Custom_Layout_triggered();

private:
    Ui::guiWindow *ui;

    /// @brief      Calibration window pointer
    /// @details    Only one of these should be up at a time
    AppCaliWindow *caliWindow = nullptr;

    /// @brief      Submethod that fills contents of boxes with OF_Const::valuesNamesList
    /// @details
    void BoxesFill();

    /// @brief      Mass update all pinboxes with certain sets of values
    /// @details    Used when toggling custom pins, initial load, and setting presets
    void BoxesUpdate();

    /// @brief      Updates boards layout header
    /// @details    Also invokes name prettification
    void LabelsUpdate();

    /// @brief      Converts system name to display name, provided in OF_Const::boardNames
    QString PrettifyName();

    /// @brief      Checks for differences in current staging settings
    /// @details    Controls enablement of "send to board" button
    void DiffUpdate();

    /// @brief      Checks for differences in NeoPixels settings
    /// @details    Controls enablement of certain settings in the NeoPixels section of settings
    void PixelsDiff();

    /// @brief      Search for available serial port devices
    /// @details    Filters for OpenFIRE devices specifically
    // (TODO: move to appserial)
    void PortsSearch();

    /// @brief      Pair serial device to portNum device, and start grabbing its info
    /// @returns    True if device could be initiated, false if syncing failed
    bool SerialInit(int portNum);

    /// @brief      Grab firmware settings from serial device
    /// @details    Currently only called by the success route of SerialInit
    void SerialLoad();

    /// @brief      Sync current settings from app to board
    /// @details    If successful, current settings get copied to "orig" settings tables
    void SyncSettings();

    /// @brief      Disables given setting of a combobox
    /// @arg        Combobox item, index number to toggle, enable state to set to
    void SetComboBoxItemEnabled(QComboBox * comboBox, const int index, const bool enabled) {
        auto * model = qobject_cast<QStandardItemModel*>(comboBox->model());
        auto * item = model->item(index);
        item->setEnabled(enabled);
    }

    // ^^^---Methods---^^^
    //
    // vvv---Internal Values---vvv

    QSerialPort serialPort;

    bool serialActive = false;

    /// @brief      List of serial port objects that were found in PortsSearch()
    QList<QSerialPortInfo> serialFoundList;

    /// @brief      Extracted COM paths, as provided from serialFoundList
    QStringList usbName;

    /// @brief      Current array of booleans
    /// @details    Meant for toggle/on-off type settings specifically
    bool boolSettings[OF_Const::boolTypesCount];

    /// @brief      Array of booleans last synced from the microcontroller
    /// @details    This is only updated on saving and loading settings successfully
    bool boolSettings_orig[OF_Const::boolTypesCount];

    /// @brief      Current array of tunable settings
    uint32_t settingsTable[OF_Const::settingsTypesCount];

    /// @brief      Array of tunables last synced from the microcontroller
    /// @details    This is only updated on saving and loading settings successfully
    uint32_t settingsTable_orig[OF_Const::settingsTypesCount];

    /// @brief      Temperature thresholds (which should be a customizable setting in the settingsTable)
    // TODO: add this to settingsTable (5.1?)
    uint8_t tempWarning = 35;
    uint8_t tempShutoff = 42;

    /// @brief      Indicator if the test window is activated (to block potentially sending noise)
    bool testMode = false;

    /// @brief      Timer that probes the board if it's still plugged in
    /// @details    Timer interval is provided in ms by ALIVE_TIMER
    QTimer *aliveTimer;

    // ^^^---Internal Values---^^^
    //
    // vvv---UI Objects down here:---vvv

    // Always remember to nullptr your fresh pointers, kids!
    // or else release mode undefined behavior will bite your ass :)

    /// @brief      Layouts that makes up the board view tab
    /// @details    Gets deleted whenever the board view is updated (i.e. board changes)
    QVBoxLayout *PinsCenter = nullptr;
    QGridLayout *PinsCenterSub = nullptr;
    QGridLayout *PinsLeft = nullptr;
    QGridLayout *PinsRight = nullptr;
    QSvgWidget *centerPic = nullptr;

    /// @brief      Objects that makes up the elements of the board view tab
    /// @details    Pinboxes stores the state of each pin to one function
    QComboBox *pinBoxes[30] = {nullptr};
    QLabel *pinLabel[30] = {nullptr};
    QWidget *padding[30] = {nullptr};

    /// @brief      Test "Buttons" in the test screen representing each button
    QLabel *testLabel[16];

    /// @brief      Objects that makes up the elements of the profiles tab
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
};
#endif // GUIWINDOW_H
