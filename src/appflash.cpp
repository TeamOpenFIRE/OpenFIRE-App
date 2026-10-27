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

#include "appflash.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryDir>
#include <QStorageInfo>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QtEndian>
#include <QtConcurrent>

#if defined(Q_OS_WIN)
    #include <io.h>
#elif defined(Q_OS_UNIX)
    #include <unistd.h>
#endif

// Inactivity limit for the download, in ms
static constexpr int downloadTimeout = 30000;

// Upper limits so that nothing absurd is ever taken in
static constexpr qint64 maxDownloadSize = 64 * 1024 * 1024;
static constexpr qint64 maxUf2Size      = 64 * 1024 * 1024;

// How long the bootloader drive may stay after the copy before it's assumed that the board didn't restart, in ms
static constexpr int rebootTimeout = 20000;

// INFO_UF2.TXT is tiny, never read more than this
static constexpr qint64 maxInfoSize = 4096;

// UF2 format
static constexpr quint32 uf2MagicStart0 = 0x0A324655;
static constexpr quint32 uf2MagicStart1 = 0x9E5D5157;
static constexpr quint32 uf2MagicEnd    = 0x0AB16F30;
static constexpr quint32 uf2FlagNotMainFlash   = 0x00000001;
static constexpr quint32 uf2FlagFileContainer  = 0x00001000;
static constexpr quint32 uf2FlagFamilyPresent  = 0x00002000;

// Raspberry Pi's UF2 family IDs. The "absolute" and "data" ones accompany the RP2350 images.
static constexpr quint32 familyAbsolute  = 0xE48BFF57;
static constexpr quint32 familyData      = 0xE48BFF58;
static constexpr quint32 familyRP2350Arm = 0xE48BFF59;
static constexpr quint32 familyRP2350Rv  = 0xE48BFF5A;
static constexpr quint32 familyRP2350Ns  = 0xE48BFF5B;


static QString ChipName(AppFlash::Chip chip)
{
    switch(chip) {
    case AppFlash::chipRP2040: return "RP2040";
    case AppFlash::chipRP2350: return "RP2350";
    default:                   return QObject::tr("unknown chip");
    }
}

static QString Html(const QString &text)
{
    return text.toHtmlEscaped();
}

// Pushes written data to the drive as far as the OS lets us; the drive may well be gone by then, so no error handling.
static void SyncFile(QFile &file)
{
    file.flush();
#if defined(Q_OS_WIN)
    _commit(file.handle());
#elif defined(Q_OS_UNIX)
    ::fsync(file.handle());
#endif
}


AppFlash::AppFlash(QObject *parent)
    : QObject(parent)
{
    pollTimer.setInterval(1000);
    connect(&pollTimer, &QTimer::timeout, this, &AppFlash::poll);
    connect(&scanWatcher, &QFutureWatcher<ScanResult>::finished, this, &AppFlash::scan_finished);
    connect(&copyWatcher, &QFutureWatcher<CopyResult>::finished, this, &AppFlash::copy_finished);
    connect(&prepareWatcher, &QFutureWatcher<PrepareResult>::finished, this, &AppFlash::prepare_finished);
}

AppFlash::~AppFlash()
{
    if(reply != nullptr) {
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
}


//// Pure helpers

QString AppFlash::AssetNameForBoard(const QByteArray &boardType)
{
    // board type as reported by the firmware -> the part of the release file's name that stands for it
    static const QMap<QByteArray, QString> names = {
        {"rpipico",            "rpipico"},
        {"rpipicow",           "rpipicow-noBT"},
        {"rpipico2",           "rpipico2"},
        {"rpipico2w",          "rpipico2w-noBT"},
        {"adafruitItsyRP2040", "adafruitItsy"},
        {"adafruitKB2040",     "adafruitKB2040"},
        {"arduinoNanoRP2040",  "arduinoNano"},
        {"waveshareZero",      "waveshareZero"},
        {"vccgndYD",           "vccgndYD"},
        {"generic-rp2350",     "generic_rp2350"},
        {"generic",            "generic"}
    };

    const auto it = names.constFind(boardType);
    return it == names.cend() ? QString() : "OpenFIREfw." + it.value() + ".uf2";
}

AppFlash::Chip AppFlash::ChipForBoard(const QByteArray &boardType)
{
    if(boardType == "rpipico2" || boardType == "rpipico2w" || boardType == "generic-rp2350")
        return chipRP2350;

    if(!AssetNameForBoard(boardType).isEmpty())
        return chipRP2040;

    return chipUnknown;
}

QString AppFlash::SafeFileName(const QString &name)
{
    QString safe;
    for(const QChar &c : name)
        safe += ((c.unicode() < 128 && c.isLetterOrNumber()) || c == '.' || c == '-' || c == '_') ? c : QChar('_');

    if(safe.endsWith(".uf2", Qt::CaseInsensitive))
        safe.chop(4);
    if(safe.isEmpty() || safe.startsWith('.'))
        safe.prepend("firmware");
    safe = safe.left(60);

    return safe + ".uf2";
}

AppFlash::Uf2Info AppFlash::InspectUf2(const QString &path)
{
    Uf2Info info;

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        info.error = tr("The file could not be read.");
        return info;
    }

    const qint64 size = file.size();
    if(size <= 0 || size % 512 != 0) {
        info.error = tr("This is not a UF2 file (its size is not a multiple of 512 bytes).");
        return info;
    }
    if(size > maxUf2Size) {
        info.error = tr("The file is far larger than any firmware for these boards.");
        return info;
    }

    // Blocks are numbered per family: a family's blocks all agree how many there are, and run 0, 1, 2...
    // Without a family ID (key -1), the file as a whole counts as one.
    struct FamilyStats {
        quint32 count = 0;
        quint32 numBlocks = 0;
        bool    numberingOk = true;
    };
    QMap<qint64, FamilyStats> families;

    QByteArray block(512, 0);
    const uchar *data = reinterpret_cast<const uchar *>(block.constData());
    auto word = [data](int offset) { return qFromLittleEndian<quint32>(data + offset); };

    const quint32 blocks = static_cast<quint32>(size / 512);
    for(quint32 i = 0; i < blocks; ++i) {
        if(file.read(block.data(), 512) != 512) {
            info.error = tr("The file could not be read completely.");
            return info;
        }

        if(word(0) != uf2MagicStart0 || word(4) != uf2MagicStart1 || word(508) != uf2MagicEnd) {
            info.error = tr("This is not a UF2 file (block %1 has no UF2 markers).").arg(i);
            return info;
        }

        const quint32 flags = word(8);
        const quint32 payload = word(16);

        // bootloaders skip these
        if(flags & uf2FlagNotMainFlash)
            continue;

        if(flags & uf2FlagFileContainer) {
            info.error = tr("This UF2 file is a file container, which these boards can't use.");
            return info;
        }

        if(payload > 476) {
            info.error = tr("This UF2 file is damaged (block %1 claims %2 bytes of data).").arg(i).arg(payload);
            return info;
        }

        FamilyStats &stats = families[(flags & uf2FlagFamilyPresent) ? static_cast<qint64>(word(28)) : -1];
        if(stats.count == 0)
            stats.numBlocks = word(24);
        else if(word(24) != stats.numBlocks)
            stats.numberingOk = false;
        if(word(20) != stats.count)
            stats.numberingOk = false;
        ++stats.count;
    }

    if(families.isEmpty()) {
        info.error = tr("This UF2 file contains no data for the board.");
        return info;
    }

    bool rp2040 = false, rp2350 = false;
    QStringList unknownFamilies;
    for(auto it = families.cbegin(); it != families.cend(); ++it) {
        const qint64 family = it.key();

        // the "absolute" and "data" blocks of RP2350 images are numbered on their own terms
        if(family != familyAbsolute && family != familyData) {
            if(!it.value().numberingOk || it.value().count != it.value().numBlocks) {
                info.error = tr("This UF2 file is damaged or incomplete (its blocks are not numbered consistently).");
                return info;
            }
        }

        if(family == familyRP2040)
            rp2040 = true;
        else if(family == familyRP2350Arm || family == familyRP2350Rv || family == familyRP2350Ns)
            rp2350 = true;
        else if(family != -1 && family != familyAbsolute && family != familyData)
            unknownFamilies << QString("0x%1").arg(family, 8, 16, QLatin1Char('0')).toUpper().replace("0X", "0x");
    }

    if(rp2040 && rp2350) {
        info.error = tr("This UF2 file mixes RP2040 and RP2350 firmware.");
        return info;
    }

    info.chip = rp2040 ? chipRP2040 : (rp2350 ? chipRP2350 : chipUnknown);
    info.blocks = blocks;
    info.valid = true;

    // Not claiming more than we know: the family only says what chip it's built for, not that it suits a specific board.
    if(info.chip == chipUnknown)
        info.warnings << tr("The file does not say which chip it is built for (no RP2040 or RP2350 family ID), so it can't be checked against your board.");
    if(!unknownFamilies.isEmpty())
        info.warnings << tr("The file also contains data for other families (%1).").arg(unknownFamilies.join(", "));
    if(families.contains(-1) && families.size() > 1)
        info.warnings << tr("Some blocks in the file have no family ID.");

    return info;
}

AppFlash::VolumeClass AppFlash::ClassifyVolume(const QString &rootPath, const QString &fsType, bool readOnly, const QString &label, Volume *out)
{
    // The bootloaders of these boards present a writable FAT drive. Anything else is not even looked at,
    // which also keeps this away from network shares and other drives that may be slow to answer.
    const QString fs = fsType.toLower();
    if(readOnly || !(fs.contains("fat") || fs == "msdos"))
        return volumeNone;

    const QFileInfo markerInfo(QDir(rootPath).filePath("INFO_UF2.TXT"));
    if(!markerInfo.isFile())
        return volumeNone;

    QFile marker(markerInfo.filePath());
    if(!marker.open(QIODevice::ReadOnly))
        return volumeNone;

    const QString text = QString::fromUtf8(marker.read(maxInfoSize)).trimmed();
    const QString lower = text.toLower();

    // The format of the marker varies between bootloaders and versions, so just look for what the
    // RP2040/RP2350 boot ROMs call themselves ("RPI-RP2", "RP2350", "Raspberry Pi RP2") and nothing exact.
    // The drive's label is no help: it varies with the board.
    if(!lower.contains("rp2"))
        return volumeOtherUf2;

    if(out != nullptr) {
        Volume volume;
        volume.rootPath = rootPath;
        volume.name = label;
        volume.infoText = text;

        const QStringList lines = text.split('\n');
        for(const QString &line : lines) {
            const int colon = line.indexOf(':');
            if(colon < 0)
                continue;

            const QString key = line.left(colon).trimmed().toLower();
            const QString value = line.mid(colon + 1).trimmed();
            if(key == "model")
                volume.model = value;
            else if(key == "board-id")
                volume.boardId = value;
        }

        if(lower.contains("rp2350"))
            volume.chip = chipRP2350;
        else if(lower.contains("rp2040") || lower.contains("rpi-rp2"))
            volume.chip = chipRP2040;

        *out = volume;
    }

    return volumeRP;
}

AppFlash::ScanResult AppFlash::Scan()
{
    ScanResult result;
    QSet<QByteArray> seenDevices;

    const QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    for(const QStorageInfo &storage : volumes) {
        if(!storage.isValid() || !storage.isReady())
            continue;

        // the same drive may be mounted in more than one place; that's still one board
        if(!storage.device().isEmpty()) {
            if(seenDevices.contains(storage.device()))
                continue;
            seenDevices.insert(storage.device());
        }

        Volume volume;
        switch(ClassifyVolume(storage.rootPath(), storage.fileSystemType(), storage.isReadOnly(), storage.name(), &volume)) {
        case volumeRP:
            result.bootloaders << volume;
            break;
        case volumeOtherUf2:
            result.otherUf2 << (storage.name().isEmpty() ? storage.rootPath() : storage.name() + " (" + storage.rootPath() + ')');
            break;
        default:
            break;
        }
    }

    return result;
}


//// Job

bool AppFlash::IsActive() const
{
    return currentState >= statePreparing && currentState <= stateWaitingReboot;
}

void AppFlash::SetState(State newState)
{
    if(currentState == newState)
        return;

    currentState = newState;
    emit StateChanged(newState);
}

void AppFlash::End(State finalState, bool ok, const QString &message)
{
    pollTimer.stop();
    SetState(finalState);
    emit Progress(0, -1);
    emit Status(Html(message).replace('\n', "<br>"));
    emit Finished(ok, message);
}

void AppFlash::Fail(const QString &message)
{
    End(stateFailed, false, message);
}

void AppFlash::DiscardDownload()
{
    partFile.close();
    if(!partFile.fileName().isEmpty())
        QFile::remove(partFile.fileName());
}

bool AppFlash::BeginJob(const QString &name, Chip chip)
{
    ++jobId;
    userCanceled = false;
    failureText.clear();
    boardChip = chip;
    fileName = SafeFileName(name);
    filePath.clear();
    fileInfo = Uf2Info();
    checksumNote.clear();
    received = 0;
    hash.reset();

    // A fresh folder for every attempt; it's removed with this object, so the file stays
    // untouched (and can be reused for a retry) until the app is closed.
    tempDir.reset(new QTemporaryDir());
    if(!tempDir->isValid()) {
        Fail(tr("A temporary folder for the firmware could not be created."));
        return false;
    }

    return true;
}

void AppFlash::Start(const QUrl &url, qint64 size, const QString &sha256, const QString &name, Chip chip)
{
    if(IsActive())
        return;

    expectedSize = size;
    expectedSha256 = sha256.toLower();
    if(!BeginJob(name, chip))
        return;

    partFile.setFileName(tempDir->filePath(fileName + ".part"));
    if(!partFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        Fail(tr("The download could not be saved (%1).").arg(partFile.errorString()));
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/octet-stream");
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArray("OpenFIREapp/") + OFAPP_VERSION);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(downloadTimeout);

    SetState(stateDownloading);
    emit Progress(0, expectedSize > 0 ? expectedSize : 0);
    emit Status(tr("<b>Step 1: downloading the firmware…</b>"));

    reply = manager.get(request);
    connect(reply, &QNetworkReply::readyRead, this, &AppFlash::download_readyRead);
    connect(reply, &QNetworkReply::finished,  this, &AppFlash::download_finished);
}

void AppFlash::StartLocal(const QString &path, Chip chip)
{
    if(IsActive())
        return;

    expectedSize = 0;
    expectedSha256.clear();
    if(!BeginJob(QFileInfo(path).fileName(), chip))
        return;

    SetState(statePreparing);
    emit Progress(0, 0);
    emit Status(tr("<b>Step 1: reading the firmware file…</b>"));

    // reading it can be slow (network drives, removable media), so not on this thread
    const QString dest = tempDir->filePath(fileName);
    const quint32 job = jobId;
    prepareWatcher.setFuture(QtConcurrent::run([path, dest, job]() { return AppFlash::PrepareLocal(path, dest, job); }));
}

AppFlash::PrepareResult AppFlash::PrepareLocal(const QString &source, const QString &dest, quint32 job)
{
    PrepareResult result;
    result.job = job;

    const QFileInfo info(source);
    if(!info.exists() || !info.isFile()) {
        result.error = tr("The selected file does not exist or is not a regular file.");
        return result;
    }

    const qint64 size = info.size();
    if(size <= 0) {
        result.error = tr("The selected file is empty.");
        return result;
    }
    if(size > maxUf2Size) {
        result.error = tr("The selected file is far larger than any firmware for these boards.");
        return result;
    }

    QFile in(source);
    if(!in.open(QIODevice::ReadOnly)) {
        result.error = tr("The selected file could not be read (%1).").arg(in.errorString());
        return result;
    }

    QFile out(dest);
    if(!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.error = tr("The file could not be copied for checking (%1).").arg(out.errorString());
        return result;
    }

    QCryptographicHash fileHash(QCryptographicHash::Sha256);
    QByteArray buffer(65536, 0);
    qint64 total = 0;
    qint64 count;
    while((count = in.read(buffer.data(), buffer.size())) > 0) {
        total += count;
        if(total > maxUf2Size) {
            result.error = tr("The selected file is far larger than any firmware for these boards.");
            return result;
        }

        if(out.write(buffer.constData(), count) != count) {
            result.error = tr("The file could not be copied for checking (%1). Is the disk full?").arg(out.errorString());
            return result;
        }

        fileHash.addData(QByteArray::fromRawData(buffer.constData(), static_cast<int>(count)));
    }

    if(count < 0) {
        result.error = tr("The selected file could not be read (%1).").arg(in.errorString());
        return result;
    }

    out.flush();
    out.close();

    // a file that is being written to while it's read can't be trusted
    if(total != size) {
        result.error = tr("The selected file changed while it was being read.");
        return result;
    }

    result.sha256 = QString::fromLatin1(fileHash.result().toHex());
    result.ok = true;
    return result;
}

void AppFlash::prepare_finished()
{
    const PrepareResult result = prepareWatcher.result();

    // cancelled, or a newer job has begun in the meantime
    if(result.job != jobId || currentState != statePreparing)
        return;

    if(!result.ok) {
        Fail(result.error);
        return;
    }

    filePath = tempDir->filePath(fileName);

    // Only claim what was actually checked: nothing here says where the file comes from.
    checksumNote = tr("This is a file from your computer (SHA-256: %1). It could not be checked against a published checksum, "
                      "so only use firmware from a source you trust.").arg(result.sha256);

    AfterFileReady();
}

void AppFlash::download_readyRead()
{
    if(reply == nullptr)
        return;

    const QByteArray chunk = reply->readAll();
    if(chunk.isEmpty())
        return;

    // Never take in more than was announced (or something absurd if nothing was).
    const qint64 limit = expectedSize > 0 ? expectedSize + 4096 : maxDownloadSize;
    if(received + chunk.size() > limit) {
        failureText = tr("The download is larger than expected, so it was stopped.");
        reply->abort();
        return;
    }

    if(partFile.write(chunk) != chunk.size()) {
        failureText = tr("The download could not be saved (%1). Is the disk full?").arg(partFile.errorString());
        reply->abort();
        return;
    }

    hash.addData(chunk);
    received += chunk.size();
    emit Progress(received, expectedSize > 0 ? expectedSize : 0);
}

void AppFlash::download_finished()
{
    QNetworkReply *finishedReply = reply;
    reply = nullptr;
    if(finishedReply == nullptr)
        return;

    finishedReply->deleteLater();

    // whatever was still unread
    if(finishedReply->error() == QNetworkReply::NoError && failureText.isEmpty() && !userCanceled) {
        reply = finishedReply;
        download_readyRead();
        reply = nullptr;
    }

    partFile.flush();
    partFile.close();

    if(userCanceled) {
        DiscardDownload();
        End(stateIdle, false, tr("Update cancelled. Nothing was written to the board."));
        return;
    }

    if(!failureText.isEmpty()) {
        DiscardDownload();
        Fail(failureText);
        return;
    }

    const int status = finishedReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if(finishedReply->error() != QNetworkReply::NoError || status != 200) {
        QString why;
        // (a cancel by the user or by us was dealt with above, so Qt 6 reporting "canceled" here is its way of saying "timed out")
        if(finishedReply->error() == QNetworkReply::TimeoutError || finishedReply->error() == QNetworkReply::OperationCanceledError)
            why = tr("the connection timed out");
        else if(status == 429 || status == 403)
            why = tr("GitHub refused the download (HTTP %1), possibly because of a rate limit; try again later").arg(status);
        else if(status > 0 && status != 200)
            why = tr("GitHub answered with an error (HTTP %1)").arg(status);
        else
            why = finishedReply->errorString();

        DiscardDownload();
        Fail(tr("The download failed: %1.").arg(why));
        return;
    }

    if(received == 0 || (expectedSize > 0 && received != expectedSize)) {
        DiscardDownload();
        Fail(tr("The download is incomplete (%1 of %2 bytes), so it was discarded.").arg(received).arg(expectedSize));
        return;
    }

    // Only claim what was actually checked.
    const QString actual = QString::fromLatin1(hash.result().toHex());
    if(!expectedSha256.isEmpty()) {
        if(actual != expectedSha256) {
            DiscardDownload();
            Fail(tr("The downloaded file does not match the checksum published by GitHub, so it was discarded."));
            return;
        }
        checksumNote = tr("The file's SHA-256 checksum matches the one published by GitHub.");
    } else {
        checksumNote = tr("GitHub publishes no checksum for this file, so it could not be verified.");
    }

    const QString finalPath = tempDir->filePath(fileName);
    QFile::remove(finalPath);
    if(!QFile::rename(partFile.fileName(), finalPath)) {
        DiscardDownload();
        Fail(tr("The downloaded file could not be moved into place."));
        return;
    }

    filePath = finalPath;
    AfterFileReady();
}

void AppFlash::AfterFileReady()
{
    fileInfo = InspectUf2(filePath);

    if(!fileInfo.valid) {
        QFile::remove(filePath);
        filePath.clear();
        Fail(tr("The downloaded file is not usable as firmware: %1").arg(fileInfo.error));
        return;
    }

    if(boardChip != chipUnknown && fileInfo.chip != chipUnknown && fileInfo.chip != boardChip) {
        QFile::remove(filePath);
        filePath.clear();
        Fail(tr("The downloaded firmware is built for an %1, but your board has an %2. Nothing was written to the board.")
             .arg(ChipName(fileInfo.chip), ChipName(boardChip)));
        return;
    }

    if(!fileInfo.warnings.isEmpty()) {
        pendingRequest = requestWarnings;
        SetState(stateConfirming);
        emit Progress(0, -1);
        emit ConfirmRequested(requestWarnings, tr("Firmware file warning"),
                              tr("The downloaded firmware could not be fully checked:\n\n - %1\n\nDo you want to continue anyway?")
                              .arg(fileInfo.warnings.join("\n - ")));
        return;
    }

    BeginWaitingForBootloader();
}

void AppFlash::Confirm(bool accepted)
{
    if(currentState != stateConfirming)
        return;

    if(!accepted) {
        End(stateIdle, false, tr("Update cancelled. Nothing was written to the board."));
        return;
    }

    if(pendingRequest == requestWarnings)
        BeginWaitingForBootloader();
    else
        StartCopy();
}

void AppFlash::Reset()
{
    if(!IsActive())
        SetState(stateIdle);
}

void AppFlash::Cancel()
{
    switch(currentState) {
    case stateDownloading:
        userCanceled = true;
        if(reply != nullptr)
            reply->abort();
        break;
    case statePreparing:
        // the reading can't be interrupted, but its result is dropped
        ++jobId;
        End(stateIdle, false, tr("Update cancelled. Nothing was written to the board."));
        break;
    case stateWaitingBootloader:
    case stateConfirming:
        End(stateIdle, false, tr("Update cancelled. Nothing was written to the board."));
        break;
    case stateWaitingReboot:
        End(stateIdle, false, tr("Stopped waiting for the board to restart."));
        break;
    default:
        // a copy in progress is not interrupted
        break;
    }
}

void AppFlash::BeginWaitingForBootloader()
{
    candidate = Volume();
    stableScans = 0;

    SetState(stateWaitingBootloader);
    emit Progress(0, -1);
    emit Status(tr("<b>Step 2: put the gun into bootloader mode.</b><br>"
                   "Click <i>Reboot to Bootloader</i> below, or hold the BOOTSEL button while plugging the board in. "
                   "Have <b>only one</b> board in bootloader mode at a time.<br><br>"
                   "%1<br>Waiting for a bootloader drive…").arg(Html(checksumNote)));

    pollTimer.start();
    poll();
}

void AppFlash::poll()
{
    if(scanWatcher.isRunning())
        return;

    if(currentState != stateWaitingBootloader && currentState != stateWaitingReboot)
        return;

    // reading the drives can be slow, so never on this thread
    scanWatcher.setFuture(QtConcurrent::run([]() { return AppFlash::Scan(); }));
}

void AppFlash::scan_finished()
{
    const ScanResult result = scanWatcher.result();

    if(currentState == stateWaitingBootloader)
        HandleBootloaderScan(result);
    else if(currentState == stateWaitingReboot)
        HandleRebootScan(result);
}

void AppFlash::HandleBootloaderScan(const ScanResult &result)
{
    const QString header = tr("<b>Step 2: put the gun into bootloader mode.</b><br>"
                              "Click <i>Reboot to Bootloader</i> below, or hold the BOOTSEL button while plugging the board in. "
                              "Have <b>only one</b> board in bootloader mode at a time.<br><br>%1<br>").arg(Html(checksumNote));

    QString others;
    if(!result.otherUf2.isEmpty())
        others = "<br>" + tr("Ignoring UF2 drive(s) that are not an RP2040/RP2350: %1").arg(Html(result.otherUf2.join(", ")));

    if(result.bootloaders.isEmpty()) {
        stableScans = 0;
        candidate = Volume();
        emit Status(header + tr("Waiting for a bootloader drive…") + "<br>" +
                    tr("(If the drive doesn't show up, your system may not be mounting it automatically.)") + others);
        return;
    }

    if(result.bootloaders.size() > 1) {
        stableScans = 0;
        candidate = Volume();

        QStringList names;
        for(const Volume &volume : result.bootloaders)
            names << (volume.name.isEmpty() ? volume.rootPath : volume.name + " (" + volume.rootPath + ')');

        emit Status(header + "<b>" + tr("More than one board is in bootloader mode: %1.").arg(Html(names.join(", "))) + "</b><br>" +
                    tr("Please have only one board in bootloader mode at a time: unplug the others, or restart them without BOOTSEL.") + others);
        return;
    }

    const Volume &volume = result.bootloaders.first();

    // the drive has to be there, unchanged, on two checks in a row before it's trusted
    if(volume.rootPath != candidate.rootPath || volume.infoText != candidate.infoText) {
        candidate = volume;
        stableScans = 1;
        emit Status(header + tr("Found a bootloader drive, checking it…") + others);
        return;
    }

    if(++stableScans < 2)
        return;

    // the file and the board have to agree where both say what they are
    if(volume.chip != chipUnknown && fileInfo.chip != chipUnknown && volume.chip != fileInfo.chip) {
        emit Status(header + "<b>" + tr("The board in bootloader mode is an %1, but this firmware is built for an %2. "
                                       "It was not written. Connect the matching board.")
                    .arg(ChipName(volume.chip), ChipName(fileInfo.chip)) + "</b>" + others);
        return;
    }

    pollTimer.stop();
    SetState(stateConfirming);
    pendingRequest = requestFlash;

    const QString board = volume.model.isEmpty() ? volume.infoText.section('\n', 0, 0).trimmed() : volume.model;
    emit ConfirmRequested(requestFlash, tr("Install firmware"),
                          tr("Install the firmware on this drive?\n\n"
                             "Drive: %1 (%2)\n"
                             "Bootloader: %3%4\n"
                             "Firmware: %5, built for %6\n"
                             "%7\n\n"
                             "Only this one file is copied to the drive. The board then restarts into the new firmware "
                             "and the drive disappears.")
                          .arg(volume.name.isEmpty() ? tr("(no label)") : volume.name, volume.rootPath, board,
                               volume.boardId.isEmpty() ? QString() : " (Board-ID " + volume.boardId + ")",
                               fileName, ChipName(fileInfo.chip), checksumNote));
}

void AppFlash::StartCopy()
{
    target = candidate;

    SetState(stateCopying);
    const qint64 total = QFileInfo(filePath).size();
    emit Progress(0, total);
    emit Status(tr("<b>Step 3: installing…</b><br>Copying the firmware to %1. Do not unplug the board.")
                .arg(Html(target.name.isEmpty() ? target.rootPath : target.name)));

    const Volume volume = target;
    const QString source = filePath;
    const QString destName = fileName;
    copyWatcher.setFuture(QtConcurrent::run([volume, source, destName, this]() {
        return AppFlash::CopyToVolume(volume, source, destName, this);
    }));
}

AppFlash::CopyResult AppFlash::CopyToVolume(const Volume &volume, const QString &source, const QString &destName, AppFlash *reporter)
{
    CopyResult result;

    // The board may have been swapped or removed since it was confirmed: write only to the very same,
    // unchanged bootloader drive.
    QStorageInfo storage(volume.rootPath);
    Volume current;
    if(!storage.isValid() || !storage.isReady() || storage.rootPath() != volume.rootPath ||
       ClassifyVolume(storage.rootPath(), storage.fileSystemType(), storage.isReadOnly(), storage.name(), &current) != volumeRP ||
       current.infoText != volume.infoText) {
        result.error = tr("The bootloader drive is gone or has changed since it was confirmed.");
        return result;
    }

    QFile src(source);
    if(!src.open(QIODevice::ReadOnly)) {
        result.error = tr("The downloaded firmware file could not be read (%1).").arg(src.errorString());
        return result;
    }

    const qint64 total = src.size();

    QFile dst(QDir(volume.rootPath).filePath(destName));
    if(!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.error = tr("The file could not be created on the drive (%1).").arg(dst.errorString());
        return result;
    }

    QByteArray buffer(32768, 0);
    qint64 done = 0;
    int lastPercent = -1;
    while(done < total) {
        const qint64 count = src.read(buffer.data(), buffer.size());
        if(count <= 0) {
            result.error = tr("The downloaded firmware file could not be read (%1).").arg(src.errorString());
            return result;
        }

        if(dst.write(buffer.constData(), count) != count) {
            result.error = tr("Writing to the drive failed after %1 of %2 bytes. The drive may be full, or the board may have been unplugged.")
                           .arg(done).arg(total);
            return result;
        }

        done += count;

        const int percent = static_cast<int>(done * 100 / total);
        if(percent != lastPercent) {
            lastPercent = percent;
            QMetaObject::invokeMethod(reporter, [reporter, done, total]() { emit reporter->Progress(done, total); }, Qt::QueuedConnection);
        }
    }

    // Everything has been handed over. The board restarts the moment it has received the last block, so
    // flushing and closing may well fail on a drive that's already gone: that's what's supposed to happen.
    result.dataComplete = true;
    SyncFile(dst);
    dst.close();

    return result;
}

void AppFlash::copy_finished()
{
    const CopyResult result = copyWatcher.result();

    if(!result.dataComplete) {
        Fail(tr("The firmware could not be copied: %1\n\n"
                "Nothing is damaged: the bootloader of the board lives in the chip and can't be overwritten. "
                "Put the board in bootloader mode again and retry.").arg(result.error));
        return;
    }

    SetState(stateWaitingReboot);
    rebootTimer.start();
    emit Progress(0, -1);
    emit Status(tr("<b>Step 3: installing…</b><br>The firmware was copied. Waiting for the board to restart…"));

    pollTimer.start();
    poll();
}

void AppFlash::HandleRebootScan(const ScanResult &result)
{
    bool stillThere = false;
    for(const Volume &volume : result.bootloaders)
        if(volume.rootPath == target.rootPath)
            stillThere = true;

    // The drive vanishing is exactly what's expected: the board restarts into the new firmware.
    if(!stillThere) {
        End(stateDone, true, tr("The firmware was installed and the board's bootloader drive disappeared, as expected when it restarts."));
        return;
    }

    if(rebootTimer.elapsed() > rebootTimeout) {
        Fail(tr("The firmware was copied, but the board is still in bootloader mode after %1 seconds. "
                "It may have refused the file. Unplug the board and plug it in again without holding BOOTSEL; "
                "if it is still in bootloader mode, try the update again.").arg(rebootTimeout / 1000));
    }
}
