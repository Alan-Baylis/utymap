#define REAL double
#define ANSI_DECLARATORS

#include "BoundingBox.hpp"
#include "meshing/MeshBuilder.hpp"
#include "triangle/triangle.h"
#include "utils/CoreUtils.hpp"
#include "utils/GeoUtils.hpp"
#include "utils/GradientUtils.hpp"

#include <functional>

using namespace utymap::heightmap;
using namespace utymap::meshing;
using namespace utymap::utils;

class MeshBuilder::MeshBuilderImpl
{
public:

    MeshBuilderImpl(const utymap::QuadKey& quadKey, const ElevationProvider& eleProvider) :
        bbox(GeoUtils::quadKeyToBoundingBox(quadKey)), eleProvider_(eleProvider)
    {
    }
     
    void addPolygon(Mesh& mesh, Polygon& polygon, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
    {
        triangulateio in, mid;

        in.numberofpoints = static_cast<int>(polygon.points.size() / 2);
        in.numberofholes = static_cast<int>(polygon.holes.size() / 2);
        in.numberofpointattributes = 0;
        in.numberofregions = 0;
        in.numberofsegments = static_cast<int>(polygon.segments.size() / 2);

        in.pointlist = polygon.points.data();
        in.holelist = polygon.holes.data();
        in.segmentlist = polygon.segments.data();
        in.segmentmarkerlist = nullptr;
        in.pointmarkerlist = nullptr;

        mid.pointlist = nullptr;
        mid.pointmarkerlist = nullptr;
        mid.trianglelist = nullptr;
        mid.segmentlist = nullptr;
        mid.segmentmarkerlist = nullptr;

        ::triangulate(const_cast<char*>("pzBQ"), &in, &mid, nullptr);

        // do not refine mesh if area is not set.
        if (std::abs(geometryOptions.area) < std::numeric_limits<double>::epsilon()) {
            fillMesh(&mid, mesh, geometryOptions, apperanceOptions);
            mid.trianglearealist = nullptr;
        }
        else {

            mid.trianglearealist = static_cast<REAL *>(malloc(mid.numberoftriangles * sizeof(REAL)));
            for (int i = 0; i < mid.numberoftriangles; ++i) {
                mid.trianglearealist[i] = geometryOptions.area;
            }

            triangulateio out;
            out.pointlist = nullptr;
            out.pointattributelist = nullptr;
            out.trianglelist = nullptr;
            out.triangleattributelist = nullptr;
            out.pointmarkerlist = nullptr;

            std::string triOptions = "prazPQ";
            for (int i = 0; i < geometryOptions.segmentSplit; i++) {
                triOptions += "Y";
            }
            ::triangulate(const_cast<char*>(triOptions.c_str()), &mid, &out, nullptr);

            fillMesh(&out, mesh, geometryOptions, apperanceOptions);

            free(out.pointlist);
            free(out.pointattributelist);
            free(out.trianglelist);
            free(out.triangleattributelist);
            free(out.pointmarkerlist);
        }

        free(in.pointmarkerlist);

        free(mid.pointlist);
        free(mid.pointmarkerlist);
        free(mid.trianglelist);
        free(mid.trianglearealist);
        free(mid.segmentlist);
        free(mid.segmentmarkerlist);
    }


    void addPlane(Mesh& mesh, const Vector2& p1, const Vector2& p2, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
    {
        double ele1 = eleProvider_.getElevation(p1.y, p1.x);
        double ele2 = eleProvider_.getElevation(p2.y, p2.x);

        ele1 += NoiseUtils::perlin2D(p1.x, p1.y, geometryOptions.eleNoiseFreq);
        ele2 += NoiseUtils::perlin2D(p2.x, p2.y, geometryOptions.eleNoiseFreq);

        addPlane(mesh, p1, p2, ele1, ele2, geometryOptions, apperanceOptions);
    }

    void addPlane(Mesh& mesh, const Vector2& p1, const Vector2& p2, double ele1, double ele2, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
    {
        auto color = apperanceOptions.gradient.evaluate((NoiseUtils::perlin2D(p1.x, p1.y, apperanceOptions.colorNoiseFreq) + 1) / 2);
        int index = static_cast<int>(mesh.vertices.size() / 3);

        addVertex(mesh, p1, ele1, color, index);
        addVertex(mesh, p2, ele2, color, index + 2);
        addVertex(mesh, p2, ele2 + geometryOptions.heightOffset, color, index + 1);
        index += 3;

        addVertex(mesh, p1, ele1 + geometryOptions.heightOffset, color, index);
        addVertex(mesh, p1, ele1, color, index + 2);
        addVertex(mesh, p2, ele2 + geometryOptions.heightOffset, color, index + 1);
    }

    void addTriangle(Mesh& mesh, const Vector3& v0, const Vector3& v1, const Vector3& v2, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
    {
        auto color = apperanceOptions.gradient.evaluate((NoiseUtils::perlin2D(v0.x, v0.z, apperanceOptions.colorNoiseFreq) + 1) / 2);
        int startIndex = static_cast<int>(mesh.vertices.size() / 3);

        addVertex(mesh, v0, color, startIndex);
        addVertex(mesh, v1, color, ++startIndex);
        addVertex(mesh, v2, color, ++startIndex);

        if (geometryOptions.hasBackSide) {
            // TODO check indices
            addVertex(mesh, v2, color, startIndex);
            addVertex(mesh, v1, color, ++startIndex);
            addVertex(mesh, v0, color, ++startIndex);
        }
    }

private:

    static void addVertex(Mesh& mesh, const Vector2& p, double ele, int color, int triIndex)
    {
        mesh.vertices.push_back(p.x);
        mesh.vertices.push_back(p.y);
        mesh.vertices.push_back(ele);
        mesh.colors.push_back(color);
        mesh.triangles.push_back(triIndex);
    }

    static void addVertex(Mesh& mesh, const Vector3& vertex, int color, int triIndex)
    {
        addVertex(mesh, Vector2(vertex.x, vertex.z), vertex.y, color, triIndex);
    }

    void fillMesh(triangulateio* io, Mesh& mesh, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
    {
        int triStartIndex = static_cast<int>(mesh.vertices.size() / 3);

        bool hasTexture = apperanceOptions.textureScale > 0 && 
                          apperanceOptions.textureMap.width() > 0 &&  
                          apperanceOptions.textureMap.height() > 0;

        ensureMeshCapacity(mesh, static_cast<std::size_t>(io->numberofpoints), 
            static_cast<std::size_t>(io->numberoftriangles), hasTexture);

        auto toUv = createMapFunc(apperanceOptions);

        for (int i = 0; i < io->numberofpoints; i++) {
            double x = io->pointlist[i * 2 + 0];
            double y = io->pointlist[i * 2 + 1];

            double ele = geometryOptions.heightOffset + (geometryOptions.elevation > std::numeric_limits<double>::lowest()
                ? geometryOptions.elevation
                : eleProvider_.getElevation(y, x));

            // do no apply noise on boundaries
            if (io->pointmarkerlist != nullptr && io->pointmarkerlist[i] != 1)
                ele += NoiseUtils::perlin2D(x, y, geometryOptions.eleNoiseFreq);

            mesh.vertices.push_back(x);
            mesh.vertices.push_back(y);
            mesh.vertices.push_back(ele);

            int color = GradientUtils::getColor(apperanceOptions.gradient, x, y, apperanceOptions.colorNoiseFreq);
            mesh.colors.push_back(color);

            if (hasTexture) {
                auto uv = toUv(x, y);
                mesh.uvs.push_back(uv.x);
                mesh.uvs.push_back(uv.y);
            }
        }

        int first = geometryOptions.flipSide ? 2 : 1;
        int second = 0;
        int third = geometryOptions.flipSide ? 1 : 2;

        for (std::size_t i = 0; i < io->numberoftriangles; i++) {
            mesh.triangles.push_back(triStartIndex + io->trianglelist[i * io->numberofcorners + first]);
            mesh.triangles.push_back(triStartIndex + io->trianglelist[i * io->numberofcorners + second]);
            mesh.triangles.push_back(triStartIndex + io->trianglelist[i * io->numberofcorners + third]);
          }
    }

    static void ensureMeshCapacity(Mesh& mesh, std::size_t pointCount, std::size_t triCount, bool hasTexture)
    {
        mesh.vertices.reserve(mesh.vertices.size() + pointCount * 3 / 2);
        mesh.triangles.reserve(mesh.triangles.size() + triCount * 3);
        mesh.colors.reserve(mesh.colors.size() + pointCount);

        if (hasTexture)
            mesh.uvs.reserve(mesh.uvs.size() + pointCount);
    }

    /// Creates function which maps geocoordinate to texture coordinate.
    std::function<Vector2(double, double)> createMapFunc(const ApperanceOptions& apperanceOptions) const
    {
        // Precalculate mapping values
        double geoHeight = bbox.maxPoint.latitude - bbox.minPoint.latitude;
        double geoWidth = bbox.maxPoint.longitude - bbox.minPoint.longitude;
        double geoX = bbox.minPoint.longitude;
        double geoY = bbox.minPoint.latitude;
        
        double uvHeight = apperanceOptions.textureMap.height();
        double uvWidth = apperanceOptions.textureMap.width();
        double uvX = apperanceOptions.textureMap.xMin;
        double uvY = apperanceOptions.textureMap.yMin;

        double scale = apperanceOptions.textureScale;

        return [=](double x, double y) {
            double percentX = (x - geoX) / geoWidth * scale;
            double percentY = (y - geoY) / geoHeight * scale;
            return Vector2(uvX + uvWidth * percentX, uvY + uvHeight * percentY);
        };
    }

    const utymap::BoundingBox bbox;
    const ElevationProvider& eleProvider_;
};

MeshBuilder::MeshBuilder(const utymap::QuadKey& quadKey, const ElevationProvider& eleProvider) :
    pimpl_(utymap::utils::make_unique<MeshBuilderImpl>(quadKey, eleProvider))
{
}

MeshBuilder::~MeshBuilder() { }

void MeshBuilder::addPolygon(Mesh& mesh, Polygon& polygon, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
{
    pimpl_->addPolygon(mesh, polygon, geometryOptions, apperanceOptions);
}

void MeshBuilder::addPlane(Mesh& mesh, const Vector2& p1, const Vector2& p2, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
{
    pimpl_->addPlane(mesh, p1, p2, geometryOptions, apperanceOptions);
}

void MeshBuilder::addPlane(Mesh& mesh, const Vector2& p1, const Vector2& p2, double ele1, double ele2, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
{
    pimpl_->addPlane(mesh, p1, p2, ele1, ele2, geometryOptions, apperanceOptions);
}

void MeshBuilder::addTriangle(Mesh& mesh, const utymap::meshing::Vector3& v0, const utymap::meshing::Vector3& v1, const utymap::meshing::Vector3& v2, const GeometryOptions& geometryOptions, const ApperanceOptions& apperanceOptions) const
{
    pimpl_->addTriangle(mesh, v0, v1, v2, geometryOptions, apperanceOptions);
}
