#pragma once

#include "core/Localization.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <functional>

class QNetworkReply;

namespace Aurora {

class TranslationUpdater final : public QObject {
    Q_OBJECT

public:
    explicit TranslationUpdater(QObject *parent = nullptr,
        const QString &cachePath = Localization::cacheDirectory(),
        const QUrl &baseUrl = repositoryUrl());
    ~TranslationUpdater() override;
    static QUrl repositoryUrl();
    void refresh();

signals:
    void finished(bool changed);

private:
    void get(const QString &file, qsizetype limit,
             std::function<void(const QByteArray &)> callback);
    void readManifest(const QByteArray &bytes);
    void finish();

    QNetworkAccessManager m_network;
    QTimer m_deadline;
    QSet<QNetworkReply *> m_replies;
    QString m_cachePath;
    QUrl m_baseUrl;
    bool m_running = false;
    bool m_changed = false;
};

} // namespace Aurora
