#ifndef FORMATS_OSM_RELATIONPROCESSOR_HPP_DEFINED
#define FORMATS_OSM_RELATIONPROCESSOR_HPP_DEFINED

#include "formats/FormatTypes.hpp"
#include "formats/osm/OsmDataContext.hpp"
#include "utils/ElementUtils.hpp"

#include <cstdint>
#include <functional>
#include <memory>

namespace utymap { namespace formats {

class RelationProcessor
{
public:

    RelationProcessor(utymap::entities::Relation& relation,
                      const utymap::formats::RelationMembers& members,
                      utymap::formats::OsmDataContext& context,
                      std::function<void(utymap::entities::Relation&)> resolve)
    : relation_(relation), members_(members), context_(context), resolve_(resolve)
    {
    }

    void process()
    {
        utymap::utils::visitRelationMembers(context_, members_, *this);
    }

    void visit(OsmDataContext::NodeMapType::const_iterator node)
    {
        if (!has(node->first))
            relation_.elements.push_back(node->second);
    }

    void visit(OsmDataContext::WayMapType::const_iterator way)
    {
        if (!has(way->first))
            relation_.elements.push_back(way->second);
    }

    void visit(OsmDataContext::AreaMapType::const_iterator area)
    {
        if (!has(area->first))
            relation_.elements.push_back(area->second);
    }

    void visit(OsmDataContext::RelationMapType::const_iterator rel, const std::string& role)
    {
        if (has(rel->first))
            return;

        resolve_(*rel->second);
        relation_.elements.push_back(rel->second);
    }

private:

    bool has(std::uint64_t id)
    {
        auto it = std::find_if(relation_.elements.begin(), relation_.elements.end(),
            [&id](const std::shared_ptr<utymap::entities::Element> e) {
            return e->id == id;
        });

        return it != relation_.elements.end();
    }

    utymap::entities::Relation& relation_;
    const utymap::formats::RelationMembers& members_;
    utymap::formats::OsmDataContext& context_;
    std::function<void(utymap::entities::Relation&)> resolve_;
};

}}

#endif // FORMATS_OSM_RELATIONPROCESSOR_HPP_DEFINED
