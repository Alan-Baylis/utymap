#include "BoundingBox.hpp"
#include "GeoCoordinate.hpp"
#include "entities/Node.hpp"
#include "entities/Way.hpp"
#include "entities/Area.hpp"
#include "entities/ElementVisitor.hpp"
#include "index/InMemoryElementStore.hpp"

using namespace utymap;
using namespace utymap::index;
using namespace utymap::entities;
using namespace utymap::mapcss;

class InMemoryElementStore::InMemoryElementStoreImpl : public ElementVisitor
{
public:

    InMemoryElementStoreImpl() :
        currentQuadKey_()
    {
    }

    void visitNode(const utymap::entities::Node& node)
    {
    }

    void visitWay(const utymap::entities::Way& way)
    {
    }

    void visitArea(const utymap::entities::Area& area)
    {
    }

    void visitRelation(const utymap::entities::Relation& relation)
    {
    }

    void setQuadKey(const QuadKey& quadKey) { currentQuadKey_ = quadKey; }

private:
    QuadKey currentQuadKey_;
};

InMemoryElementStore::InMemoryElementStore(const StyleProvider& styleProvider, StringTable& stringTable) :
ElementStore(styleProvider, stringTable),
    pimpl_(std::unique_ptr<InMemoryElementStore::InMemoryElementStoreImpl>(
        new InMemoryElementStore::InMemoryElementStoreImpl()))
{
}

InMemoryElementStore::~InMemoryElementStore()
{
}
void InMemoryElementStore::store(const utymap::entities::Element& element, const QuadKey& quadKey)
{
    pimpl_->setQuadKey(quadKey);
    element.accept(*pimpl_);
}
