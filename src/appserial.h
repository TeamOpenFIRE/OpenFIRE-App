#ifndef APPSERIAL_H
#define APPSERIAL_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

class AppSerial : public QObject
{
    Q_OBJECT

public:
    enum {
        Serial_SearchPorts = 0,
        Serial_GetSettings,
        Serial_OneShot,
        Serial_Sync,
        Serial_Disconnect
    } serialReturnTypes_e;

    QSerialPort port;

    /// @brief      Primary list of found serial devices
    /// @note       Only to be updated when new found ports list is different from this one.
    QList<QSerialPortInfo> currentPorts;

    QStringList currentPortsNames;

    /// @brief      Searches for new serial devices
    /// @returns    Whether devices list has changed (true) or not (false)
    /// @note       If found devices are different, this list is merged into the new currentPorts.
    bool SearchPorts();

    /// @brief      Creates a string list of ports based on given list of serial devices
    /// @returns    New list of port names
    /// @param      QList
    ///             Serial ports list
    QStringList GeneratePortsList(const QList<QSerialPortInfo> &);

    /// @brief      Grabs settings from connected Serial device to App_Const
    /// @returns    Success (true) or failure (false)
    /// @param      Index of currentPorts list to pull from
    bool GetSettings(const QString &);

    /// @brief      Sends serial message to currently connected Serial device
    /// @returns    Success (true) or failure (false)
    /// @param      QString
    ///             String to send to device
    bool OneShotSend(const QByteArray &);

    /// @brief      Commits settings (App_Const) to currently connected Serial device
    /// @returns    Success (true) or failure (false)
    bool CommitSettings();

    /// @brief      Disconnects current serial device and clears port name
    void Disconnect();

signals:
    /// @brief
    void Serial_SetProgressRange(const int &);

    /// @brief
    void Serial_ProgressUpdate(const int &, const char* = nullptr);
};

#endif // APPSERIAL_H
