#pragma once

#include <QSize>
#include <QtGlobal>
#include <QtMath>

#include <cmath>

namespace Aurora::GridZoom {

// Target row height (or cell size in the compact grid), in logical pixels.
constexpr int kMinimum = 96;
constexpr int kMaximum = 720;
constexpr int kDefault = 180;

inline int clamp(int size)
{
    return qBound(kMinimum, size, kMaximum);
}

inline int step(int size, int steps)
{
    return clamp(qRound(size * std::pow(1.15, steps)));
}

// Justified rows may shrink or grow around the target to fill the width.
inline qreal minRowHeight(int size)
{
    return size * 0.75;
}

inline qreal maxRowHeight(int size)
{
    return size * 1.15;
}

// Image size a tile needs, in device pixels rounded up to 256px steps, or 0 while
// the server's 256px thumbnail is still sharp enough (up to 1.5x upscaling).
inline int tileImageDimension(const QSize &tileSize, qreal devicePixelRatio)
{
    const int needed = qCeil(qMax(tileSize.width(), tileSize.height()) * devicePixelRatio);
    if (needed <= 384)
        return 0;
    return qMin(1536, (needed + 255) / 256 * 256);
}

} // namespace Aurora::GridZoom
