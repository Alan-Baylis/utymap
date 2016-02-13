#include "BoundingBox.hpp"
#include "Exceptions.hpp"
#include "clipper/clipper.hpp"
#include "builders/TerraBuilder.hpp"
#include "entities/ElementVisitor.hpp"
#include "meshing/Polygon.hpp"
#include "meshing/MeshBuilder.hpp"
#include "meshing/LineGridSplitter.hpp"
#include "index/GeoUtils.hpp"
#include "utils/CompatibilityUtils.hpp"
#include "utils/MapCssUtils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <sstream>
#include <map>
#include <unordered_map>
#include <vector>

using namespace ClipperLib;
using namespace utymap::builders;
using namespace utymap::entities;
using namespace utymap::index;
using namespace utymap::heightmap;
using namespace utymap::mapcss;
using namespace utymap::meshing;

const uint64_t Scale = 1E7; // max precision for Lat/Lon: seven decimal positions
const double Tolerance = 10; // Tolerance for splitting algorithm
const double AreaTolerance = 100; // Tolerance for meshing

// Properties of terrain region union.
struct Properties
{
    std::string gradientKey;
    float eleNoiseFreq;
    float colorNoiseFreq;
    float maxArea;
    float heightOffset;

    Properties() : gradientKey(), eleNoiseFreq(0), colorNoiseFreq(0),
        heightOffset(0), maxArea(0)
    {
    }
};

// Represents terrain region points.
struct Region
{
    bool isLayer;
    Properties properties;
    Paths points;
};

typedef std::vector<Point> Points;
typedef std::vector<Region> Regions;
typedef std::unordered_map<std::string, Regions> Layers;

// mapcss specific keys
const static std::string TerrainLayerKey = "terrain-layer";
const static std::string ColorNoiseFreqKey = "color-noise-freq";
const static std::string EleNoiseFreqKey = "ele-noise-freq";
const static std::string GradientKey= "color";
const static std::string MaxAreaKey = "max-area";
const static std::string WidthKey = "width";
const static std::string LayerPriorityKey = "layer-priority";

class TerraBuilder::TerraBuilderImpl : public ElementVisitor
{
public:

    TerraBuilderImpl(StringTable& stringTable, const StyleProvider& styleProvider, 
        ElevationProvider& eleProvider, std::function<void(const Mesh&)> callback) :
            stringTable_(stringTable), styleProvider_(styleProvider), meshBuilder_(eleProvider), 
            callback_(callback), quadKey_(),splitter_()
    {
    }

    inline void setQuadKey(const QuadKey& quadKey)
    {
        quadKey_ = quadKey;
    }

    void visitNode(const utymap::entities::Node& node)
    {
    }

    void visitWay(const utymap::entities::Way& way)
    {
        Style style = styleProvider_.forElement(way, quadKey_.levelOfDetail);
        Region region = createRegion(style, way.coordinates);

        // make polygon from line by offsetting it using width specified
        float width = utymap::utils::getFloat(WidthKey, stringTable_, style);
        Paths offsetSolution;
        offset_.AddPaths(region.points, jtMiter, etOpenSquare);
        offset_.Execute(offsetSolution, width * Scale);
        offset_.Clear();
        region.points = offsetSolution;

        std::string type = region.isLayer ? utymap::utils::getString(TerrainLayerKey, stringTable_, style) : "";
        layers_[type].push_back(region);
    }

    void visitArea(const utymap::entities::Area& area)
    {
        Style style = styleProvider_.forElement(area, quadKey_.levelOfDetail);
        Region region = createRegion(style, area.coordinates);
        std::string type = region.isLayer 
            ? utymap::utils::getString(TerrainLayerKey, stringTable_, style)
            : "";
        layers_[type].push_back(region);
    }

    void visitRelation(const utymap::entities::Relation& relation)
    {
        Region region;
        struct RelationVisitor : public ElementVisitor
        {
            const Relation& relation;
            TerraBuilder::TerraBuilderImpl& builder;
            Region& region;

            RelationVisitor(TerraBuilder::TerraBuilderImpl& builder, const Relation& relation, Region& region) :
                builder(builder), relation(relation), region(region) {}

            void visitNode(const utymap::entities::Node& node) { node.accept(builder); }

            void visitWay(const utymap::entities::Way& way) { way.accept(builder); }

            void visitArea(const utymap::entities::Area& area) 
            {
                Path path;
                path.reserve(area.coordinates.size());
                for (const GeoCoordinate& c : area.coordinates) {
                    path.push_back(IntPoint(c.longitude * Scale, c.latitude * Scale));
                }
                region.points.push_back(path);
            }

            void visitRelation(const utymap::entities::Relation& relation)  { relation.accept(builder); }
        } visitor(*this, relation, region);

        for (const auto& element : relation.elements) {
            // if there are no tags, then this element is result of clipping
            if (element->tags.empty())
                element->tags = relation.tags;
            element->accept(visitor);
        }

        if (!region.points.empty()) {
            Style style = styleProvider_.forElement(relation, quadKey_.levelOfDetail);
            region.isLayer = style.has(stringTable_.getId(TerrainLayerKey));
            if (!region.isLayer) {
                region.properties = createRegionProperties(style, "");
            }
            std::string type = region.isLayer ? utymap::utils::getString(TerrainLayerKey, stringTable_, style) : "";
            layers_[type].push_back(region);
        }
    }

    // builds tile mesh using data provided.
    void build()
    {
        configureSplitter(quadKey_.levelOfDetail);
        Style style = styleProvider_.forCanvas(quadKey_.levelOfDetail);

        buildLayers(style);
        buildBackground(style);

        callback_(mesh_);
    }
private:

    void configureSplitter(int levelOfDetails)
    {    
        // TODO
        switch (levelOfDetails)
        {
            case 1: splitter_.setParams(Scale, 3, Tolerance); break;
            default: throw std::domain_error("Unknown Level of details:" + std::to_string(levelOfDetails));
        };
    }

    Region createRegion(const Style& style, const std::vector<GeoCoordinate>& coordinates)
    {
        Region region;
        Path path;
        path.reserve(coordinates.size());
        for (const GeoCoordinate& c : coordinates) {
            path.push_back(IntPoint(c.longitude * Scale, c.latitude * Scale));
        }
        region.points.push_back(path);

        region.isLayer = style.has(stringTable_.getId(TerrainLayerKey));

        if (!region.isLayer)
            region.properties = createRegionProperties(style, "");

        return std::move(region);
    }

    Properties createRegionProperties(const Style& style, const std::string& prefix)
    {
        Properties properties;
        properties.eleNoiseFreq = utymap::utils::getFloat(prefix + EleNoiseFreqKey, stringTable_, style);
        properties.colorNoiseFreq = utymap::utils::getFloat(prefix + ColorNoiseFreqKey, stringTable_, style);
        properties.gradientKey = utymap::utils::getString(prefix + GradientKey, stringTable_, style);
        properties.maxArea = utymap::utils::getFloat(prefix + MaxAreaKey, stringTable_, style);
        return std::move(properties);
    }

    void buildFromRegions(const Regions& regions, const Properties& properties)
    {
        // merge all regions together
        Clipper clipper;
        for (const Region& region : regions) {
            clipper.AddPaths(region.points, ptSubject, true);
        }
        Paths result;
        clipper.Execute(ctUnion, result, pftPositive, pftPositive);

        buildFromPaths(result, properties);
    }

    void buildFromPaths(Paths& path, const Properties& properties)
    {
        clipper_.AddPaths(path, ptSubject, true);
        path.clear();
        clipper_.Execute(ctDifference, path, pftPositive, pftPositive);
        clipper_.moveSubjectToClip();
        populateMesh(properties, path);
    }

    // process all found layers.
    void buildLayers(const Style& style)
    {
        // 1. process layers: regions with shared properties.
        std::stringstream ss(utymap::utils::getString(LayerPriorityKey, stringTable_, style));
        while (ss.good())
        {
            std::string name;
            getline(ss, name, ',');
            auto layer = layers_.find(name);
            if (layer != layers_.end()) {
                Properties properties = createRegionProperties(style, name + "-");
                buildFromRegions(layer->second, properties);
                layers_.erase(layer);
            }
        }

        // 2. Process the rest: each region has aready its own properties.
        for (auto& layer : layers_) {
            for (auto& region : layer.second) {
                buildFromPaths(region.points, region.properties);
            }
        }
    }

    // process the rest area.
    void buildBackground(const Style& style)
    {
        BoundingBox bbox = utymap::index::GeoUtils::quadKeyToBoundingBox(quadKey_);
        Path tileRect;
        tileRect.push_back(IntPoint(bbox.minPoint.longitude*Scale, bbox.minPoint.latitude *Scale));
        tileRect.push_back(IntPoint(bbox.maxPoint.longitude *Scale, bbox.minPoint.latitude *Scale));
        tileRect.push_back(IntPoint(bbox.maxPoint.longitude *Scale, bbox.maxPoint.latitude*Scale));
        tileRect.push_back(IntPoint(bbox.minPoint.longitude*Scale, bbox.maxPoint.latitude*Scale));

        clipper_.AddPath(tileRect, ptSubject, true);
        Paths background;
        clipper_.Execute(ctDifference, background, pftPositive, pftPositive);
        clipper_.Clear();

        if (!background.empty())
            populateMesh(createRegionProperties(style, ""), background);
    }
    
    void populateMesh(const Properties& properties, Paths& paths)
    {
        ClipperLib::SimplifyPolygons(paths);
        ClipperLib::CleanPolygons(paths);

        bool hasHeightOffset = properties.heightOffset > 0;
        // calculate approximate size of overall points
        auto size = 0;
        for (auto i = 0; i < paths.size(); ++i) {
            size += paths[i].size() * 1.5;
        }

        Polygon polygon(size);
        for (const Path& path : paths) {
            double area = ClipperLib::Area(path);

            if (std::abs(area) < AreaTolerance) continue;

            Points points = restorePoints(path);
            if (area < 0)
                polygon.addHole(points);
            else
                polygon.addContour(points);
        }

        if (!polygon.points.empty())
            fillMesh(properties, polygon);
    }

    // restores mesh points from clipper points and injects new ones according to grid.
    Points restorePoints(const Path& path)
    {
        int lastItemIndex = path.size() - 1;
        Points points;
        points.reserve(path.size());
        for (int i = 0; i <= lastItemIndex; i++) {
            splitter_.split(path[i], path[i == lastItemIndex ? 0 : i + 1], points);
        }

        return std::move(points);
    }

    void processHeightOffset(const std::vector<Points>& contours)
    {
        // TODO
    }

    void fillMesh(const Properties& properties, Polygon& polygon)
    {
        Mesh regionMesh = meshBuilder_.build(polygon, MeshBuilder::Options
        {
            /* area=*/ properties.maxArea,
            /* elevation noise frequency*/ properties.eleNoiseFreq,
            /* color noise frequency. */ properties.colorNoiseFreq,
            styleProvider_.getGradient(properties.gradientKey),
            /* segmentSplit=*/ 0
        });

        auto startVertIndex = mesh_.vertices.size() / 3;
        mesh_.vertices.insert(mesh_.vertices.end(),
            regionMesh.vertices.begin(),
            regionMesh.vertices.end());

        for (const auto& tri : regionMesh.triangles) {
            mesh_.triangles.push_back(startVertIndex + tri);
        }

        mesh_.colors.insert(mesh_.colors.end(),
            regionMesh.colors.begin(),
            regionMesh.colors.end());
    }

const StyleProvider& styleProvider_;
StringTable& stringTable_;
std::function<void(const Mesh&)> callback_;

ClipperEx clipper_;
ClipperOffset offset_;
LineGridSplitter splitter_;
MeshBuilder meshBuilder_;
QuadKey quadKey_;
Layers layers_;
Mesh mesh_;
};

void TerraBuilder::visitNode(const utymap::entities::Node& node) { pimpl_->visitNode(node); }

void TerraBuilder::visitWay(const utymap::entities::Way& way) { pimpl_->visitWay(way); }

void TerraBuilder::visitArea(const utymap::entities::Area& area) { pimpl_->visitArea(area); }

void TerraBuilder::visitRelation(const utymap::entities::Relation& relation) { pimpl_->visitRelation(relation); }

void TerraBuilder::prepare(const utymap::QuadKey& quadKey) { pimpl_->setQuadKey(quadKey); }

void TerraBuilder::complete() { pimpl_->build(); }

TerraBuilder::~TerraBuilder() { }

TerraBuilder::TerraBuilder(StringTable& stringTable, const StyleProvider& styleProvider, ElevationProvider& eleProvider,
                           std::function<void(const Mesh&)> callback) :
    pimpl_(std::unique_ptr<TerraBuilder::TerraBuilderImpl>(new TerraBuilder::TerraBuilderImpl(stringTable, styleProvider, eleProvider, callback)))
{
}
