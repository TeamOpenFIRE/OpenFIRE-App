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

#ifndef GUIWINDOW_H
#define GUIWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QGraphicsItem>
#include <QPen>

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
    void on_comPortSelector_currentIndexChanged(int index);

    void on_confirmButton_clicked();

    void serialPort_readyRead();

    void pinBoxes_activated(int index);

    void renameBoxes_clicked();

    void colorBoxes_clicked();

    void layoutToggles_stateChanged(int arg1);

    void irBoxes_activated(int index);

    void runModeBoxes_activated(int index);

    void on_customPinsEnabled_stateChanged(int arg1);

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

    void on_neopixelStrandLengthBox_valueChanged(int arg1);

    void on_solenoidNormalIntervalBox_valueChanged(int arg1);

    void on_solenoidFastIntervalBox_valueChanged(int arg1);

    void on_solenoidHoldLengthBox_valueChanged(int arg1);

    void on_autofireWaitFactorBox_valueChanged(int arg1);

    void on_productIdInput_textEdited(const QString &arg1);

    void on_productNameInput_textEdited(const QString &arg1);

    void on_clearEepromBtn_clicked();

    void on_productIdInput_textChanged(const QString &arg1);

    void on_testBtn_clicked();

    void selectedProfile_isChecked(bool isChecked);

    void on_calib1Btn_clicked();

    void on_calib2Btn_clicked();

    void on_calib3Btn_clicked();

    void on_calib4Btn_clicked();

    void on_actionAbout_UI_triggered();

private:
    Ui::guiWindow *ui;

    // Used by pinBoxes, matching boardInputs_e
    QStringList valuesNameList = {
        "Unmapped",
        "Trigger",
        "Button A",
        "Button B",
        "Button C",
        "Start",
        "Select",
        "D-Pad Up",
        "D-Pad Down",
        "D-Pad Left",
        "D-Pad Right",
        "External Pedal 1",
        "External Pedal 2",
        "Home Button",
        "Pump Action",
        "Rumble Signal",
        "Solenoid Signal",
        "Rumble Switch",
        "Solenoid Switch",
        "Autofire Switch",
        "RGB LED Red",
        "RGB LED Green",
        "RGB LED Blue",
        "External NeoPixel",
        "Camera SDA",
        "Camera SCL",
        "Peripherals SDA",
        "Peripherals SCL",
        "Battery Sensor",
        "Analog Pin X",
        "Analog Pin Y",
        "Temp Sensor"
    };

    // List of serial port objects that were found in PortsSearch()
    QList<QSerialPortInfo> serialFoundList;
    // Extracted COM paths, as provided from serialFoundList
    QStringList usbName;

    // Tracks the amount of differences between current config and loaded config.
    // Resets after every call to DiffUpdate()
    uint8_t settingsDiff;

    // Current array of booleans, meant to be used as a bitmask
    bool boolSettings[9];
    // Array of booleans, as loaded from the gun firmware
    bool boolSettings_orig[9];

    // Current table of tunable settings
    uint16_t settingsTable[8];
    // Table of tunables, as loaded from gun firmware
    uint16_t settingsTable_orig[8];

    // because pinBoxes' "->currentIndex" gets updated AFTER calling its activation signal,
    // we need to save its last index to properly compare and prevent duplicate changes,
    // and then update it at the end of the activate signal.
    int pinBoxesOldIndex[30];

    // Uses the same logic as pinBoxesOldIndex, since the irSensor and runMode comboboxes
    // are hooked to a single signal.
    uint8_t irSensOldIndex[4];
    uint8_t runModeOldIndex[4];

    bool testMode = false;

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

    // ^^^---Values---^^^
    //
    // vvv---Methods---vvv

    void BoxesFill();

    void BoxesUpdate();

    void DiffUpdate();

    void PopupWindow(QString errorTitle, QString errorMessage, QString windowTitle, int errorType);

    void PortsSearch();

    void SelectionUpdate(uint8_t newSelection);

    bool SerialInit(int portNum);

    void SerialLoad();

    void SyncSettings();
};
#endif // GUIWINDOW_H
