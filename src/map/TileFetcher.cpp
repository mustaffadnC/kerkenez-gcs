#include "map/TileFetcher.h"

#include <QNetworkReply>
#include <QNetworkRequest>

#include "map/TileCache.h"

namespace kerkenez {

namespace {
// OSM requires a real identifying User-Agent; a generic one gets blocked.
const auto kUserAgent =
    QByteArrayLiteral("KerkenezGCS/0.3 (+https://github.com/conny0506/kerkenez-gcs)");
constexpr int kMaxQueued = 96;
} // namespace

TileFetcher::TileFetcher(TileCache *cache, QObject *parent)
    : QObject(parent)
    , m_cache(cache)
{
}

void TileFetcher::setUrlTemplate(const QString &urlTemplate)
{
    m_urlTemplate = urlTemplate;
    m_failed.clear();
}

void TileFetcher::setOffline(bool offline)
{
    m_offline = offline;
    if (offline)
        m_queue.clear();
    else
        m_failed.clear(); // give previously unreachable tiles another chance
}

void TileFetcher::request(const TileId &id)
{
    if (m_offline)
        return;
    const QString key = id.key();
    if (m_inFlight.contains(key) || m_failed.contains(key))
        return;
    if (m_cache->containsOnDisk(id))
        return;
    if (m_queue.contains(id))
        return;

    // The viewport changes faster than tiles arrive; drop the oldest requests
    // rather than growing an unbounded backlog of stale tiles.
    if (m_queue.size() >= kMaxQueued)
        m_queue.removeFirst();
    m_queue.append(id);
    pump();
}

void TileFetcher::cancelPending()
{
    m_queue.clear();
}

void TileFetcher::pump()
{
    while (!m_queue.isEmpty() && m_inFlight.size() < m_maxConcurrent)
        startRequest(m_queue.takeLast()); // newest first: they are on screen now
}

void TileFetcher::startRequest(const TileId &id)
{
    QString url = m_urlTemplate;
    url.replace(QLatin1String("{z}"), QString::number(id.z));
    url.replace(QLatin1String("{x}"), QString::number(id.x));
    url.replace(QLatin1String("{y}"), QString::number(id.y));

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("User-Agent", kUserAgent);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    const QString key = id.key();
    m_inFlight.insert(key);

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, id, key] {
        reply->deleteLater();
        m_inFlight.remove(key);

        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray payload = reply->readAll();
            QImage image;
            if (image.loadFromData(payload, "PNG")) {
                m_cache->insert(id, image, payload);
                emit tileReady(id);
            } else {
                m_failed.insert(key);
            }
        } else {
            m_failed.insert(key);
        }
        pump();
    });
}

} // namespace kerkenez
