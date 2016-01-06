#ifndef INDEX_STYLEPROVIDER_HPP_DEFINED
#define INDEX_STYLEPROVIDER_HPP_DEFINED

#include "index/StringTable.hpp"
#include "entities/Element.hpp"
#include "mapcss/Stylesheet.hpp"
#include "mapcss/Style.hpp"

#include <memory>

namespace utymap { namespace mapcss {

// This class responsible for filtering elements.
class StyleProvider
{
public:

    StyleProvider(const utymap::mapcss::StyleSheet&, utymap::index::StringTable&);

    ~StyleProvider();

    // Creates style for given element at given level of details.
    utymap::mapcss::Style forElement(const utymap::entities::Element&, int levelOfDetails) const;

    // Creates style for canvas at given level of details.
    utymap::mapcss::Style forCanvas(int levelOfDetails) const;

private:
    class StyleProviderImpl;
    std::unique_ptr<StyleProviderImpl> pimpl_;
};

}}

#endif // INDEX_STYLEPROVIDER_HPP_DEFINED
