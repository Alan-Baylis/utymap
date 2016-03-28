#ifndef INDEX_ELEMENTSTORE_HPP_DEFINED
#define INDEX_ELEMENTSTORE_HPP_DEFINED

#include "BoundingBox.hpp"
#include "QuadKey.hpp"
#include "entities/Element.hpp"
#include "entities/ElementVisitor.hpp"
#include "index/LodRange.hpp"
#include "formats/FormatTypes.hpp"
#include "mapcss/StyleProvider.hpp"

#include <cstdint>
#include <memory>

namespace utymap { namespace index {

// Defines API to store elements.
class ElementStore
{
public:
    ElementStore(utymap::index::StringTable& stringTable);

    virtual ~ElementStore();

    // Searches for elements for given quadKey
    virtual void search(const utymap::QuadKey& quadKey,
                        const utymap::mapcss::StyleProvider& styleProvider,
                        utymap::entities::ElementVisitor& visitor) = 0;

    // Checks whether there is data for given quadkey.
    virtual bool hasData(const utymap::QuadKey& quadKey) const = 0;

    // Stores element in storage in all affected tiles at given level of details range.
    bool store(const utymap::entities::Element& element, 
               const utymap::index::LodRange& range,
               const utymap::mapcss::StyleProvider& styleProvider);

    // Stores element in storage only in given quadkey.
    bool store(const utymap::entities::Element& element,
               const utymap::QuadKey& quadKey,
               const utymap::mapcss::StyleProvider& styleProvider);

protected:
    // Stores element in given quadkey.
    virtual void storeImpl(const utymap::entities::Element& element, const utymap::QuadKey& quadKey) = 0;

private:
    class ElementGeometryVisitor;
    class BoundingBoxVisitor;

    void storeInTileRange(const utymap::entities::Element& element,
                          const utymap::BoundingBox& elementBbox,
                          int levelOfDetails,
                          bool shouldClip);

    std::uint32_t clipKeyId_, skipKeyId_;
};

}}

#endif // INDEX_ELEMENTSTORE_HPP_DEFINED
