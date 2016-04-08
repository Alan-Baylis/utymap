#ifndef BOUNDINGBOX_HPP_DEFINED
#define BOUNDINGBOX_HPP_DEFINED

#include "GeoCoordinate.hpp"

namespace utymap {

// Represents geo bounding box
struct BoundingBox
{
    // Point with minimal latitude and longitude.
    GeoCoordinate minPoint;
    // Point with maximum latitude and longitude.
    GeoCoordinate maxPoint;

    BoundingBox() : 
        BoundingBox(GeoCoordinate(90, 180), GeoCoordinate(-90, -180))
    {
    }

    BoundingBox(const GeoCoordinate& minPoint, const GeoCoordinate& maxPoint) :
        minPoint(minPoint), maxPoint(maxPoint) 
    {
    }

    BoundingBox& operator+=(const BoundingBox& rhs)
    {
        expand(rhs);
        return *this;
    }

    inline bool isValid() const 
    {
        // TODO possible that minLon > maxLon at some locations
        return minPoint.latitude <= maxPoint.latitude &&
               minPoint.longitude <= maxPoint.longitude;
    }

    // Expands bounding box using another bounding box.
    inline void expand(const BoundingBox& rhs)
    {
        minPoint.latitude = minPoint.latitude < rhs.minPoint.latitude ? minPoint.latitude : rhs.minPoint.latitude;
        minPoint.longitude = minPoint.longitude < rhs.minPoint.longitude ? minPoint.longitude : rhs.minPoint.longitude;

        maxPoint.latitude = maxPoint.latitude > rhs.maxPoint.latitude ? maxPoint.latitude : rhs.maxPoint.latitude;
        maxPoint.longitude = maxPoint.longitude > rhs.maxPoint.longitude ? maxPoint.longitude : rhs.maxPoint.longitude;
    }

    // Expands bounding box using given coordinate.
    inline void expand(const GeoCoordinate& c)
    {
        minPoint = GeoCoordinate(
            minPoint.latitude < c.latitude ? minPoint.latitude : c.latitude,
            minPoint.longitude < c.longitude ? minPoint.longitude : c.longitude);

        maxPoint = GeoCoordinate(
            maxPoint.latitude > c.latitude ? maxPoint.latitude : c.latitude,
            maxPoint.longitude > c.longitude ? maxPoint.longitude : c.longitude);
    }

    // Expands bounging box from collection of geo data.
    template<typename ForwardIterator>
    inline void expand(ForwardIterator begin, ForwardIterator end)
    {
        for (; begin != end; ++begin)
            expand(*begin);
    }

    // Checks whether given bounding box inside the current one.
    inline bool contains(const BoundingBox& bbox) const
    {
        return contains(bbox.minPoint) && contains(bbox.maxPoint);
    }

    // Checks whether given coordinate inside the bounding box.
    inline bool contains(const GeoCoordinate& coordinate) const
    {
        return coordinate.latitude > minPoint.latitude && coordinate.longitude > minPoint.longitude &&
               coordinate.latitude < maxPoint.latitude && coordinate.longitude < maxPoint.longitude;
    }

    // Checks whether given bounding box intersects the current one.
    inline bool intersects(const BoundingBox& rhs) const
    {
        double minLat = minPoint.latitude < rhs.minPoint.latitude ? rhs.minPoint.latitude : minPoint.latitude;
        double minLon = minPoint.longitude < rhs.minPoint.longitude ? rhs.minPoint.longitude : minPoint.longitude;
        double maxLat = maxPoint.latitude > rhs.maxPoint.latitude ? rhs.maxPoint.latitude : maxPoint.latitude;
        double maxLon = maxPoint.longitude > rhs.maxPoint.longitude ? rhs.maxPoint.longitude : maxPoint.longitude;

        return minLat <= maxLat && minLon <= maxLon;
    }

    // Returns center of bounding box.
    inline GeoCoordinate center() const
    {
        return GeoCoordinate(
            (maxPoint.latitude - minPoint.latitude) / 2,
            (maxPoint.longitude - minPoint.longitude) / 2);
    }
};

}
#endif // BOUNDINGBOX_HPP_DEFINED
