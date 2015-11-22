#ifndef ENTITIES_ELEMENT_HPP_DEFINED
#define ENTITIES_ELEMENT_HPP_DEFINED

#include "ElementVisitor.hpp"
#include "../index/StringTable.hpp"

#include <cstdint>
#include <string>
#include <sstream>
#include <vector>

namespace utymap { namespace entities {

// Represents tag
struct Tag
{
    // Returns key string id.
    uint32_t key;
    // Returns value string id.
    uint32_t value;

    Tag(uint32_t key, uint32_t value) : key(key), value(value) { }
};

// Represents element stored in index.
class Element
{
public:
    // Returns id of given element.
    uint64_t id;
    // Returns tag collection represented by vector of tuple<uint,uint>.
    std::vector<Tag> tags;

    // Visitor method.
    virtual void accept(ElementVisitor&) const = 0;

    // Returns string reprsentation of element.
    virtual std::string toString(utymap::index::StringTable& st) const
    {
        std::ostringstream stm;

        stm << '[' << id << ']' << "{";
        for (const Tag& tag : tags) {
            stm << (st.getString(tag.key) + ':' +
                    st.getString(tag.value) + ',');
        }
        stm << "}";

        return stm.str();
    }
};

}}
#endif // ENTITIES_ELEMENT_HPP_DEFINED
