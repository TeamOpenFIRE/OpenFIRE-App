/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Serial input/output routines.

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

#ifndef APPSERIAL_H
#define APPSERIAL_H

#include <QObject>
#include <QMessageBox>
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

    /// @brief      Macro for saving batches of settings being received over serial
    /// @returns    Success (true) or failure (false)
    /// @param      void*
    ///             Pointer to data array
    /// @param      unordered_map
    ///             Map from OF_Const::OFPresets to reference for string names and respective indices
    /// @param      size_t
    ///             Size of data blocks that void* points to, for correct seeks through array elements by address
    bool BatchStoreSettings(void*, const std::unordered_map<std::string_view, int>&, const size_t&);

    /// @brief      Macro for processing name of data type
    /// @returns    Name received from serial
    QByteArray RecvDataName();

    /// @brief      Sends serial message to currently connected Serial device
    /// @returns    Success (true) or failure (false)
    /// @param      QString
    ///             String to send to device
    bool OneShotSend(const char*, const unsigned int & = 0, const bool & = false);
    bool OneShotSend(const char &, const bool & = false);

    /// @brief      Commits settings (App_Const) to currently connected Serial device
    /// @returns    Success (true) or failure (false)
    bool CommitSettings();

    /// @brief      Macro for committing batches of data to board
    /// @returns    Success (true) or failure (false)
    /// @param      void*
    ///             Pointer to data array
    /// @param      unordered_map
    ///             Map from OF_Const::OFPresets to reference for string names and respective indices
    /// @param      size_t
    ///             Size of data blocks that void* points to, for correct seeks through array elements by address
    bool BatchSendSettings(void*, const std::unordered_map<std::string_view, int>&, const size_t&, const size_t& = 0);

    /// @brief      Disconnects current serial device and clears port name
    void Disconnect();

    /// @brief      Prompts user to reboot to bootloader
    void RequestToReboot();

    /// @brief      Sends magic baud 1200 signal to reset connected board to bootloader
    void RebootToBootldr();

    /// @brief      Show error message popup regarding serial
    void ShowError(const QString& titleText, const QString& text, const QMessageBox::Icon icon = QMessageBox::Warning) {
        if(!syncError.isVisible()) {
            syncError.setWindowTitle(titleText);
            syncError.setText(text);
            syncError.setIcon(icon);
            syncError.show();
        }
    }

private:
    QMessageBox syncError;

    char RXbuf[64];
    char TXbuf[64];

signals:
    /// @brief
    void Serial_SetProgressRange(const int &);

    /// @brief
    void Serial_ProgressUpdate(const int &, const QString& = nullptr);
};

#endif // APPSERIAL_H
