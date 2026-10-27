/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Firmware download and UF2 flashing for RP2040/RP2350 boards.

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

#ifndef APPFLASH_H
#define APPFLASH_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QList>
#include <QTimer>
#include <QFile>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QFutureWatcher>
#include <memory>

class QNetworkReply;
class QTemporaryDir;

/// @brief  Downloads a firmware .uf2 and copies it to an RP2040/RP2350 board in BOOTSEL mode.
/// @details    The whole job (download, checks, waiting for the board, copy) is a small state machine
///             that reports back through signals; it never blocks the UI thread. The only things
///             ever written are the temporary download and a single .uf2 file on the one drive that
///             was recognized as an RP2040/RP2350 bootloader and confirmed by the user.
class AppFlash : public QObject
{
    Q_OBJECT

public:
    enum Chip {
        chipUnknown,
        chipRP2040,
        chipRP2350
    };

    enum State {
        stateIdle,
        statePreparing,         ///< reading a file from the computer
        stateDownloading,
        stateWaitingBootloader,
        stateConfirming,
        stateCopying,
        stateWaitingReboot,
        stateDone,
        stateFailed
    };

    /// @brief      What a confirmation requested through ConfirmRequested() is about
    enum Request {
        requestWarnings,
        requestFlash
    };

    /// @brief      Result of checking a UF2 file
    struct Uf2Info {
        bool        valid = false;
        QString     error;          ///< why it's not valid
        QStringList warnings;       ///< valid, but the user should know before flashing
        quint32     blocks = 0;
        Chip        chip = chipUnknown;     ///< as far as the family IDs in the file tell
    };

    /// @brief      A mounted drive that looks like an RP2040/RP2350 UF2 bootloader
    struct Volume {
        QString rootPath;
        QString name;           ///< volume label
        QString infoText;       ///< contents of INFO_UF2.TXT
        QString model;          ///< "Model:" line, if there is one
        QString boardId;        ///< "Board-ID:" line, if there is one
        Chip    chip = chipUnknown;     ///< as far as INFO_UF2.TXT tells
    };

    struct ScanResult {
        QList<Volume> bootloaders;
        QStringList   otherUf2;     ///< UF2 drives that are not an RP2040/RP2350 (left alone)
    };

    enum VolumeClass {
        volumeNone,
        volumeOtherUf2,
        volumeRP
    };

    static constexpr quint32 familyRP2040 = 0xE48BFF56;
    static constexpr quint32 familyRP2350 = 0xE48BFF59;

    explicit AppFlash(QObject *parent = nullptr);
    ~AppFlash();

    /// @brief      Starts the update: downloads the file, checks it, then waits for the board.
    /// @param      url         where to get the .uf2 from
    /// @param      size        size in bytes announced by GitHub (0 if unknown)
    /// @param      sha256      checksum as lowercase hex provided by GitHub (empty if there is none)
    /// @param      fileName    name of the release file
    /// @param      boardChip   chip of the board being updated, if known
    void Start(const QUrl &url, qint64 size, const QString &sha256, const QString &fileName, Chip boardChip);

    /// @brief      Starts the update from a .uf2 file on this computer instead of a download
    /// @details    The file is first copied into the job's temporary folder, so that what gets checked
    ///             is exactly what gets written, even if the original changes in between. After that,
    ///             everything is like with a download, except that there is no published checksum.
    /// @param      path        the .uf2 file
    /// @param      boardChip   chip of the board being updated, if known
    void StartLocal(const QString &path, Chip boardChip);

    /// @brief      Stops the job; not possible while the file is being copied
    void Cancel();

    /// @brief      Answers a ConfirmRequested()
    void Confirm(bool accepted);

    /// @brief      Forgets how the last job ended, so that a new one can be offered; no effect while one is running
    void Reset();

    State state() const { return currentState; }
    bool  IsActive()  const;
    bool  IsCopying() const { return currentState == stateCopying; }

    //// Pure helpers, public so that they can be tested on their own

    /// @brief      Checks that a file is a plausible UF2 for an RP2040/RP2350
    static Uf2Info InspectUf2(const QString &path);

    /// @brief      Decides if a mounted volume is an RP2040/RP2350 UF2 bootloader (reads INFO_UF2.TXT)
    /// @details    Only writable FAT volumes holding INFO_UF2.TXT are even looked at. The volume's name
    ///             is never used to identify it.
    static VolumeClass ClassifyVolume(const QString &rootPath, const QString &fsType, bool readOnly, const QString &label, Volume *out);

    /// @brief      Looks at all mounted volumes. Blocking: call from a worker thread.
    static ScanResult Scan();

    /// @brief      Name of the release file for a board type as reported by the firmware ("rpipico2"), empty if none
    static QString AssetNameForBoard(const QByteArray &boardType);

    /// @brief      Chip a board type is built around
    static Chip ChipForBoard(const QByteArray &boardType);

    /// @brief      A file name that is safe to write to a FAT drive, always ending in .uf2
    static QString SafeFileName(const QString &name);

signals:
    /// @brief      Rich text describing what's going on or what the user should do next
    void Status(const QString &text);

    /// @brief      Download/copy progress; total is 0 when the size isn't known (busy), negative to hide it
    void Progress(qint64 done, qint64 total);

    void StateChanged(AppFlash::State newState);

    /// @brief      The user has to approve something, answer with Confirm()
    void ConfirmRequested(int request, const QString &title, const QString &text);

    /// @brief      The job ended, successfully or not
    void Finished(bool ok, const QString &message);

private slots:
    void download_readyRead();
    void download_finished();
    void poll();
    void scan_finished();
    void copy_finished();
    void prepare_finished();

private:
    struct PrepareResult {
        bool    ok = false;
        QString error;
        QString sha256;
        quint32 job = 0;
    };

    struct CopyResult {
        bool    dataComplete = false;   ///< every byte was handed to the drive
        QString error;
    };

    void SetState(State newState);
    void Fail(const QString &message);
    void End(State finalState, bool ok, const QString &message);
    void DiscardDownload();
    bool BeginJob(const QString &name, Chip chip);
    void AfterFileReady();
    static PrepareResult PrepareLocal(const QString &source, const QString &dest, quint32 job);
    void BeginWaitingForBootloader();
    void HandleBootloaderScan(const ScanResult &result);
    void HandleRebootScan(const ScanResult &result);
    void StartCopy();
    static CopyResult CopyToVolume(const Volume &volume, const QString &source, const QString &destName, AppFlash *reporter);

    QNetworkAccessManager manager;
    QNetworkReply *reply = nullptr;
    std::unique_ptr<QTemporaryDir> tempDir;
    QFile        partFile;
    QCryptographicHash hash{QCryptographicHash::Sha256};

    State   currentState = stateIdle;
    bool    userCanceled = false;
    QString failureText;        ///< set by download_readyRead() when it aborts the transfer itself

    QString fileName;
    QString filePath;           ///< finished, checked download
    QString expectedSha256;
    qint64  expectedSize = 0;
    qint64  received = 0;
    Chip    boardChip = chipUnknown;
    Uf2Info fileInfo;
    QString checksumNote;

    int     pendingRequest = requestWarnings;

    QTimer  pollTimer;
    QFutureWatcher<ScanResult> scanWatcher;
    QFutureWatcher<CopyResult> copyWatcher;
    QFutureWatcher<PrepareResult> prepareWatcher;
    quint32 jobId = 0;          ///< tells results of a cancelled or replaced job from the current one
    Volume  candidate;
    int     stableScans = 0;
    Volume  target;
    QElapsedTimer rebootTimer;
};

#endif // APPFLASH_H
