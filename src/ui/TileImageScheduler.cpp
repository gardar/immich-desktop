#include "ui/TileImageScheduler.h"

#include "ui/GridZoom.h"
#include "ui/widgets/MediaTile.h"

#include <QTimer>

namespace Aurora {
namespace {

// Long enough to skip tiles in a fast scroll, short enough not to notice.
constexpr qint64 kVisibleDwellMs = 250;
// Tiles just off screen are prefetched once scrolling has settled.
constexpr qint64 kNearDwellMs = 600;
constexpr int kMaxCacheChecksPerUpdate = 64;
constexpr int kMaxFetchesPerUpdate = 16;

} // namespace

TileImageScheduler::TileImageScheduler(ImmichClient *client, std::function<void()> rerun,
                                       QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_rerun(std::move(rerun))
    , m_dwellTimer(new QTimer(this))
{
    m_clock.start();
    m_dwellTimer->setSingleShot(true);
    connect(m_dwellTimer, &QTimer::timeout, this, [this] { m_rerun(); });
}

bool TileImageScheduler::dwelled(const QString &assetId, bool visible, qint64 now,
                                 qint64 *wait)
{
    auto waited = [now, wait](QHash<QString, qint64> &since, const QString &id, qint64 needed) {
        const qint64 start = since.value(id, -1);
        if (start < 0)
            since.insert(id, now);
        const qint64 elapsed = start < 0 ? 0 : now - start;
        if (elapsed >= needed)
            return true;
        *wait = qMin(*wait, needed - elapsed);
        return false;
    };
    if (!visible)
        m_visibleSince.remove(assetId);
    const bool nearLongEnough = waited(m_nearSince, assetId, kNearDwellMs);
    return (visible && waited(m_visibleSince, assetId, kVisibleDwellMs)) || nearLongEnough;
}

void TileImageScheduler::update(const QList<MediaTile *> &tiles, const QRect &viewport)
{
    const qint64 now = m_clock.elapsed();
    qint64 wait = kNearDwellMs + 1;
    int cacheChecks = 0;
    int fetches = 0;
    bool capped = false;
    // Fetches are sent after the loop, farthest first: later requests run first,
    // so the tile nearest the middle of the screen gets the highest priority.
    struct Fetch {
        QString assetId;
        int tileImageSize; // 0 for the thumbnail.
        bool visible;
    };
    QList<Fetch> toFetch;

    for (MediaTile *tile : tiles) {
        const QString assetId = tile->asset().id;
        const bool visible = viewport.intersects(tile->geometry());
        if (tile->hasThumbnail()) {
            // A sharper image for large tiles, through the same cache-then-server steps.
            const int wanted = GridZoom::tileImageDimension(tile->size(), tile->devicePixelRatioF());
            if (wanted <= tile->thumbnailDimension() ||
                wanted <= m_tileImageFetching.value(assetId))
                continue;
            if (m_tileImageChecked.value(assetId) < wanted) {
                if (cacheChecks >= kMaxCacheChecksPerUpdate) {
                    capped = true;
                    continue;
                }
                ++cacheChecks;
                m_tileImageChecked.insert(assetId, wanted);
                m_client->loadTileImage(assetId, wanted, ImmichClient::ImageSource::CacheOnly);
            }
            if (!dwelled(assetId, visible, now, &wait))
                continue;
            if (fetches >= kMaxFetchesPerUpdate) {
                capped = true;
                continue;
            }
            ++fetches;
            m_tileImageFetching.insert(assetId, wanted);
            toFetch.append({assetId, wanted, visible});
            continue;
        }

        // A prefetch already queued is moved up once its tile is on screen.
        if (tile->hasThumbnailError() || (m_fetching.contains(assetId) &&
                                          (m_fetching.value(assetId) || !visible)))
            continue;
        if (m_fetching.contains(assetId)) {
            m_fetching.insert(assetId, true);
            toFetch.append({assetId, 0, true});
            continue;
        }
        if (!m_cacheChecked.contains(assetId)) {
            if (cacheChecks >= kMaxCacheChecksPerUpdate) {
                capped = true;
                continue;
            }
            ++cacheChecks;
            m_cacheChecked.insert(assetId);
            m_client->loadThumbnail(assetId, ImmichClient::ImageSource::CacheOnly);
        }
        if (!dwelled(assetId, visible, now, &wait))
            continue;
        if (fetches >= kMaxFetchesPerUpdate) {
            capped = true;
            continue;
        }
        ++fetches;
        m_fetching.insert(assetId, visible);
        toFetch.append({assetId, 0, visible});
    }

    for (auto it = toFetch.crbegin(); it != toFetch.crend(); ++it) {
        const auto priority = it->visible ? ImmichClient::FetchPriority::OnScreen
                                          : ImmichClient::FetchPriority::Normal;
        if (it->tileImageSize > 0)
            m_client->loadTileImage(it->assetId, it->tileImageSize,
                                    ImmichClient::ImageSource::CacheOrServer, priority);
        else
            m_client->loadThumbnail(it->assetId, ImmichClient::ImageSource::CacheOrServer,
                                    priority);
    }

    // Continue shortly when this round hit its limit, or when the nearest
    // waiting tile has stayed long enough to fetch.
    if (capped)
        m_dwellTimer->start(60);
    else if (wait <= kNearDwellMs)
        m_dwellTimer->start(int(wait) + 10);
}

void TileImageScheduler::tileLeft(const QString &assetId, bool hadImage)
{
    m_nearSince.remove(assetId);
    m_visibleSince.remove(assetId);
    m_cacheChecked.remove(assetId);
    m_tileImageChecked.remove(assetId);
    if (hadImage) {
        m_fetching.remove(assetId);
        m_tileImageFetching.remove(assetId);
    }
}

void TileImageScheduler::finished(const QString &assetId)
{
    m_fetching.remove(assetId);
}

void TileImageScheduler::clear()
{
    m_dwellTimer->stop();
    m_nearSince.clear();
    m_visibleSince.clear();
    m_cacheChecked.clear();
    m_fetching.clear();
    m_tileImageChecked.clear();
    m_tileImageFetching.clear();
}

} // namespace Aurora
