#pragma once

#include <QLocale>
#include <QStringList>
#include <QTranslator>
#include <memory>

namespace Aurora {

// Owns installed translators for the entire application lifetime. Initialize before
// constructing widgets, or before rebuilding the UI after a language change.
class Localization final {
public:
    static QString cacheDirectory();
    explicit Localization(const QString &cachePath = cacheDirectory());
    void initialize(const QString &language,
                    const QStringList &systemLanguages = QLocale::system().uiLanguages());
    static QStringList availableLanguages(const QString &cachePath = cacheDirectory());
    QString language() const { return m_language; }

private:
    QString m_language;
    QTranslator m_application;
    QByteArray m_downloadedData;
    std::unique_ptr<QTranslator> m_downloaded;
    QTranslator m_qt;
    QString m_cachePath;
};

} // namespace Aurora
