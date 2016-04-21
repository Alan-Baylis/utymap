#ifndef ENTITIES_ELEMENTVISITOR_HPP_DEFINED
#define ENTITIES_ELEMENTVISITOR_HPP_DEFINED

namespace utymap { namespace entities {

class Node;
class Way;
class Area;
class Relation;

class ElementVisitor
{
public:
    // Visits node.
    virtual void visitNode(const Node&) = 0;

    // Visits way.
    virtual void visitWay(const Way&) = 0;

    // Visits area.
    virtual void visitArea(const Area&) = 0;

    // Visits relation.
    virtual void visitRelation(const Relation&) = 0;
};

}}

#endif // ENTITIES_ELEMENTVISITOR_HPP_DEFINED
