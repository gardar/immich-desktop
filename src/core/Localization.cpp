#include "core/Localization.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLibraryInfo>
#include <QRegularExpression>
#include <QStandardPaths>

namespace Aurora {

QString Localization::cacheDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/translations");
}

Localization::Localization(const QString &cachePath)
    : m_cachePath(cachePath)
{
}

QStringList Localization::availableLanguages(const QString &cachePath)
{
    QStringList languages{QStringLiteral("en")};
    const QRegularExpression name(QStringLiteral("^immich_([a-z]{2,3}(?:_[A-Za-z0-9]{2,8}){0,2})\\.qm$"));
    for (const auto &directory : {QStringLiteral(":/i18n"), cachePath}) {
        const auto catalogs = QDir(directory).entryList(
            {QStringLiteral("immich_*.qm")}, QDir::Files, QDir::Name);
        for (const auto &catalog : catalogs) {
            const auto match = name.match(catalog);
            QTranslator check;
            if (match.hasMatch() && check.load(QDir(directory).filePath(catalog)) &&
                !languages.contains(match.captured(1)))
                languages.append(match.captured(1));
        }
    }
    return languages;
}

void Localization::initialize(const QString &language, const QStringList &systemLanguages)
{
    QCoreApplication::removeTranslator(&m_application);
    if (m_downloaded)
        QCoreApplication::removeTranslator(m_downloaded.get());
    m_downloaded.reset();
    m_downloadedData.clear();
    QCoreApplication::removeTranslator(&m_qt);
    const auto available = availableLanguages(m_cachePath);
    const bool explicitLanguage = available.contains(language);
    const auto candidates = explicitLanguage ? QStringList{language} : systemLanguages;
    m_language = QStringLiteral("en");
    QLocale locale = explicitLanguage ? QLocale(language) : QLocale::system();
    for (QString candidate : candidates) {
        candidate.replace(QLatin1Char('-'), QLatin1Char('_'));
        // Try region/script-specific catalogs before the base language.
        while (!candidate.isEmpty()) {
            if (available.contains(candidate)) {
                m_language = candidate;
                if (explicitLanguage)
                    locale = QLocale(candidate);
                break;
            }
            const int separator = candidate.lastIndexOf(QLatin1Char('_'));
            candidate = separator < 0 ? QString() : candidate.left(separator);
        }
        if (!candidate.isEmpty())
            break;
    }
    QLocale::setDefault(locale);
    if (m_qt.load(QLocale(m_language), QStringLiteral("qtbase"), QStringLiteral("_"),
                  QStringLiteral(":/i18n")) ||
        m_qt.load(QLocale(m_language), QStringLiteral("qtbase"), QStringLiteral("_"),
                  QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(&m_qt);
    if (m_application.load(QStringLiteral(":/i18n/immich_%1.qm").arg(m_language)) ||
        m_application.load(QStringLiteral(":/i18n/immich_en.qm")))
        QCoreApplication::installTranslator(&m_application);
    // Install last so downloaded messages win; missing entries use the embedded backup.
    // Use owned bytes, not a memory-mapped file: Windows must be able to replace
    // the cached catalog even while its previous version is installed.
    QFile downloaded(QDir(m_cachePath).filePath(QStringLiteral("immich_%1.qm").arg(m_language)));
    if (downloaded.open(QIODevice::ReadOnly) && downloaded.size() <= 2 * 1024 * 1024) {
        m_downloadedData = downloaded.readAll();
        m_downloaded = std::make_unique<QTranslator>();
        if (m_downloaded->load(reinterpret_cast<const uchar *>(m_downloadedData.constData()),
                               m_downloadedData.size()))
            QCoreApplication::installTranslator(m_downloaded.get());
    }
}

} // namespace Aurora
