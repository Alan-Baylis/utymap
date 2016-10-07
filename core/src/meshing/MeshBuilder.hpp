#ifndef MESHING_MESHBUILDER_HPP_DEFINED
#define MESHING_MESHBUILDER_HPP_DEFINED

#include "QuadKey.hpp"
#include "heightmap/ElevationProvider.hpp"
#include "mapcss/ColorGradient.hpp"
#include "mapcss/StyleSheet.hpp"
#include "meshing/MeshTypes.hpp"
#include "meshing/Polygon.hpp"

#include <memory>

namespace utymap { namespace meshing {

/// Provides the way to build mesh in 3D space.
class MeshBuilder final
{
public:

    struct GeometryOptions final
    {
        GeometryOptions(double area, double eleNoiseFreq, double elevation, double heightOffset, 
                        bool flipSide = false, bool hasBackSide = false, int segmentSplit = 0) :
            area(area),
            eleNoiseFreq(eleNoiseFreq),
            elevation(elevation),
            heightOffset(heightOffset),
            flipSide(flipSide),
            hasBackSide(hasBackSide),
            segmentSplit(segmentSplit)
        {
        }

        /// Max area of triangle in refined mesh.
        double area;
        
        /// Elevation noise frequency.
        double eleNoiseFreq;
        
        /// Fixed elevation. If specified elevation provider is not used.
        double elevation;
        
        /// Height offset.
        double heightOffset;
        
        /// If set then triangle side is flipped.
        bool flipSide;

        /// If set then backside should be generated too.
        bool hasBackSide;

        /// Flag indicating whether to suppress boundary segment splitting.
        ///     0 = split segments (default)
        ///     1 = no new vertices on the boundary
        ///     2 = prevent all segment splitting, including internal boundaries
        int segmentSplit;
    };

    struct AppearanceOptions final
    {
        /// Gradient used to calculate color at every vertex.
        const utymap::mapcss::ColorGradient& gradient;

        /// Color noise frequency.
        double colorNoiseFreq;

        /// Atlas id.
        std::uint16_t textureId;

        /// Texture coordinates map inside atlas.
        utymap::mapcss::TextureRegion textureRegion;

        /// Texture scale.
        double textureScale;

        AppearanceOptions(const utymap::mapcss::ColorGradient& gradient,
                          double colorNoiseFreq,
                          std::uint16_t textureId,
                          const utymap::mapcss::TextureRegion& textureRegion,
                          double textureScale) :
            gradient(gradient),
            colorNoiseFreq(colorNoiseFreq),
            textureId(textureId),
            textureRegion(std::move(textureRegion)),
            textureScale(textureScale)
        {
        }
    };

    /// Creates builder with given elevation provider.
    MeshBuilder(const utymap::QuadKey& quadKey, 
                const utymap::heightmap::ElevationProvider& eleProvider);

    ~MeshBuilder();

    /// Adds polygon to existing mesh using options provided.
    void addPolygon(Mesh& mesh, 
                    Polygon& polygon, 
                    const GeometryOptions& geometryOptions, 
                    const AppearanceOptions& appearanceOptions) const;

    /// Adds simple plane to existing mesh using options provided.
    void addPlane(Mesh& mesh, 
                  const Vector2& p1,
                  const Vector2& p2, 
                  const GeometryOptions& geometryOptions, 
                  const AppearanceOptions& appearanceOptions) const;

    /// Adds simple plane to existing mesh using elevation and options provided.
    void addPlane(Mesh& mesh, 
                  const Vector2& p1, 
                  const Vector2& p2, 
                  double ele1, 
                  double ele2, 
                  const GeometryOptions& geometryOptions, 
                  const AppearanceOptions& appearanceOptions) const;

    /// Adds triangle to mesh using options provided.
    void addTriangle(Mesh& mesh,
                     const utymap::meshing::Vector3& v0,
                     const utymap::meshing::Vector3& v1,
                     const utymap::meshing::Vector3& v2,
                     const utymap::meshing::Vector2& uv0,
                     const utymap::meshing::Vector2& uv1,
                     const utymap::meshing::Vector2& uv2,
                     const GeometryOptions& geometryOptions,
                     const AppearanceOptions& appearanceOptions) const;

    /// Writes texture mapping info into mesh. 
    /// NOTE we want to support tiling with atlas textures. It requires to write some 
    /// specific logic in shader. So, this code passes all information needed by it.
    /// This method exists because of performance optimization: you need to call it manually
    /// after all geometry related to one specific texture has been added.
    void writeTextureMappingInfo(Mesh& mesh, 
                                 const AppearanceOptions& appearanceOptions) const;

private:
    class MeshBuilderImpl;
    std::unique_ptr<MeshBuilderImpl> pimpl_;
};

}}

#endif // MESHING_MESHBUILDER_HPP_DEFINED
