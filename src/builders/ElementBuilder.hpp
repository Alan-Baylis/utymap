#ifndef BUILDERS_ELEMENTBUILDER_HPP_DEFINED
#define BUILDERS_ELEMENTBUILDER_HPP_DEFINED

#include "QuadKey.hpp"
#include "entities/ElementVisitor.hpp"

namespace utymap { namespace builders {

// Responsible for mesh building from one or multimply elements inside quadkey.
class ElementBuilder: public utymap::entities::ElementVisitor
{
public:
    // This method called before first matched element is visited.
    virtual void prepare(const utymap::QuadKey& quadKey) = 0;

    // This method called once all elements are visited.
    virtual void complete() = 0;
};

}}

#endif // BUILDERS_ELEMENTBUILDER_HPP_DEFINED
