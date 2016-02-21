#ifndef FORMATS_OSM_MULTIPOLYGONPROCESSOR_HPP_DEFINED
#define FORMATS_OSM_MULTIPOLYGONPROCESSOR_HPP_DEFINED

#include "GeoCoordinate.hpp"
#include "entities/Element.hpp"
#include "formats/FormatTypes.hpp"
#include "index/StringTable.hpp"

#include <unordered_map>

namespace utymap { namespace formats {

// Builds relation from multipolygon (see http://wiki.openstreetmap.org/wiki/Talk:Relation:multipolygon)
struct MultipolygonProcessor
{
private:

    // Helper class which provides the way to handle coordinate sequences.
    struct CoordinateSequence;
    typedef std::vector<std::shared_ptr<MultipolygonProcessor::CoordinateSequence>> CoordinateSequences;

public:
    MultipolygonProcessor(std::uint64_t id,
        utymap::formats::RelationMembers& members,
        utymap::formats::Tags& tags,
        utymap::index::StringTable& stringTable,
        std::unordered_map<std::uint64_t, std::shared_ptr<utymap::entities::Area>>& areaMap,
        std::unordered_map<std::uint64_t, std::shared_ptr<utymap::entities::Way>>& wayMap);

    // Builds relation from multipolygon relation.
    utymap::entities::Relation process();

private:

    utymap::formats::Tags restoreTags(const std::vector<utymap::entities::Tag>& tags) const;

    std::vector<utymap::entities::Tag> convertTags(const Tags& tags) const;

    void simpleCase(utymap::entities::Relation& relation,
                    const CoordinateSequences& sequences,
                    const std::vector<int>& outerIndecies,
                    const std::vector<int>& innerIndecies);

    void complexCase(utymap::entities::Relation& relation, CoordinateSequences& sequences);

    CoordinateSequences createRings(CoordinateSequences& sequences);

    void fillRelation(utymap::entities::Relation& relation, CoordinateSequences& rings);

    std::vector<utymap::entities::Tag> getTags(const CoordinateSequence& outer) const;

    std::uint64_t id_;
    utymap::formats::RelationMembers& members_;
    utymap::formats::Tags& tags_;
    utymap::index::StringTable& stringTable_;
    std::unordered_map<std::uint64_t, std::shared_ptr<utymap::entities::Area>>& areaMap_;
    std::unordered_map<std::uint64_t, std::shared_ptr<utymap::entities::Way>>& wayMap_;
};
}}

#endif // FORMATS_OSM_MULTIPOLYGONPROCESSOR_HPP_DEFINED
