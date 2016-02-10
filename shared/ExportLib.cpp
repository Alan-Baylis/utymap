#if _MSC_VER
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API
#endif

#include "QuadKey.hpp"
#include "builders/ElementBuilder.hpp"
#include "builders/ExternalBuilder.hpp"
#include "builders/TerraBuilder.hpp"
#include "builders/TileBuilder.hpp"
#include "heightmap/FlatElevationProvider.hpp"
#include "index/GeoStore.hpp"
#include "index/InMemoryElementStore.hpp"
#include "index/PersistentElementStore.hpp"
#include "index/LodRange.hpp"
#include "index/StringTable.hpp"
#include "mapcss/MapCssParser.hpp"
#include "mapcss/StyleProvider.hpp"
#include "mapcss/StyleSheet.hpp"
#include "meshing/MeshTypes.hpp"

#include <cstdint>
#include <string>
#include <fstream>

static utymap::TileBuilder* tileLoaderPtr = nullptr;
static utymap::index::GeoStore* geoStorePtr = nullptr;
static utymap::index::InMemoryElementStore* inMemoryStorePtr = nullptr;
static utymap::index::PersistentElementStore* persistentStorePtr = nullptr;
static utymap::index::StringTable* stringTablePtr = nullptr;
static utymap::mapcss::StyleProvider* styleProviderPtr = nullptr;
static utymap::heightmap::ElevationProvider* eleProviderPtr = nullptr;

const std::string InMemoryStorageKey = "InMemory";
const std::string PersistentStorageKey = "OnDisk";

extern "C"
{
    // Called when mesh is built.
    typedef void OnMeshBuilt(const char* name,
                             const double* vertices, int vertexCount,
                             const int* triangles, int triCount,
                             const int* colors, int colorCount);
    // Called when element is loaded.
    typedef void OnElementLoaded(uint64_t id, const char* name,
                                 const char** tags, int size,
                                 const double* vertices, int vertexCount);
    // Called when operation is completed
    typedef void OnError(const char* errorMessage);

    // Composes object graph.
    void EXPORT_API configure(const char* stringPath, // path to string table directory
                              const char* stylePath,   // path to mapcss file
                              const char* dataPath,    // path to index directory
                              OnError* errorCallback)
    {
        std::ifstream styleFile(stylePath);
        utymap::mapcss::MapCssParser parser;
        utymap::mapcss::StyleSheet stylesheet = parser.parse(styleFile);

        stringTablePtr = new utymap::index::StringTable(stringPath);
        styleProviderPtr = new utymap::mapcss::StyleProvider(stylesheet, *stringTablePtr);

        inMemoryStorePtr = new utymap::index::InMemoryElementStore(*stringTablePtr);
        persistentStorePtr = new utymap::index::PersistentElementStore(dataPath, *stringTablePtr);

        geoStorePtr = new utymap::index::GeoStore(*stringTablePtr);
        geoStorePtr->registerStore(InMemoryStorageKey, *inMemoryStorePtr);
        geoStorePtr->registerStore(PersistentStorageKey, *persistentStorePtr);

        eleProviderPtr = new utymap::heightmap::FlatElevationProvider();
        tileLoaderPtr = new utymap::TileBuilder(*geoStorePtr, *stringTablePtr, *styleProviderPtr);

        // register predefined element builders
        tileLoaderPtr->registerElementBuilder("terrain", [&](const utymap::TileBuilder::MeshCallback& meshFunc,
                                                             const utymap::TileBuilder::ElementCallback& elementFunc) {
            return std::shared_ptr<utymap::builders::ElementBuilder>(
                new utymap::builders::TerraBuilder(*stringTablePtr, *styleProviderPtr, *eleProviderPtr, meshFunc));
        });
    }

    // Registers external element builder.
    void EXPORT_API registerElementBuilder(const char* name)
    {
        tileLoaderPtr->registerElementBuilder(name, [&](const utymap::TileBuilder::MeshCallback& meshFunc,
                                                        const utymap::TileBuilder::ElementCallback& elementFunc) {
            return std::shared_ptr<utymap::builders::ElementBuilder>(new utymap::builders::ExternalBuilder(elementFunc));
        });
    }

    void EXPORT_API cleanup()
    {
        delete tileLoaderPtr;
        delete geoStorePtr;
        delete persistentStorePtr;
        delete inMemoryStorePtr;
        delete stringTablePtr;
        delete styleProviderPtr;
        delete eleProviderPtr;
    }

    // TODO for single element as well

    void EXPORT_API addToPersistentStore(const char* path, int startLod, int endLod, OnError* errorCallback)
    {
        geoStorePtr->add(PersistentStorageKey, path, utymap::index::LodRange(startLod, endLod), *styleProviderPtr);
    }

    void EXPORT_API addToInMemoryStore(const char* path, int startLod, int endLod, OnError* errorCallback)
    {
        geoStorePtr->add(InMemoryStorageKey, path, utymap::index::LodRange(startLod, endLod), *styleProviderPtr);
    }

    void EXPORT_API loadTile(int tileX, int tileY, int levelOfDetail,
                             OnMeshBuilt* meshCallback,
                             OnElementLoaded* elementCallback,
                             OnError* errorCallback)
    {
        utymap::QuadKey quadKey;
        quadKey.tileX = tileX;
        quadKey.tileY = tileY;
        quadKey.levelOfDetail = levelOfDetail;

        tileLoaderPtr->build(quadKey, [&meshCallback](const utymap::meshing::Mesh& mesh) {
            meshCallback(mesh.name.data(),
                mesh.vertices.data(), mesh.vertices.size(),
                mesh.triangles.data(), mesh.triangles.size(),
                mesh.colors.data(), mesh.colors.size());
        }, [&elementCallback](const utymap::entities::Element& element) {
            // TODO call elementCallback
        });
    }

    void EXPORT_API search(double latitude, double longitude, double radius,
                           OnElementLoaded* elementCallback,
                           OnError* errorCallback)
    {
        // TODO
    }
}
