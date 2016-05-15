#ifndef MAPCSS_STYLE_HPP_INCLUDED
#define MAPCSS_STYLE_HPP_INCLUDED

#include "Exceptions.hpp"
#include "mapcss/StyleDeclaration.hpp"
#include "utils/CompatibilityUtils.hpp"

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>

namespace utymap { namespace mapcss {

// Represents style for element.
struct Style
{
    typedef std::uint32_t key_type;
    typedef std::shared_ptr<utymap::mapcss::StyleDeclaration> value_type;

    std::unordered_map<key_type, value_type> declarations;

    inline bool has(key_type key) const
    {
        return declarations.find(key) != declarations.end();
    }

    inline bool has(key_type key, const std::string& value) const
    {
        auto it = declarations.find(key);
        return it != declarations.end() && *it->second->value() == value;
    }

    inline void put(const value_type& declaration)
    {
        declarations[declaration->key()] = declaration;
    }

    inline value_type get(key_type key) const
    {
        auto it = declarations.find(key);
        if (it == declarations.end())
            throw MapCssException(std::string("Cannot find declaration with the key:") + std::to_string(key));

        return it->second;
    }
};

}}
#endif  // MAPCSS_STYLE_HPP_INCLUDED
