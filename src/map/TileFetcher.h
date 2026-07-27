#pragma once

#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QString>

#include "map/TileId.h"

namespace kerkenez {

class TileCache;

// Downloads missing tiles into the cache, honouring the OSM tile usage policy:
// identifying User-Agent, at most two concurrent requests, everything cached.
// In offline mode nothing is requested — the map draws whatever the cache has.
class TileFetcher : public QObject
{
    Q_OBJECT
public:
    explicit TileFetcher(TileCache *cache, QObject *parent = nullptr);

    void setUrlTemplate(const QString &urlTemplate);
    QString urlTemplate() const { return m_urlTemplate; }

    void setOffline(bool offline);
    bool isOffline() const { return m_offline; }

    // No-op when the tile is cached, already queued or known to be missing.
    void request(const TileId &id);
    void cancelPending();

signals:
    void tileReady(const kerkenez::TileId &id);

private:
    void pump();
    void startRequest(const TileId &id);

    QNetworkAccessManager m_network;
    TileCache *m_cache;
    QString m_urlTemplate = QStringLiteral("https://tile.openstreetmap.org/{z}/{x}/{y}.png");
    QList<TileId> m_queue;
    QSet<QString> m_inFlight;
    QSet<QString> m_failed;
    int m_maxConcurrent = 2;
    bool m_offline = false;
};

} // namespace kerkenez
