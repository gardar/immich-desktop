#pragma once

#include "core/ImmichTypes.h"
#include <QDialog>
#include <QUuid>

class QLineEdit;
class QLabel;
class QPushButton;

namespace Aurora {
class ImmichClient;

class ShareLinkDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ShareLinkDialog(ImmichClient *client, const ImmichAsset &asset,
                             QWidget *parent = nullptr);
private:
    ImmichClient *m_client;
    ImmichAsset m_asset;
    QUuid m_requestId;
    QLineEdit *m_link;
    QLabel *m_status;
    QPushButton *m_create;
    QPushButton *m_copy;
};
} // namespace Aurora
