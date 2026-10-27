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

#include "appupdate.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

// How long to wait for GitHub before giving up, in ms
static constexpr int requestTimeout = 10000;

// Safety net against a never-ending list: 10 pages of 100 releases
static constexpr int maxPages = 10;

static const QString releasesApi = "https://api.github.com/repos/TeamOpenFIRE/OpenFIRE-Firmware/releases";

AppUpdate::AppUpdate(QObject *parent)
    : QObject(parent)
{
    connect(&manager, &QNetworkAccessManager::finished, this, &AppUpdate::reply_finished);
}

void AppUpdate::CheckLatest()
{
    if(currentState == stateChecking)
        return;

    currentState = stateChecking;
    error.clear();
    bestRelease = QJsonObject();
    bestVersion = QVersionNumber();
    pagesFetched = 0;

    // The list of all releases rather than GitHub's "latest": that one is the most recently *created*
    // release, which is not necessarily the highest version (think of a fix for 5.1 published after 6.0).
    Request(QUrl(releasesApi + "?per_page=" + QString::number(perPage)));
}

void AppUpdate::Request(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/vnd.github+json");
    // GitHub rejects requests without a user agent
    request.setHeader(QNetworkRequest::UserAgentHeader, QByteArray("OpenFIREapp/") + OFAPP_VERSION);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(requestTimeout);

    manager.get(request);
}

void AppUpdate::Fail(const QString &message)
{
    currentState = stateFailed;
    tag.clear();
    url.clear();
    releaseAssets.clear();
    error = message;

    emit Finished();
}

bool AppUpdate::ConsiderRelease(const QJsonObject &release, QJsonObject &best, QVersionNumber &bestVersion)
{
    if(release.value("draft").toBool() || release.value("prerelease").toBool())
        return false;

    const QVersionNumber version = ParseVersion(release.value("tag_name").toString());
    if(version.isNull())
        return false;

    // only ever link to GitHub, whatever the response claims
    if(!release.value("html_url").toString().startsWith("https://github.com/"))
        return false;

    if(!bestVersion.isNull() && QVersionNumber::compare(version, bestVersion) <= 0)
        return false;

    best = release;
    bestVersion = version;
    return true;
}

QUrl AppUpdate::NextPageUrl(const QByteArray &linkHeader)
{
    // <https://api.github.com/...&page=2>; rel="next", <https://api.github.com/...&page=5>; rel="last"
    const QRegularExpression next("<([^>]+)>\\s*;\\s*rel=\"next\"");
    const QRegularExpressionMatch match = next.match(QString::fromLatin1(linkHeader));
    if(!match.hasMatch())
        return QUrl();

    // GitHub links to its own pages of the list by repository ID ("/repositories/123/releases?page=2"),
    // but accept the form with the name as well. Anything else is not followed.
    const QString candidate = match.captured(1);
    static const QRegularExpression allowed("^https://api\\.github\\.com/(repos/TeamOpenFIRE/OpenFIRE-Firmware|repositories/\\d+)/releases(\\?[^#]*)?$");
    return allowed.match(candidate).hasMatch() ? QUrl(candidate) : QUrl();
}

void AppUpdate::reply_finished(QNetworkReply *reply)
{
    reply->deleteLater();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if(reply->error() != QNetworkReply::NoError && status == 0) {
        Fail(tr("GitHub could not be reached (%1).").arg(reply->errorString()));
        return;
    } else if(status == 429 || (status == 403 && reply->rawHeader("X-RateLimit-Remaining") == "0")) {
        Fail(tr("GitHub's rate limit for this network has been reached. Try again later."));
        return;
    } else if(status == 404) {
        Fail(tr("GitHub has no published release."));
        return;
    } else if(status != 200) {
        Fail(tr("GitHub answered with an error (HTTP %1).").arg(status));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if(!doc.isArray()) {
        Fail(tr("GitHub's answer was not understood."));
        return;
    }

    ++pagesFetched;
    for(const QJsonValue &value : doc.array())
        ConsiderRelease(value.toObject(), bestRelease, bestVersion);

    // A release listed on a later page may be the highest version, so go through all of the pages.
    const QUrl next = NextPageUrl(reply->rawHeader("Link"));
    if(next.isValid() && pagesFetched < maxPages) {
        Request(next);
        return;
    }

    if(bestVersion.isNull()) {
        Fail(tr("GitHub has no published release."));
        return;
    }

    tag = bestRelease.value("tag_name").toString();
    url = bestRelease.value("html_url").toString();
    error.clear();
    releaseAssets.clear();

    for(const QJsonValue &value : bestRelease.value("assets").toArray()) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value("name").toString();
        const QString assetUrl = asset.value("browser_download_url").toString();

        // skip anything that isn't a plain file on github.com
        if(name.isEmpty() || name.contains('/') || name.contains('\\') || !assetUrl.startsWith("https://github.com/"))
            continue;

        Asset entry;
        entry.name = name;
        entry.url  = QUrl(assetUrl);
        entry.size = static_cast<qint64>(asset.value("size").toDouble());

        // "sha256:<64 hex digits>", only when GitHub provides it
        const QString digest = asset.value("digest").toString();
        if(digest.startsWith("sha256:", Qt::CaseInsensitive)) {
            const QString hex = digest.mid(7).toLower();
            if(QRegularExpression("^[0-9a-f]{64}$").match(hex).hasMatch())
                entry.sha256 = hex;
        }

        releaseAssets << entry;
    }

    currentState = stateSuccess;
    emit Finished();
}

const AppUpdate::Asset *AppUpdate::FindAsset(const QString &name) const
{
    for(const Asset &asset : releaseAssets)
        if(asset.name == name)
            return &asset;
    return nullptr;
}

QVersionNumber AppUpdate::ParseVersion(const QString &text)
{
    const QRegularExpressionMatch match = QRegularExpression("\\d+(\\.\\d+)*").match(text);
    // normalized: "6.3.0" and "6.3" are the same version (Qt would rank the first higher)
    return match.hasMatch() ? QVersionNumber::fromString(match.captured()).normalized() : QVersionNumber();
}
