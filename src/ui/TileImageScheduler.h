#pragma once

#include "core/ImmichClient.h"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QSet>
#include <functional>

class QTimer;

namespace Aurora {

class MediaTile;

// Decides which thumbnails a photo grid asks for, and when.
// - The disk cache is checked as soon as a tile comes near the screen.
// - The server is only asked once a tile has stayed on screen for a moment,
//   or just off screen a little longer (prefetch once scrolling settles), so
//   tiles flown past in a fast scroll never queue a download. Once asked, a
//   download is kept even if the tile scrolls away, so it's cached for next time.
// - Tiles are handled nearest first.
class TileImageScheduler final : public QObject {
    Q_OBJECT

public:
    // rerun is called when the grid should run update() again.
    TileImageScheduler(ImmichClient *client, std::function<void()> rerun,
                       QObject *parent = nullptr);

    // Tiles near the viewport, nearest first; viewport is in the same coordinates
    // as the tiles' geometry.
    void update(const QList<MediaTile *> &tiles, const QRect &viewport);
    // The tile left the area near the viewport; the grid drops its image.
    void tileLeft(const QString &assetId, bool hadImage);
    // A thumbnail (or tile image) arrived, or failed for good.
    void finished(const QString &assetId);
    void clear();

private:
    bool dwelled(const QString &assetId, bool visible, qint64 now, qint64 *wait);

    ImmichClient *m_client;
    std::function<void()> m_rerun;
    QElapsedTimer m_clock;
    QTimer *m_dwellTimer;
    QHash<QString, qint64> m_nearSince;
    QHash<QString, qint64> m_visibleSince;
    QSet<QString> m_cacheChecked;
    QHash<QString, bool> m_fetching; // Queued downloads; true once queued as on screen.
    QHash<QString, int> m_tileImageChecked;
    QHash<QString, int> m_tileImageFetching;
};

} // namespace Aurora
