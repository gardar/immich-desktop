#include "ui/widgets/VideoPlayerDialog.h"
#include "ui/IconUtils.h"
#include "ui/widgets/ShareLinkDialog.h"

#include "ui/widgets/VideoPlayerWidget.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace Aurora {

VideoPlayerDialog::VideoPlayerDialog(ImmichClient *client, const ImmichAsset &asset,
                                     QWidget *parent)
    : QDialog(parent)
    , m_asset(asset)
    , m_player(new VideoPlayerWidget(client, this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowTitle(asset.fileName.isEmpty() ? tr("Video") : asset.fileName);
    resize(1000, 700);

    auto *downloadButton = new QPushButton(tr("Download"), this);
    auto *trashButton = new QPushButton(tr("Trash"), this);
    auto *deleteButton = new QPushButton(tr("Delete"), this);
    auto *shareButton = new QPushButton(tr("Share link"), this);
    configureActionButton(downloadButton, QStringLiteral(":/icons/download.svg"));
    configureActionButton(trashButton, QStringLiteral(":/icons/trash.svg"));
    configureActionButton(deleteButton, QStringLiteral(":/icons/x.svg"));
    configureActionButton(shareButton, QStringLiteral(":/icons/external-link.svg"));
    downloadButton->setToolTip(tr("Download original file"));
    trashButton->setToolTip(tr("Move to trash"));
    deleteButton->setToolTip(tr("Delete permanently"));

    auto *actions = new QHBoxLayout;
    actions->setSpacing(10);
    actions->addStretch(1);
    actions->addWidget(downloadButton);
    actions->addWidget(shareButton);
    actions->addWidget(trashButton);
    actions->addWidget(deleteButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addWidget(m_player, 1);
    layout->addLayout(actions);

    connect(downloadButton, &QPushButton::clicked, this, [this] {
        emit downloadRequested(m_asset);
    });
    connect(shareButton, &QPushButton::clicked, this, [this, client] {
        (new ShareLinkDialog(client, m_asset, this))->show();
    });
    connect(trashButton, &QPushButton::clicked, this, [this] {
        emit trashRequested(m_asset);
    });
    connect(deleteButton, &QPushButton::clicked, this, [this] {
        emit deleteRequested(m_asset);
    });

    m_player->setAsset(asset, true);
}

} // namespace Aurora
