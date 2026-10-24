#pragma once

#include <QLocale>
#include <QStringList>
#include <QTranslator>

namespace Aurora {

// Owns installed translators for the entire application lifetime. Initialize before
// constructing widgets, or before rebuilding the UI after a language change.
class Localization final {
public:
    void initialize(const QString &language,
                    const QStringList &systemLanguages = QLocale::system().uiLanguages());
    static QStringList availableLanguages();
    QString language() const { return m_language; }

private:
    QString m_language;
    QTranslator m_application;
    QTranslator m_qt;
};

} // namespace Aurora
