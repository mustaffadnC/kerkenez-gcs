#include "map/TileMath.h"

#include <cmath>

namespace kerkenez::TileMath {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthCircumference = 40075016.686; // metres at the equator
constexpr double kEarthRadius = 6371008.8;           // mean radius, metres
} // namespace

int tileCount(int zoom)
{
    return 1 << zoom;
}

double lonToTileX(double lon, int zoom)
{
    return (lon + 180.0) / 360.0 * tileCount(zoom);
}

double latToTileY(double lat, int zoom)
{
    // Mercator diverges at the poles. Clamp just inside the ±85.05112877980°
    // limit — clamping *to* it lands a hair outside in floating point and
    // yields a negative row index.
    const double clamped = std::fmax(-85.05112877, std::fmin(85.05112877, lat));
    const double rad = clamped * kPi / 180.0;
    return (1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / kPi) / 2.0 * tileCount(zoom);
}

double tileXToLon(double x, int zoom)
{
    return x / tileCount(zoom) * 360.0 - 180.0;
}

double tileYToLat(double y, int zoom)
{
    const double n = kPi - 2.0 * kPi * y / tileCount(zoom);
    return 180.0 / kPi * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}

double metersPerPixel(double lat, int zoom)
{
    return kEarthCircumference * std::cos(lat * kPi / 180.0)
        / (tileCount(zoom) * double(kTileSize));
}

double haversineMeters(double lat1, double lon1, double lat2, double lon2)
{
    const double p1 = lat1 * kPi / 180.0;
    const double p2 = lat2 * kPi / 180.0;
    const double dp = (lat2 - lat1) * kPi / 180.0;
    const double dl = (lon2 - lon1) * kPi / 180.0;
    const double a = std::sin(dp / 2) * std::sin(dp / 2)
        + std::cos(p1) * std::cos(p2) * std::sin(dl / 2) * std::sin(dl / 2);
    return 2 * kEarthRadius * std::atan2(std::sqrt(a), std::sqrt(1 - a));
}

int wrapTileX(int x, int zoom)
{
    const int n = tileCount(zoom);
    return ((x % n) + n) % n;
}

} // namespace kerkenez::TileMath
