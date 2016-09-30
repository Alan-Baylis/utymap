#ifndef INDEX_STYLEPROVIDER_HPP_DEFINED
#define INDEX_STYLEPROVIDER_HPP_DEFINED

#include "index/StringTable.hpp"
#include "entities/Element.hpp"
#include "mapcss/ColorGradient.hpp"
#include "mapcss/StyleSheet.hpp"
#include "mapcss/Style.hpp"

#include <string>
#include <memory>

namespace utymap { namespace mapcss {

/// This class responsible for filtering elements.
class StyleProvider final
{
public:

    StyleProvider(const StyleSheet&, 
                  utymap::index::StringTable&);

    ~StyleProvider();
    StyleProvider(StyleProvider&&);

    StyleProvider(const StyleProvider&) = delete;
    StyleProvider&operator=(const StyleProvider&) = delete;
    StyleProvider&operator=(StyleProvider&&) = delete;

    /// Checks whether style is defined for the element.
    bool hasStyle(const utymap::entities::Element&, int levelOfDetails) const;

    /// Returns style for given element at given level of details.
    utymap::mapcss::Style forElement(const utymap::entities::Element&, int levelOfDetails) const;

    /// Returns style for canvas at given level of details.
    utymap::mapcss::Style forCanvas(int levelOfDetails) const;

    /// Returns color gradient for given key.
    const ColorGradient& getGradient(const std::string& key) const;

    /// Returns texture group from given atlas using key.
    const TextureGroup& getTexture(const std::string& atlas, const std::string& key) const;

private:
    class StyleProviderImpl;
    std::unique_ptr<StyleProviderImpl> pimpl_;
};

}}

#endif // INDEX_STYLEPROVIDER_HPP_DEFINED
