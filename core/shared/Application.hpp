#ifndef APPLICATION_HPP_DEFINED
#define APPLICATION_HPP_DEFINED

#include "BoundingBox.hpp"
#include "QuadKey.hpp"
#include "LodRange.hpp"
#include "builders/BuilderContext.hpp"
#include "builders/QuadKeyBuilder.hpp"
#include "builders/buildings/BuildingBuilder.hpp"
#include "builders/misc/BarrierBuilder.hpp"
#include "builders/poi/TreeBuilder.hpp"
#include "builders/terrain/TerraBuilder.hpp"
#include "heightmap/FlatElevationProvider.hpp"
#include "heightmap/SrtmElevationProvider.hpp"
#include "index/GeoStore.hpp"
#include "index/InMemoryElementStore.hpp"
#include "index/PersistentElementStore.hpp"
#include "mapcss/MapCssParser.hpp"
#include "mapcss/StyleSheet.hpp"
#include "mapcss/TextureAtlasParser.hpp"
#include "meshing/MeshTypes.hpp"
#include "utils/CoreUtils.hpp"
#include "utils/GeoUtils.hpp"

#include "Callbacks.hpp"
#include "ExportElementVisitor.hpp"

#include <cstdint>
#include <exception>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

// Exposes API for external usage.
class Application
{
    const int SrtmElevationLodStart = 42; // NOTE: disable for initial MVP

public:

    // Composes object graph.
    Application(const char* stringPath, 
                const char* elePath, 
                OnError* errorCallback) :
        stringTable_(stringPath), geoStore_(stringTable_), flatEleProvider_(),
        srtmEleProvider_(elePath), quadKeyBuilder_(geoStore_, stringTable_)
    {
        registerDefaultBuilders();
    }

    // Register stylesheet.
    void registerStylesheet(const char* path)
    {
        getStyleProvider(path);
    }

    void registerInMemoryStore(const char* key)
    {
        geoStore_.registerStore(key, utymap::utils::make_unique<utymap::index::InMemoryElementStore>(stringTable_));
    }

    void registerPersistentStore(const char* key, const char* dataPath)
    {
        geoStore_.registerStore(key, utymap::utils::make_unique<utymap::index::PersistentElementStore>(dataPath, stringTable_));
    }

    // Preload elevation data. Not thread safe.
    void preloadElevation(const utymap::QuadKey& quadKey)
    {
        getElevationProvider(quadKey).preload(utymap::utils::GeoUtils::quadKeyToBoundingBox(quadKey));
    }

    // Adds data to store.
    void addToStore(const char* key, 
                    const char* styleFile, 
                    const char* path, 
                    const utymap::QuadKey& quadKey, 
                    OnError* errorCallback)
    {
        safeExecute([&]() {
            geoStore_.add(key, path, quadKey, getStyleProvider(styleFile));
        }, errorCallback);
    }

    // Adds data to store.
    void addToStore(const char* key, 
                    const char* styleFile, 
                    const char* path, 
                    const utymap::BoundingBox& bbox, 
                    const utymap::LodRange& range, 
                    OnError* errorCallback)
    {
        safeExecute([&]() {
            geoStore_.add(key, path, bbox, range, getStyleProvider(styleFile));
        }, errorCallback);
    }

    // Adds data to store.
    void addToStore(const char* key, 
                    const char* styleFile, 
                    const char* path, 
                    const utymap::LodRange& range, 
                    OnError* errorCallback)
    {
        safeExecute([&]() {
            geoStore_.add(key, path, range, getStyleProvider(styleFile));
        }, errorCallback);
    }

    // Adds element to in-memory store.
    void addToStore(const char* key,
                    const char* styleFile, 
                    const utymap::entities::Element& element, 
                    const utymap::LodRange& range, 
                    OnError* errorCallback)
    {
        safeExecute([&]() {
            geoStore_.add(key, element, range, getStyleProvider(styleFile));
        }, errorCallback);
    }

    bool hasData(const utymap::QuadKey& quadKey)
    {
        return geoStore_.hasData(quadKey);
    }

    // Loads quadKey.
    void loadQuadKey(const char* styleFile, 
                     const utymap::QuadKey& quadKey, 
                     OnMeshBuilt* meshCallback,
                     OnElementLoaded* elementCallback, 
                     OnError* errorCallback)
    {
        safeExecute([&]() {
            auto& styleProvider = getStyleProvider(styleFile);
            ExportElementVisitor elementVisitor(stringTable_, styleProvider, quadKey.levelOfDetail, elementCallback);
            quadKeyBuilder_.build(quadKey, styleProvider, getElevationProvider(quadKey),
                [&meshCallback](const utymap::meshing::Mesh& mesh) {
                // NOTE do not notify if mesh is empty.
                if (!mesh.vertices.empty()) {
                    meshCallback(mesh.name.data(),
                        mesh.vertices.data(), static_cast<int>(mesh.vertices.size()),
                        mesh.triangles.data(), static_cast<int>(mesh.triangles.size()),
                        mesh.colors.data(), static_cast<int>(mesh.colors.size()));
                }
            }, [&elementVisitor](const utymap::entities::Element& element) {
                element.accept(elementVisitor);
            });
        }, errorCallback);
    }

    // Gets id for the string.
    std::uint32_t getStringId(const char* str) const
    {
        return stringTable_.getId(str);
    }

private:

    static void safeExecute(const std::function<void()>& action, 
                     OnError* errorCallback)
    {
        try {
            action();
        }
        catch (std::exception& ex) {
            errorCallback(ex.what());
        }
    }

    utymap::heightmap::ElevationProvider& getElevationProvider(const utymap::QuadKey& quadKey)
    {
        return quadKey.levelOfDetail <= SrtmElevationLodStart
            ? flatEleProvider_
            : static_cast<utymap::heightmap::ElevationProvider&>(srtmEleProvider_);
    }

    const utymap::mapcss::StyleProvider& getStyleProvider(const std::string& filePath)
    {
        auto pair = styleProviders_.find(filePath);
        if (pair != styleProviders_.end())
            return *pair->second;

        std::ifstream styleFile(filePath);
        if (!styleFile.good())
            throw std::invalid_argument(std::string("Cannot read mapcss file:") + filePath);

        // NOTE not safe, but don't want to use boost filesystem only for this task.
        std::string dir = filePath.substr(0, filePath.find_last_of("\\/") + 1);
        utymap::mapcss::MapCssParser parser(dir);
        utymap::mapcss::StyleSheet stylesheet = parser.parse(styleFile);

        // TODO pass texture atlas string
        auto textureAtlas = utymap::mapcss::TextureAtlasParser::parse("");
        
        styleProviders_.emplace(filePath, utymap::utils::make_unique<const utymap::mapcss::StyleProvider>(
            stylesheet, textureAtlas, stringTable_));

        return *styleProviders_[filePath];
    }

    void registerDefaultBuilders()
    {
        quadKeyBuilder_.registerElementBuilder("terrain", [&](const utymap::builders::BuilderContext& context) {
            return utymap::utils::make_unique<utymap::builders::TerraBuilder>(context);
        });

        quadKeyBuilder_.registerElementBuilder("building", [&](const utymap::builders::BuilderContext& context) {
            return utymap::utils::make_unique<utymap::builders::BuildingBuilder>(context);
        });

        quadKeyBuilder_.registerElementBuilder("tree", [&](const utymap::builders::BuilderContext& context) {
            return utymap::utils::make_unique<utymap::builders::TreeBuilder>(context);
        });

        quadKeyBuilder_.registerElementBuilder("barrier", [&](const utymap::builders::BuilderContext& context) {
            return utymap::utils::make_unique<utymap::builders::BarrierBuilder>(context);
        });
    }

    utymap::index::StringTable stringTable_;
    utymap::index::GeoStore geoStore_;

    utymap::heightmap::FlatElevationProvider flatEleProvider_;
    utymap::heightmap::SrtmElevationProvider srtmEleProvider_;

    utymap::builders::QuadKeyBuilder quadKeyBuilder_;
    std::unordered_map<std::string, std::unique_ptr<const utymap::mapcss::StyleProvider>> styleProviders_;
};

#endif // APPLICATION_HPP_DEFINED
