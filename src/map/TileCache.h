#pragma once

#include <QCache>
#include <QImage>
#include <QString>

#include "map/TileId.h"

namespace kerkenez {

// Two-level tile store: an in-memory LRU in front of a <dir>/<z>/<x>/<y>.png
// tree. The disk level is what makes offline operation possible — once an
// area has been viewed, it renders with no network at all.
class TileCache
{
public:
    explicit TileCache(QString directory, int ramLimitMb = 64);

    bool get(const TileId &id, QImage *image) const;
    bool containsOnDisk(const TileId &id) const;

    // encodedPng is the untouched network payload; storing it avoids a
    // decode/encode round trip and keeps the bytes byte-identical to upstream.
    void insert(const TileId &id, const QImage &image, const QByteArray &encodedPng = {});

    QString directory() const { return m_directory; }
    qint64 diskUsageBytes() const;

private:
    QString filePath(const TileId &id) const;

    mutable QCache<QString, QImage> m_ram;
    QString m_directory;
};

} // namespace kerkenez
