#pragma once

namespace kerkenez::TileMath {

// Web Mercator (EPSG:3857) slippy-map conversions, 256 px tiles.
// Tile coordinates are returned as doubles so panning stays sub-tile smooth.

inline constexpr int kTileSize = 256;
inline constexpr int kMinZoom = 2;
inline constexpr int kMaxZoom = 19;

int tileCount(int zoom);

double lonToTileX(double lon, int zoom);
double latToTileY(double lat, int zoom);
double tileXToLon(double x, int zoom);
double tileYToLat(double y, int zoom);

// Ground resolution at a latitude, for scale bars.
double metersPerPixel(double lat, int zoom);

// Great-circle distance, used for trail thinning and the scale bar.
double haversineMeters(double lat1, double lon1, double lat2, double lon2);

// Wraps a tile column into [0, tileCount) so the map scrolls across the
// antimeridian; rows are clamped by the caller instead (no vertical wrap).
int wrapTileX(int x, int zoom);

} // namespace kerkenez::TileMath
