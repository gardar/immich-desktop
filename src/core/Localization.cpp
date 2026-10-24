#include "core/Localization.h"

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>

namespace Aurora {

QStringList Localization::availableLanguages()
{
    QStringList languages{QStringLiteral("en")};
    const auto catalogs = QDir(QStringLiteral(":/i18n")).entryList(
        {QStringLiteral("immich_*.qm")}, QDir::Files, QDir::Name);
    for (const auto &catalog : catalogs) {
        const QString language = catalog.mid(7, catalog.size() - 10);
        if (!languages.contains(language))
            languages.append(language);
    }
    return languages;
}

void Localization::initialize(const QString &language, const QStringList &systemLanguages)
{
    QCoreApplication::removeTranslator(&m_application);
    QCoreApplication::removeTranslator(&m_qt);
    const auto available = availableLanguages();
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
    if (m_application.load(QStringLiteral(":/i18n/immich_%1.qm").arg(m_language)))
        QCoreApplication::installTranslator(&m_application);
}

} // namespace Aurora
