#ifndef INDEX_PERSISTENTELEMENTSTORE_HPP_DEFINED
#define INDEX_PERSISTENTELEMENTSTORE_HPP_DEFINED

#include "QuadKey.hpp"
#include "entities/Element.hpp"
#include "index/ElementStore.hpp"

#include <string>
#include <memory>

namespace utymap { namespace index {

// Provides API to store elements in persistent store.
class PersistentElementStore : public ElementStore
{
public:
    PersistentElementStore(const std::string& path, const StyleFilter& styleFilter);

    ~PersistentElementStore();

protected:
    void store(const utymap::entities::Element& element, const QuadKey& quadKey);

private:
    class PersistentElementStoreImpl;
    std::unique_ptr<PersistentElementStoreImpl> pimpl_;
};

}}

#endif // INDEX_PERSISTENTELEMENTSTORE_HPP_DEFINED
