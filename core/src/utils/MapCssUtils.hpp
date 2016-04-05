#ifndef UTILS_MAPCSSUTILS_HPP_DEFINED
#define UTILS_MAPCSSUTILS_HPP_DEFINED

#include "entities/Element.hpp"
#include "index/StringTable.hpp"
#include "mapcss/Style.hpp"
#include "utils/GeoUtils.hpp"

#include <string>

namespace utymap { namespace utils {

    // Checks whether key exists.
    inline bool hasKey(const std::string& key,
                       utymap::index::StringTable& stringTable,
                       const utymap::mapcss::Style& style)
    {
        return style.has(stringTable.getId(key));
    }

    // Gets raw declaration value as string.
    inline std::shared_ptr<std::string> getString(const std::string& key,
                                                  utymap::index::StringTable& stringTable,
                                                  const utymap::mapcss::Style& style,
                                                  const std::string& defaultValue = "")
    {
        uint32_t keyId = stringTable.getId(key);
        return style.has(keyId)
            ? style.get(keyId)->value()
            : std::shared_ptr<std::string>(new std::string(defaultValue));
    }

    // Gets declaration value as double. Evaluates if necessary.
    inline double getDouble(const std::string& key,
                            const std::vector<utymap::entities::Tag>& tags,
                            utymap::index::StringTable& stringTable,
                            const utymap::mapcss::Style& style)
    {
        uint32_t keyId = stringTable.getId(key);
        return style.get(keyId)->evaluate(tags, stringTable);
    }

    // Gets declaration value as double without evaluation.
    inline double getDouble(const std::string& key,
                            utymap::index::StringTable& stringTable,
                            const utymap::mapcss::Style& style,
                            double defaultValue = 0)
    {
        uint32_t keyId = stringTable.getId(key);
        return style.has(keyId)
            ? std::stod(*style.get(keyId)->value())
            : defaultValue;
    }

    // Gets width. Evaluation is not supported.
    inline double getWidth(const std::string& key,
                           const std::vector<utymap::entities::Tag>& tags,
                           utymap::index::StringTable& stringTable,
                           const utymap::mapcss::Style& style,
                           const utymap::GeoCoordinate& coordinate,
                           int levelOfDetail)
    {
        
        uint32_t keyId = stringTable.getId(key);
        auto declaration = style.get(keyId);
        auto rawValue = declaration->value();

        // defined in meters: use raw value
        if ((*rawValue)[rawValue->size() - 1] == 'm') {
            double meters = std::stod(rawValue->substr(0, rawValue->size() - 1));
            return GeoUtils::getOffset(coordinate, meters);
        }

        // defined in grad: use lod ratio
        return std::stod(*style.get(keyId)->value()) * GeoUtils::getLodRatio(levelOfDetail);
    }
}}

#endif // UTILS_MAPCSSUTILS_HPP_DEFINED