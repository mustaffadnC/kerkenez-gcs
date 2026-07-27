#include "map/TileCache.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace kerkenez {

TileCache::TileCache(QString directory, int ramLimitMb)
    : m_ram(ramLimitMb * 1024) // cost is tracked in KiB
    , m_directory(std::move(directory))
{
    QDir().mkpath(m_directory);
}

QString TileCache::filePath(const TileId &id) const
{
    return QStringLiteral("%1/%2/%3/%4.png").arg(m_directory).arg(id.z).arg(id.x).arg(id.y);
}

bool TileCache::get(const TileId &id, QImage *image) const
{
    const QString key = id.key();
    if (const QImage *cached = m_ram.object(key)) {
        if (image)
            *image = *cached;
        return true;
    }

    QImage fromDisk;
    if (!fromDisk.load(filePath(id), "PNG"))
        return false;

    m_ram.insert(key, new QImage(fromDisk), qMax(1, int(fromDisk.sizeInBytes() / 1024)));
    if (image)
        *image = fromDisk;
    return true;
}

bool TileCache::containsOnDisk(const TileId &id) const
{
    return QFileInfo::exists(filePath(id));
}

void TileCache::insert(const TileId &id, const QImage &image, const QByteArray &encodedPng)
{
    if (image.isNull())
        return;

    m_ram.insert(id.key(), new QImage(image), qMax(1, int(image.sizeInBytes() / 1024)));

    const QString path = filePath(id);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    if (encodedPng.isEmpty())
        image.save(&file, "PNG");
    else
        file.write(encodedPng);
    file.commit();
}

qint64 TileCache::diskUsageBytes() const
{
    qint64 total = 0;
    QDirIterator it(m_directory, {QStringLiteral("*.png")}, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

} // namespace kerkenez
