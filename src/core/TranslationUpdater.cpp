#include "core/TranslationUpdater.h"

#include "AppVersion.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTranslator>
#include <utility>

namespace Aurora {
namespace {
constexpr qsizetype maximumCatalogSize = 2 * 1024 * 1024;
constexpr qsizetype maximumManifestSize = 64 * 1024;
}

QUrl TranslationUpdater::repositoryUrl()
{
    return QUrl(QStringLiteral("https://raw.githubusercontent.com/%1/translations/")
        .arg(QString::fromLatin1(Config::GitHubRepository)));
}

TranslationUpdater::TranslationUpdater(QObject *parent, const QString &cachePath, const QUrl &baseUrl)
    : QObject(parent), m_cachePath(cachePath), m_baseUrl(baseUrl)
{
    m_deadline.setSingleShot(true);
    m_deadline.setInterval(5000);
    connect(&m_deadline, &QTimer::timeout, this, &TranslationUpdater::finish);
}

TranslationUpdater::~TranslationUpdater()
{
    for (auto *reply : std::as_const(m_replies)) {
        reply->disconnect(this);
        reply->abort();
    }
}

void TranslationUpdater::refresh()
{
    if (m_running)
        return;
    m_running = true;
    m_changed = false;
    m_deadline.start();
    get(QStringLiteral("index.json"), maximumManifestSize,
        [this](const QByteArray &bytes) { readManifest(bytes); });
}

void TranslationUpdater::get(const QString &file, qsizetype limit,
                             std::function<void(const QByteArray &)> callback)
{
    QNetworkRequest request(m_baseUrl.resolved(QUrl(file)));
    if (file == QStringLiteral("index.json"))
        request.setRawHeader("Cache-Control", "no-cache");
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("immich-desktop-translations"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(5000);
    auto *reply = m_network.get(request);
    m_replies.insert(reply);
    connect(reply, &QNetworkReply::readyRead, this, [reply, limit] {
        if (reply->bytesAvailable() > limit)
            reply->abort();
    });
    connect(reply, &QNetworkReply::downloadProgress, this, [reply, limit](qint64 received, qint64 total) {
        if (received > limit || total > limit)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, limit, callback] {
        m_replies.remove(reply);
        const bool valid = reply->error() == QNetworkReply::NoError &&
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
            reply->bytesAvailable() <= limit;
        const QByteArray bytes = valid ? reply->readAll() : QByteArray();
        reply->deleteLater();
        callback(bytes);
        if (m_running && m_replies.isEmpty())
            finish();
    });
}

void TranslationUpdater::readManifest(const QByteArray &bytes)
{
    const auto manifest = QJsonDocument::fromJson(bytes).object();
    if (manifest.value(QStringLiteral("version")).toInt() != 1 ||
        !manifest.value(QStringLiteral("catalogs")).isObject())
        return;
    const auto catalogs = manifest.value(QStringLiteral("catalogs")).toObject();
    if (catalogs.size() > 64)
        return;
    const QRegularExpression locale(QStringLiteral("^[a-z]{2,3}(?:_[A-Za-z0-9]{2,8}){0,2}$"));
    const QRegularExpression digest(QStringLiteral("^[0-9a-f]{64}$"));
    qsizetype totalSize = 0;
    for (auto it = catalogs.begin(); it != catalogs.end(); ++it) {
        const auto entry = it.value().toObject();
        const QString hash = entry.value(QStringLiteral("sha256")).toString();
        const qint64 size = entry.value(QStringLiteral("size")).toInteger();
        if (!locale.match(it.key()).hasMatch() || !digest.match(hash).hasMatch() ||
            size <= 0 || size > maximumCatalogSize)
            continue;
        totalSize += size;
        if (totalSize > 16 * 1024 * 1024)
            break;
        const QString destination = QDir(m_cachePath).filePath(QStringLiteral("immich_%1.qm").arg(it.key()));
        QFile existing(destination);
        if (existing.open(QIODevice::ReadOnly) && existing.size() == size &&
            QCryptographicHash::hash(existing.readAll(), QCryptographicHash::Sha256).toHex() == hash.toLatin1())
            continue;
        const QString filename = QStringLiteral("immich_%1_%2.qm").arg(it.key(), hash);
        get(filename, size, [this, destination, hash, size](const QByteArray &catalog) {
            if (catalog.size() != size ||
                QCryptographicHash::hash(catalog, QCryptographicHash::Sha256).toHex() != hash.toLatin1())
                return;
            QTranslator check;
            if (!check.load(reinterpret_cast<const uchar *>(catalog.constData()), catalog.size()))
                return;
            if (!QDir().mkpath(m_cachePath))
                return;
            QSaveFile output(destination);
            if (output.open(QIODevice::WriteOnly) && output.write(catalog) == catalog.size() && output.commit())
                m_changed = true;
        });
    }
}

void TranslationUpdater::finish()
{
    if (!m_running)
        return;
    m_running = false;
    m_deadline.stop();
    const auto pending = m_replies;
    m_replies.clear();
    for (auto *reply : pending) {
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    emit finished(m_changed);
}

} // namespace Aurora
