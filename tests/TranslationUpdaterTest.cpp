#include "core/Localization.h"
#include "core/TranslationUpdater.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

class CatalogServer : public QTcpServer {
public:
    QHash<QByteArray, QByteArray> files;
    int catalogRequests = 0;

    CatalogServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto *socket = nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                    QByteArray request = socket->property("request").toByteArray() + socket->readAll();
                    socket->setProperty("request", request);
                    if (!request.contains("\r\n\r\n") || socket->property("answered").toBool())
                        return;
                    socket->setProperty("answered", true);
                    const QByteArray path = request.split(' ').value(1);
                    if (path.endsWith(".qm"))
                        ++catalogRequests;
                    const bool found = files.contains(path);
                    const QByteArray body = files.value(path);
                    socket->write(QByteArray("HTTP/1.1 ") + (found ? "200 OK" : "404 Not Found") +
                        "\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) +
                        "\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    }

    QUrl url() const { return QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(serverPort())); }

    void catalog(const QByteArray &data, const QString &language = QStringLiteral("de"))
    {
        const QString hash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
        const QJsonObject entry{{"sha256", hash}, {"size", data.size()}};
        files["/index.json"] = QJsonDocument(QJsonObject{{"version", 1},
            {"catalogs", QJsonObject{{language, entry}}}}).toJson();
        files[QStringLiteral("/immich_%1_%2.qm").arg(language, hash).toUtf8()] = data;
    }
};

class TranslationUpdaterTest : public QObject {
    Q_OBJECT

    QByteArray fixture() const
    {
        QFile file(QStringLiteral(":/fixtures/remote_de.qm"));
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return file.readAll();
    }

private slots:
    void downloadCacheAndBackup()
    {
        QTemporaryDir cache;
        QVERIFY(cache.isValid());
        CatalogServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        const auto data = fixture();
        QVERIFY(!data.isEmpty());
        server.catalog(data);
        Aurora::TranslationUpdater updater(nullptr, cache.path(), server.url());
        QSignalSpy finished(&updater, &Aurora::TranslationUpdater::finished);
        updater.refresh();
        QTRY_COMPARE(finished.size(), 1);
        QCOMPARE(finished.at(0).at(0).toBool(), true);
        Aurora::Localization localization(cache.path());
        localization.initialize(QStringLiteral("de"));
        QCOMPARE(QCoreApplication::translate("Aurora::Sidebar", "Library"), QStringLiteral("Remote library"));
        // Downloaded partial catalogs still use embedded translations for missing messages.
        QCOMPARE(QCoreApplication::translate("Aurora::AppearancePage", "Appearance"), QStringLiteral("Darstellung"));
        updater.refresh();
        QTRY_COMPARE(finished.size(), 2);
        QCOMPARE(finished.at(1).at(0).toBool(), false);
        QCOMPARE(server.catalogRequests, 1); // Matching checksum avoids downloading again.
        QFile replacement(QStringLiteral(":/i18n/immich_fr.qm"));
        QVERIFY(replacement.open(QIODevice::ReadOnly));
        server.catalog(replacement.readAll());
        updater.refresh();
        QTRY_COMPARE(finished.size(), 3);
        QCOMPARE(finished.at(2).at(0).toBool(), true);
        server.close();
        updater.refresh();
        QTRY_COMPARE(finished.size(), 4);
        QCOMPARE(finished.at(3).at(0).toBool(), false);
        localization.initialize(QStringLiteral("de"));
        QCOMPARE(QCoreApplication::translate("Aurora::Sidebar", "Library"), QStringLiteral("Photothèque"));
        QVERIFY(QFile::remove(cache.filePath(QStringLiteral("immich_de.qm"))));
        localization.initialize(QStringLiteral("de"));
        QCOMPARE(QCoreApplication::translate("Aurora::Sidebar", "Library"), QStringLiteral("Mediathek"));
    }

    void badDownloadsNeverReplaceCache_data()
    {
        QTest::addColumn<QByteArray>("failure");
        QTest::newRow("bad manifest") << QByteArray("manifest");
        QTest::newRow("checksum mismatch") << QByteArray("checksum");
        QTest::newRow("missing catalog") << QByteArray("404");
        QTest::newRow("invalid qm") << QByteArray("qm");
    }

    void badDownloadsNeverReplaceCache()
    {
        QFETCH(QByteArray, failure);
        QTemporaryDir cache;
        QVERIFY(cache.isValid());
        const QByteArray original = fixture();
        QFile saved(cache.filePath(QStringLiteral("immich_de.qm")));
        QVERIFY(saved.open(QIODevice::WriteOnly));
        QCOMPARE(saved.write(original), original.size());
        saved.close();
        CatalogServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        server.catalog(original + "changed");
        if (failure == "manifest")
            server.files["/index.json"] = "not JSON";
        else if (failure == "checksum") {
            for (auto it = server.files.begin(); it != server.files.end(); ++it)
                if (it.key().endsWith(".qm"))
                    it.value() = "tampered";
        } else if (failure == "404") {
            const auto keys = server.files.keys();
            for (const auto &key : keys)
                if (key.endsWith(".qm"))
                    server.files.remove(key);
        } else
            server.catalog(QByteArray("invalid catalog with a valid checksum"));
        Aurora::TranslationUpdater updater(nullptr, cache.path(), server.url());
        QSignalSpy finished(&updater, &Aurora::TranslationUpdater::finished);
        updater.refresh();
        QTRY_COMPARE(finished.size(), 1);
        QCOMPARE(finished.at(0).at(0).toBool(), false);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), original);
    }

    void discoverNewLanguageAndRejectUnsafeNames()
    {
        QTemporaryDir cache;
        CatalogServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        server.catalog(fixture(), QStringLiteral("pl"));
        Aurora::TranslationUpdater updater(nullptr, cache.path(), server.url());
        QSignalSpy finished(&updater, &Aurora::TranslationUpdater::finished);
        updater.refresh();
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(Aurora::Localization::availableLanguages(cache.path()).contains(QStringLiteral("pl")));
        Aurora::Localization localization(cache.path());
        localization.initialize(QStringLiteral("pl"));
        QCOMPARE(QCoreApplication::translate("Aurora::Sidebar", "Library"), QStringLiteral("Remote library"));
        QCOMPARE(QCoreApplication::translate("Aurora::LibraryPage", "%n item(s)", nullptr, 1), QStringLiteral("1 item"));
        server.catalog(fixture(), QStringLiteral("../escape"));
        updater.refresh();
        QTRY_COMPARE(finished.size(), 2);
        QCOMPARE(finished.at(1).at(0).toBool(), false);
        QCOMPARE(server.catalogRequests, 1);
    }
};

QTEST_GUILESS_MAIN(TranslationUpdaterTest)
#include "TranslationUpdaterTest.moc"
