#ifndef GUIWINDOW_H
#define GUIWINDOW_H

#include <QMainWindow>

enum boardTypes_e {
    nothing = 0,
    rpipico,
    adafruitItsyRP2040,
    arduinoNanoRP2040,
    unknown = 255
};

enum boardInputs_e {
    btnReserved = -1,
    btnUnmapped = 0,
    btnTrigger,
    btnGunA,
    btnGunB,
    btnGunC,
    btnStart,
    btnSelect,
    btnGunUp,
    btnGunDown,
    btnGunLeft,
    btnGunRight,
    btnPedal,
    btnHome,
    btnPump,
    rumblePin,
    solenoidPin,
    tempPin,
    rumbleSwitch,
    solenoidSwitch,
    autofireSwitch,
    ledR,
    ledG,
    ledB,
    neoPixel,
    analogX,
    analogY
};

enum boolTypes_e {
    customPins = 0,
    rumble,
    solenoid,
    autofire,
    simplePause,
    holdToPause,
    commonAnode,
    lowButtonsMode
};

enum settingsTypes_e {
    rumbleStrength = 0,
    rumbleInterval,
    solenoidNormalInterval,
    solenoidFastInterval,
    solenoidHoldLength,
    customLEDcount,
    autofireWaitFactor,
    holdToPauseLength
};

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
    void on_comPortSelector_currentIndexChanged(int index);

    void on_confirmButton_clicked();

    void serialPort_readyRead();

    void pinBoxes_activated(int index);

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

    void on_actionAbout_IR_GUN4ALL_triggered();

private:
    Ui::guiWindow *ui;

    void BoxesUpdate();

    void DiffUpdate();

    void PopupWindow(QString errorTitle, QString errorMessage, QString windowTitle, int errorType);

    void PortsSearch();

    void SelectionUpdate(uint8_t newSelection);

    bool SerialInit(int portNum);

    void SerialLoad();
};
#endif // GUIWINDOW_H
