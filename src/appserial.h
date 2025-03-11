#ifndef APPSERIAL_H
#define APPSERIAL_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>

class SerialThreadWorker : public QObject
{
    Q_OBJECT

public:
    SerialThreadWorker() {};

    enum {
        Serial_SearchPorts = 0,
        Serial_GetSettings,
        Serial_OneShot,
        Serial_Sync,
        Serial_Disconnect
    } serialReturnTypes_e;

    QSerialPort *port;

    /// @brief      Primary list of found serial devices
    /// @note       Only to be updated when new found ports list is different from this one.
    QList<QSerialPortInfo> currentPorts;

    QStringList currentPortsNames;

    /// @brief      Searches for new serial devices
    /// @returns    Whether devices list has changed (true) or not (false)
    /// @note       If found devices are different, this list is merged into the new currentPorts.
    bool SearchPorts();

    QStringList GeneratePortsList(const QList<QSerialPortInfo> &);

    /// @brief      Grabs settings from connected Serial device to App_Const
    /// @returns    Success (true) or failure (false)
    /// @param      Index of currentPorts list to pull from
    bool GetSettings(const QString &);

    /// @brief      Sends serial message to currently connected Serial device
    /// @returns    Success (true) or failure (false)
    /// @param      QString
    ///             String to send to device
    bool OneShotSend(const QString &);

    /// @brief      Commits settings (App_Const) to currently connected Serial device
    /// @returns    Success (true) or failure (false)
    bool CommitSettings();

    /// @brief      Disconnects current serial device and clears port name
    /// @returns    Always true
    bool Disconnect();

signals:
    void Worker_StartupFinished();

    /// @brief      Signal emitted at the end of SearchPorts method.
    /// @param      int
    ///             Serial method indicator
    /// @param      bool
    ///             Result of SearchPorts method
    void SearchPorts_Result(const int &, const bool &);

    /// @brief      Signal emitted at the end of GetSettings method.
    /// @param      int
    ///             Serial method indicator
    /// @param      bool
    ///             Result of GetSettings method
    void GetSettings_Result(const int &, const bool &);

    /// @brief      Signal emitted at the end of OneShotSend method.
    /// @param      int
    ///             Serial method indicator
    /// @param      bool
    ///             Result of OneShotSend method
    void OneShot_Result(const int &, const bool &);

    /// @brief      Signal emitted at the end of CommitSettings method.
    /// @param      int
    ///             Serial method indicator
    /// @param      bool
    ///             Result of CommitSettings method
    void Commit_Result(const int &, const bool &);

    /// @brief      Signal emitted at the end of Disconnect method.
    /// @param      int
    ///             Serial method indicator
    void Disconnect_Result(const int &, const bool &);

    /// @brief
    void Serial_SetProgressRange(const int &);

    /// @brief
    void Serial_ProgressUpdate(const int &);

public slots:
    /// @brief      "Constructor" process for the thread + serialDevice setup, if needed
    void serialProcessStartup() {
        port = new QSerialPort();
        emit Worker_StartupFinished();
    }

    /// @brief      Runs SearchPorts method
    void Sig_SearchPorts() { emit SearchPorts_Result(Serial_SearchPorts, SearchPorts()); }

    /// @brief      Runs GetSettings method
    void Sig_GetSettings(const QString &portName) { emit GetSettings_Result(Serial_GetSettings, GetSettings(portName)); }

    /// @brief      Runs OneShotSend method
    void Sig_OneShotSend(const QString &buffer) { emit OneShot_Result(Serial_OneShot, OneShotSend(buffer)); }

    /// @brief      Runs CommitSettings method
    void Sig_CommitSettings() { emit Commit_Result(Serial_Sync, CommitSettings()); }

    void Sig_Disconnect() { emit Disconnect_Result(Serial_Disconnect, Disconnect()); }
};

class SerialThreadController : public QObject
{
    Q_OBJECT
    QThread serialThread;
public:
    SerialThreadController() {
        serialWorker.moveToThread(&serialThread);
        connect(this, &SerialThreadController::operate,        &serialWorker, &SerialThreadWorker::serialProcessStartup);
        connect(this, &SerialThreadController::SearchPorts,    &serialWorker, &SerialThreadWorker::Sig_SearchPorts);
        connect(this, &SerialThreadController::GetSettings,    &serialWorker, &SerialThreadWorker::Sig_GetSettings);
        connect(this, &SerialThreadController::OneShotSend,    &serialWorker, &SerialThreadWorker::Sig_OneShotSend);
        connect(this, &SerialThreadController::CommitSettings, &serialWorker, &SerialThreadWorker::Sig_CommitSettings);
        connect(this, &SerialThreadController::Disconnect,     &serialWorker, &SerialThreadWorker::Sig_Disconnect);
        serialThread.start();
    }

    void SerialEnd() {
        serialThread.quit();
        serialThread.wait();
    }
    SerialThreadWorker serialWorker;
signals:
    /// @brief      Starts the serialProcess "constructor" in the thread.
    void operate();

    ////// Senders

    /// @brief      Signal thread to run SearchPorts method.
    void SearchPorts();

    /// @brief      Signal thread to run GetSettings method.
    void GetSettings(const QString &);

    /// @brief      Signal thread to run OneShotSend method.
    /// @param      QString
    ///             String to send
    void OneShotSend(const QString &);

    /// @brief      Signal thread to run CommitSettings method.
    void CommitSettings();

    /// @brief      Signal thread to run Disconnect method.
    void Disconnect();

};

#endif // APPSERIAL_H
