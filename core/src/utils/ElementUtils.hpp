#ifndef UTILS_ELEMENTUTILS_HPP_DEFINED
#define UTILS_ELEMENTUTILS_HPP_DEFINED

#include "entities/Element.hpp"
#include "formats/FormatTypes.hpp"
#include "index/StringTable.hpp"
#include "utils/CompatibilityUtils.hpp"

#include <algorithm>
#include <cstdint>

namespace utymap { namespace utils {

// Sets tags to element.
inline void setTags(utymap::index::StringTable& stringTable,
             utymap::entities::Element& element,
             const utymap::formats::Tags& tags)
{
    for (const auto& tag : tags) {
        std::uint32_t key = stringTable.getId(tag.key);
        std::uint32_t value = stringTable.getId(tag.value);
        element.tags.push_back(utymap::entities::Tag(key, value));
    }
    stringTable.flush();
    // NOTE: tags should be sorted to speed up mapcss styling
    std::sort(element.tags.begin(), element.tags.end());
}

// Convert format specific tags to entity ones.
inline std::vector<utymap::entities::Tag> convertTags(utymap::index::StringTable& stringTable, const utymap::formats::Tags& tags)
{
    std::vector<utymap::entities::Tag> convertedTags(tags.size());
    std::transform(tags.begin(), tags.end(), convertedTags.begin(), [&](const utymap::formats::Tag& tag) {
        return utymap::entities::Tag(stringTable.getId(tag.key), stringTable.getId(tag.value));
    });
    return std::move(convertedTags);
}

// Gets mesh name
inline std::string getMeshName(const std::string& prefix, const utymap::entities::Element& element) {
    return prefix + std::to_string(element.id);
}

}}

#endif // UTILS_ELEMENTUTILS_HPP_DEFINED
