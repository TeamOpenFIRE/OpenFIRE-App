/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Main interface.

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

#ifndef APPMAINWINDOW_H
#define APPMAINWINDOW_H

// Interval of the aliveTimer object that probes the board to ensure it's connected
#define ALIVE_TIMER 5000

#include "appcommon.h"
#include "appcali.h"
#ifdef OFAPP_DEBUG
#include "appdebug.h"
#endif
#include "appserial.h"
#include "apppreviewer.h"

#include <QMainWindow>
#include <QSerialPort>
#include <QFuture>
#include <QFutureWatcher>
#include <QGraphicsItem>
#include <QPen>
#include <QTimer>
#include <QLayout>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSvgWidget>
#include <QProgressBar>

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

    /// global
    void on_comPortSelector_currentTextChanged(const QString &);

    void on_confirmButton_clicked();

    /// pin layouts
    void pinBoxes_currentIndexChanged(int index);

    void on_customPinsEnabled_stateChanged(int arg1);

    void on_presetsBox_currentIndexChanged(int index);

    /// button mapping
    void btnFuncTypeBox_currentIndexChanged(int index);

    void btnFuncBox_currentTextChanged(const QString &);

    void on_aStickModeBox_currentIndexChanged(int index);

    /// gun settings
    void on_rumbleToggle_stateChanged(int arg1);

    void on_solenoidToggle_stateChanged(int arg1);

    void on_autofireToggle_stateChanged(int arg1);

    void on_simplePauseToggle_stateChanged(int arg1);

    void on_holdToPauseToggle_stateChanged(int arg1);

    void on_commonAnodeToggle_stateChanged(int arg1);

    void on_lowButtonsToggle_stateChanged(int arg1);

    void on_rumbleFFToggle_stateChanged(int arg1);

    void on_tempWarningBox_valueChanged(int arg1);

    void on_tempShutoffBox_valueChanged(int arg1);

    void on_rumbleIntensityBox_valueChanged(int arg1);

    void on_rumbleLengthBox_valueChanged(int arg1);

    void on_holdToPauseLengthBox_valueChanged(int arg1);

    void on_solenoidOnLengthBox_valueChanged(int arg1);

    void on_solenoidOffLengthBox_valueChanged(int arg1);

    void on_solenoidHoldLengthBox_valueChanged(int arg1);

    void on_neopixelStrandLengthBox_valueChanged(int arg1);

    void on_customLEDstaticSpinbox_valueChanged(int arg1);

    void on_customLEDstaticBtn1_clicked();

    void on_customLEDstaticBtn2_clicked();

    void on_customLEDstaticBtn3_clicked();

    void on_invertStaticPixelsBox_stateChanged(int arg1);

    void on_i2cOLEDtoggle_stateChanged(int arg1);

    void on_oledAltAddrsToggle_stateChanged(int arg1);

    void on_tinyUSBLayoutToggle_stateChanged(int arg1);

    void on_tUSB_p1_toggled(bool checked);

    void on_tUSB_p2_toggled(bool checked);

    void on_tUSB_p3_toggled(bool checked);

    void on_tUSB_p4_toggled(bool checked);

    void on_productIdInput_valueChanged(int arg1);

    void on_productNameInput_textEdited(const QString &arg1);

    /// cali profiles
    void renameBoxes_clicked();

    void colorBoxes_clicked();

    void profileBoxes_activated(int arg1);

    void selectedProfile_isChecked(bool isChecked);

    void caliBtns_clicked();

    /// gun tests
    void on_rumbleTestBtn_clicked();

    void on_solenoidTestBtn_clicked();

    void on_redLedTestBtn_clicked();

    void on_greenLedTestBtn_clicked();

    void on_blueLedTestBtn_clicked();

    void on_baudResetBtn_clicked();

    void on_clearEepromBtn_clicked();

    void on_testBtn_clicked();

    /// system/background
    void on_tabWidget_currentChanged(int index);

    void serialPort_readyRead();

    void serialPort_SearchFinished();

    void serialPort_progressSet(const int &);

    void serialPort_progressUpdate(const int &, const QString& = nullptr);

    void on_actionShow_Unsafe_Settings_toggled(bool arg1);

    void on_actionCompatible_Boards_triggered();

    void on_actionOpenFIRE_Documentation_triggered();

    void on_actionOpenFIRE_Serial_Usage_triggered();

    void on_actionImport_Custom_Layout_triggered();

    void on_actionExport_Custom_Layout_triggered();

    void on_actionOpen_IR_Emitter_Alignment_Assistant_triggered();

    void CaliWindowExiting(const int &mode, const int & = -1, const int & = -1, const int & = -1, const int & = -1, const float & = -1, const float & = -1);

    void CaliWindowRequestedExit();

    void on_actionDebug_Window_triggered();

    void on_actionAbout_UI_triggered();

private:
    Ui::guiWindow *ui;

    bool eventFilter(QObject* object, QEvent* event) override;

    /// @brief      Calibration window pointer
    /// @details    Only one of these should be up at a time
    AppCaliWindow *caliWindow = nullptr;

    /// @brief      Boards previewer window
    AppBoardsPreviewer boardsWindow;

#ifdef OFAPP_DEBUG
    /// @brief      Serial debug window
    AppDebugWindow debugWindow;
#endif

    /// @brief      Macro for making new CaliWindows
    /// @param      int
    ///             CaliWindow type (should be one of AppCaliWindow::AppCaliStates_e
    void NewCaliWindow(const int &);

    /// @brief      Mass update all pinboxes with certain sets of values
    /// @details    Used when toggling custom pins, initial load, and setting presets
    void BoxesUpdate();

    /// @brief      Updates boards layout header
    /// @details    Also invokes name prettification
    void LabelsUpdate();

    /// @brief      Converts system name to display name, provided in OF_Const::boardNames
    QString PrettifyName(QString);

    /// @brief      Checks for differences in current staging settings
    /// @details    Controls enablement of "send to board" button
    void DiffUpdate();

    /// @brief      Checks for differences in NeoPixels settings
    /// @details    Controls enablement of certain settings in the NeoPixels section of settings
    void PixelsDiff();

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

    // Instance of App's Serial operators
    AppSerial serial;

    /// @brief      Current pointers to device/layout maps for synced device
    std::unordered_map<std::string_view, std::vector<int>>::const_iterator presetMap;
    std::unordered_map<std::string_view, std::vector<unsigned int>>::const_iterator layoutMap;
    std::unordered_map<std::string_view, std::vector<int>>::const_iterator pinCapabilityMap;

    // result of async serial operations
    QFuture<bool> serialSearchFuture;
    QFutureWatcher<bool> serialSearchWatcher;

    // Flag that's set during I/O operations so that the readyRead signal doesn't interfere and absorb RX buffer mid-method.
    bool serialActive = false;

    /// @brief      Timer that probes the board if it's still plugged in
    /// @details    Timer interval is provided in ms by ALIVE_TIMER
    QTimer aliveTimer;

    // ^^^---Internal Values---^^^
    //
    // vvv---UI Objects down here:---vvv

    // Always remember to nullptr your fresh pointers, kids!
    // or else release mode undefined behavior will bite your ass :)

    /// @brief      Renderer that makes up the centerpiece of the board view tab
    QSvgWidget boardPic;

    /// @brief      Current board picture's byte array representation
    /// @details    Used to quickly copy/modify for board view highlights
    QByteArray origBoardPicFile;
    QString highlightBoardPic;

    /// @brief      Objects that makes up the elements of the board view tab
    /// @details    Pinboxes stores the state of each pin to one function
    QVector<QComboBox*> pinBoxes;
    QVector<QLabel*> pinLabel;

    /// @brief      Test "Buttons" in the test screen representing each button
    QVector<QLabel*> testLabel;

    /// @brief      Analog stick graphic view
    QGraphicsScene analogGfxScene;
    QGraphicsEllipseItem* analogPos;

    /// @brief      Button Mapping elements
    /// @details    Array 1 = onscreen input / offscreen input / gamepad mode input
    ///             Array 2: 0 = input type, 1 = input data
    ///             Array 3 = button
    QComboBox btnFuncBox[App_Common::inputTypes][App_Common::inputFuncTypes][BUTTON_COUNT-1];
    QVector<QGroupBox*> btnFuncGBoxes;
    QVector<QHBoxLayout*> btnFuncLayout;

    /// @brief      Objects that makes up the elements of the profiles tab
    QVector<QRadioButton*> selectedProfile;
    QVector<QLabel*> topOffset;
    QVector<QLabel*> bottomOffset;
    QVector<QLabel*> leftOffset;
    QVector<QLabel*> rightOffset;
    QVector<QLabel*> TLled;
    QVector<QLabel*> TRled;
    QVector<QComboBox*> irSens;
    QVector<QComboBox*> runMode;
    QVector<QComboBox*> layoutMode;
    QVector<QComboBox*> aspectRatio;
    QVector<QPushButton*> color;
    QVector<QPushButton*> renameBtn;
    QVector<QPushButton*> caliBtn;

    QProgressBar *statusProgressBar = nullptr;
};
#endif // GUIWINDOW_H
