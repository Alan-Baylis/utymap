#include "BoundingBox.hpp"
#include "Exceptions.hpp"
#include "clipper/clipper.hpp"
#include "builders/BuilderContext.hpp"
#include "builders/terrain/TerraBuilder.hpp"
#include "builders/terrain/TerraGenerator.hpp"
#include "entities/Node.hpp"
#include "entities/Way.hpp"
#include "entities/Area.hpp"
#include "entities/Relation.hpp"

#include "utils/GeoUtils.hpp"
#include "utils/GeometryUtils.hpp"
#include "utils/NoiseUtils.hpp"

#include <map>

using namespace ClipperLib;
using namespace utymap::builders;
using namespace utymap::entities;
using namespace utymap::index;
using namespace utymap::heightmap;
using namespace utymap::mapcss;
using namespace utymap::meshing;
using namespace utymap::utils;

namespace {
    const static double Scale = 1E7;
    const static std::string TerrainLayerKey = "terrain-layer";
    const static std::string WidthKey = "width";

    // Converts coordinate to clipper's IntPoint.
    inline IntPoint toIntPoint(double x, double y)
    {
        return IntPoint((cInt)(x * Scale), (cInt)(y * Scale));
    }

    // Visits relation and fills region.
    struct RelationVisitor : public ElementVisitor
    {
        const Relation& relation;
        ElementVisitor& builder;
        TerraGenerator::Region& region;

        RelationVisitor(ElementVisitor& b, const Relation& r, TerraGenerator::Region& reg) :
                builder(b), relation(r), region(reg) {}

        void visitNode(const utymap::entities::Node& n) { n.accept(builder); }

        void visitWay(const utymap::entities::Way& w) { w.accept(builder); }

        void visitArea(const utymap::entities::Area& a)
        {
            Path path;
            path.reserve(a.coordinates.size());
            for (const utymap::GeoCoordinate& c : a.coordinates)
                path.push_back(toIntPoint(c.longitude, c.latitude));

            region.points.push_back(path);
            region.area += std::abs(utymap::utils::getArea(a.coordinates));
        }

        void visitRelation(const utymap::entities::Relation& r)  { r.accept(builder); }
    };
}

class TerraBuilder::TerraBuilderImpl : public ElementBuilder
{
public:

    TerraBuilderImpl(const BuilderContext& context) :
        ElementBuilder(context), 
        style_(context.styleProvider.forCanvas(context.quadKey.levelOfDetail)), 
        clipper_(),
        generator_(context, style_, clipper_)
    {
        tileRect_.push_back(toIntPoint(context.boundingBox.minPoint.longitude, context.boundingBox.minPoint.latitude));
        tileRect_.push_back(toIntPoint(context.boundingBox.maxPoint.longitude, context.boundingBox.minPoint.latitude));
        tileRect_.push_back(toIntPoint(context.boundingBox.maxPoint.longitude, context.boundingBox.maxPoint.latitude));
        tileRect_.push_back(toIntPoint(context.boundingBox.minPoint.longitude, context.boundingBox.maxPoint.latitude));

        clipper_.AddPath(tileRect_, ptClip, true);
    }

    void visitNode(const utymap::entities::Node& node)
    {
    }

    void visitWay(const utymap::entities::Way& way)
    {
        Style style = context_.styleProvider.forElement(way, context_.quadKey.levelOfDetail);
        auto region = createRegion(style, way.coordinates);

        // make polygon from line by offsetting it using width specified
        double width = style.getValue(WidthKey, 
            context_.boundingBox.maxPoint.latitude - context_.boundingBox.minPoint.latitude,
            context_.boundingBox.center());

        Paths solution;
        offset_.AddPaths(region->points, jtMiter, etOpenSquare);
        offset_.Execute(solution, width *  Scale);
        offset_.Clear();
       
        clipper_.AddPaths(solution, ptSubject, true);
        clipper_.Execute(ctIntersection, solution);
        clipper_.removeSubject();

        region->points = solution;
        std::string type = region->isLayer
            ? *style.getString(TerrainLayerKey)
            : "";
        generator_.addRegion(type, region);
    }

    void visitArea(const utymap::entities::Area& area)
    {
        Style style = context_.styleProvider.forElement(area, context_.quadKey.levelOfDetail);
        auto region = createRegion(style, area.coordinates);
        std::string type = region->isLayer
            ? *style.getString(TerrainLayerKey)
            : "";
        generator_.addRegion(type, region);
    }

    void visitRelation(const utymap::entities::Relation& rel)
    {
        auto region = std::make_shared<TerraGenerator::Region>();
        RelationVisitor visitor(*this, rel, *region);

        for (const auto& element : rel.elements) {
            // if there are no tags, then this element is result of clipping
            if (element->tags.empty())
                element->tags = rel.tags;

            if (context_.styleProvider.hasStyle(rel, context_.quadKey.levelOfDetail))
                element->accept(visitor);
        }

        if (!region->points.empty()) {
            Style style = context_.styleProvider.forElement(rel, context_.quadKey.levelOfDetail);
            region->isLayer = style.has(context_.stringTable.getId(TerrainLayerKey));
            if (!region->isLayer)
                region->context = std::make_shared<TerraGenerator::RegionContext>(generator_.createRegionContext(style, ""));

            std::string type = region->isLayer 
                ? *style.getString(TerrainLayerKey)
                : "";
            generator_.addRegion(type, region);
        }
    }

    // builds tile mesh using data provided.
    void complete()
    {
        clipper_.Clear();
        generator_.generate(tileRect_);
    }

private:

    std::shared_ptr<TerraGenerator::Region> createRegion(const Style& style, const std::vector<GeoCoordinate>& coordinates)
    {
        auto region = std::make_shared<TerraGenerator::Region>();
        Path path;
        path.reserve(coordinates.size());
        for (const GeoCoordinate& c : coordinates)
            path.push_back(toIntPoint(c.longitude, c.latitude));

        region->points.push_back(path);

        region->isLayer = style.has(context_.stringTable.getId(TerrainLayerKey));
        if (!region->isLayer)
            region->context = std::make_shared<TerraGenerator::RegionContext>(generator_.createRegionContext(style, ""));

        region->area = std::abs(utymap::utils::getArea(coordinates));

        return region;
    }

    const Style style_;
    ClipperEx clipper_;
    ClipperOffset offset_;
    TerraGenerator generator_;
    ClipperLib::Path tileRect_;
};

void TerraBuilder::visitNode(const utymap::entities::Node& node) { pimpl_->visitNode(node); }

void TerraBuilder::visitWay(const utymap::entities::Way& way) { pimpl_->visitWay(way); }

void TerraBuilder::visitArea(const utymap::entities::Area& area) { pimpl_->visitArea(area); }

void TerraBuilder::visitRelation(const utymap::entities::Relation& relation) { pimpl_->visitRelation(relation); }

void TerraBuilder::complete() { pimpl_->complete(); }

TerraBuilder::~TerraBuilder() { }

TerraBuilder::TerraBuilder(const BuilderContext& context) :
    utymap::builders::ElementBuilder(context),
    pimpl_(new TerraBuilder::TerraBuilderImpl(context))
{
}
