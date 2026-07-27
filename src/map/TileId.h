#pragma once

#include <QHashFunctions>
#include <QString>

namespace kerkenez {

struct TileId
{
    int z = 0;
    int x = 0;
    int y = 0;

    QString key() const { return QStringLiteral("%1/%2/%3").arg(z).arg(x).arg(y); }
};

inline bool operator==(const TileId &a, const TileId &b)
{
    return a.z == b.z && a.x == b.x && a.y == b.y;
}

inline size_t qHash(const TileId &id, size_t seed = 0)
{
    return qHashMulti(seed, id.z, id.x, id.y);
}

} // namespace kerkenez
