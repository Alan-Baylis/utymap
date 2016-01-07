#ifndef INDEX_ELEMENTSTORE_HPP_DEFINED
#define INDEX_ELEMENTSTORE_HPP_DEFINED

#include "BoundingBox.hpp"
#include "GeoCoordinate.hpp"
#include "QuadKey.hpp"
#include "entities/Element.hpp"
#include "entities/ElementVisitor.hpp"
#include "formats/FormatTypes.hpp"
#include "mapcss/StyleProvider.hpp"

#include <memory>

namespace utymap { namespace index {

// Defines API to store elements.
class ElementStore
{
public:
    ElementStore(const utymap::mapcss::StyleProvider& styleProvider, utymap::index::StringTable& stringTable);

    virtual ~ElementStore();

    // Stores element in storage.
    bool store(const utymap::entities::Element& element);

protected:
    // Stores element in given quadkey.
    virtual void store(const utymap::entities::Element& element, const QuadKey& quadKey) = 0;

private:
    friend class ElementGeometryClipper;
    void storeInTileRange(const utymap::entities::Element& element,
                          const utymap::BoundingBox& elementBbox,
                          int levelOfDetails,
                          bool shoudClip);

    const utymap::mapcss::StyleProvider& styleProvider_;
    utymap::index::StringTable& stringTable_;
};

}}

#endif // INDEX_ELEMENTSTORE_HPP_DEFINED
