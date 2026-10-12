#pragma once

#include <QColor>
#include <QPixmap>
#include <QSize>
#include <QString>

class QPushButton;

namespace Aurora {

void setIconAppearance(const QString &pack, bool minimalist);
void configureActionButton(QPushButton *button, const QString &iconResource);
void setActionButtonLabel(QPushButton *button, const QString &label);

QPixmap renderSvgIcon(const QString &resourcePath, const QColor &color,
                      const QSize &size, qreal devicePixelRatio = 1.0);

} // namespace Aurora
