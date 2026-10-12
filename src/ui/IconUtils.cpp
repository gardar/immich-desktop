#include "ui/IconUtils.h"

#include <QFile>
#include <QApplication>
#include <QPushButton>
#include <QStyle>
#include <QPainter>
#include <QSvgRenderer>

namespace Aurora {
namespace {
QString activeIconPack = QStringLiteral("lucide");
bool minimalistUi = false;

void refreshActionButton(QPushButton *button)
{
    const QString label = button->property("actionLabel").toString();
    button->setText(minimalistUi ? QString() : label);
    button->setProperty("iconOnly", minimalistUi || label.isEmpty());
    if (minimalistUi || label.isEmpty()) {
        button->setMinimumWidth(40);
        button->setMaximumWidth(40);
    } else {
        button->setMaximumWidth(button->property("actionMaximumWidth").toInt());
        button->setMinimumWidth(button->property("actionMinimumWidth").toInt());
    }
    if (!label.isEmpty()) {
        button->setToolTip(label);
        button->setAccessibleName(label);
    }
    button->ensurePolished();
    button->style()->unpolish(button);
    button->style()->polish(button);
    const auto color = button->property("primary").toBool() ? QColor(Qt::white)
        : button->palette().color(QPalette::ButtonText);
    button->setIcon(QIcon(renderSvgIcon(button->property("actionIcon").toString(), color,
        QSize(20, 20), button->devicePixelRatioF())));
    button->setIconSize(QSize(20, 20));
    button->updateGeometry();
}
} // namespace

void setIconAppearance(const QString &pack, bool minimalist)
{
    activeIconPack = pack;
    minimalistUi = minimalist;
    for (auto *widget : QApplication::allWidgets()) {
        if (auto *button = qobject_cast<QPushButton *>(widget); button && button->property("actionIcon").isValid())
            refreshActionButton(button);
        widget->update();
    }
}

void configureActionButton(QPushButton *button, const QString &iconResource)
{
    if (!button->property("actionIcon").isValid()) {
        button->setProperty("actionMinimumWidth", button->minimumWidth());
        button->setProperty("actionMaximumWidth", button->maximumWidth());
    }
    button->setProperty("actionLabel", button->text());
    button->setProperty("actionIcon", iconResource);
    refreshActionButton(button);
}

void setActionButtonLabel(QPushButton *button, const QString &label)
{
    button->setProperty("actionLabel", label);
    refreshActionButton(button);
}

QPixmap renderSvgIcon(const QString &resourcePath, const QColor &color,
                      const QSize &size, qreal devicePixelRatio)
{
    QString resolved = resourcePath;
    if (activeIconPack != QStringLiteral("lucide") && resourcePath.startsWith(QStringLiteral(":/icons/"))) {
        const QString alternative = QStringLiteral(":/icons/%1/%2").arg(activeIconPack, resourcePath.mid(8));
        if (QFile::exists(alternative))
            resolved = alternative;
    }
    QFile file(resolved);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QByteArray source = file.readAll();
    source.replace("currentColor", color.name(QColor::HexRgb).toUtf8());
    QSvgRenderer renderer(source);
    if (!renderer.isValid())
        return {};

    const QSize pixelSize = (size * devicePixelRatio).expandedTo(QSize(1, 1));
    QPixmap pixmap(pixelSize);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(devicePixelRatio);

    QPainter painter(&pixmap);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    return pixmap;
}

} // namespace Aurora
