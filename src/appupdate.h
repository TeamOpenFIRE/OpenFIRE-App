/*  OpenFIRE App: a configuration utility for the OpenFIRE light gun system.
    Firmware release lookup.

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

#ifndef APPUPDATE_H
#define APPUPDATE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUrl>
#include <QJsonObject>
#include <QVersionNumber>
#include <QNetworkAccessManager>

/// @brief  Looks up the latest stable firmware release on GitHub.
/// @details    Requests are asynchronous and run on the event loop without blocking it,
///             so a slow or unreachable GitHub never holds up the rest of the app.
class AppUpdate : public QObject
{
    Q_OBJECT

public:
    enum State {
        stateIdle,
        stateChecking,
        stateSuccess,
        stateFailed
    };

    /// @brief      Web page listing the firmware releases
    static inline const QString releasesPage = "https://github.com/TeamOpenFIRE/OpenFIRE-Firmware/releases";

    /// @brief      A file attached to the release
    struct Asset {
        QString name;
        QUrl    url;
        qint64  size = 0;
        QString sha256;     ///< lowercase hex as provided by GitHub, empty if it didn't provide one
    };

    explicit AppUpdate(QObject *parent = nullptr);

    /// @brief      Starts looking up the latest stable release; returns right away.
    /// @details    Does nothing if a lookup is already running. Emits Finished() when done.
    void CheckLatest();

    State   state()     const { return currentState; }
    QString latestTag() const { return tag; }
    QString latestUrl() const { return url; }

    /// @brief      Files attached to the latest release (only valid after a successful lookup)
    const QList<Asset> &assets() const { return releaseAssets; }

    /// @brief      Finds a release file by its exact name; null pointer if the release has none
    const Asset *FindAsset(const QString &name) const;

    /// @brief      Why the last lookup failed, ready to show to the user (empty if it didn't)
    QString errorText() const { return error; }

    /// @brief      Considers one release of the GitHub release list for being the latest stable one
    /// @details    Drafts, pre-releases and tags without a version number are skipped. The highest
    ///             version wins, whatever the order of the list or the dates of the releases; on a tie
    ///             the one listed first (the newest) stays.
    /// @param      release     one entry of GitHub's list of releases
    /// @param      best        the best release found so far, replaced if this one is better
    /// @param      bestVersion version of best, updated along with it
    /// @return     whether release became the best
    static bool ConsiderRelease(const QJsonObject &release, QJsonObject &best, QVersionNumber &bestVersion);

    /// @brief      The URL of the next page of results out of a Link header, empty if there is none
    /// @details    Only URLs into GitHub's releases API of this project are accepted.
    static QUrl NextPageUrl(const QByteArray &linkHeader);

    /// @brief      Extracts the version number out of a tag or firmware version string
    /// @details    "v6.3" -> 6.3, "6.3-abc123" -> 6.3. Trailing zeros don't count ("6.3.0" is "6.3").
    ///             Null version if there is none.
    static QVersionNumber ParseVersion(const QString &text);

signals:
    /// @brief      A lookup has completed, check state() for the outcome
    void Finished();

private:
    void Request(const QUrl &url);
    void reply_finished(QNetworkReply *reply);
    void Fail(const QString &message);

    /// how many releases to ask GitHub for per page (the most it allows)
    static inline int perPage = 100;

    QNetworkAccessManager manager;
    State   currentState = stateIdle;
    QString tag;
    QString url;
    QString error;
    QList<Asset> releaseAssets;

    // while going through the pages of the release list
    QJsonObject    bestRelease;
    QVersionNumber bestVersion;
    int            pagesFetched = 0;
};

#endif // APPUPDATE_H
