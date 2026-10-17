#pragma once

#include "core/ImmichClient.h"

#include <QDialog>

namespace Aurora {

class VideoPlayerWidget;

class VideoPlayerDialog final : public QDialog {
    Q_OBJECT

public:
    explicit VideoPlayerDialog(ImmichClient *client, const ImmichAsset &asset,
                               QWidget *parent = nullptr);

signals:
    void downloadRequested(const Aurora::ImmichAsset &asset);
    void trashRequested(const Aurora::ImmichAsset &asset);
    void deleteRequested(const Aurora::ImmichAsset &asset);

private:
    ImmichAsset m_asset;
    VideoPlayerWidget *m_player;
};

} // namespace Aurora
