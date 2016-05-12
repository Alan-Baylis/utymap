#ifndef SCENE_BUILDINGS_FACADES_FACADEBUILDER_HPP_DEFINED
#define SCENE_BUILDINGS_FACADES_FACADEBUILDER_HPP_DEFINED

#include "builders/BuilderContext.hpp"
#include "builders/MeshContext.hpp"
#include "meshing/Polygon.hpp"
#include "mapcss/ColorGradient.hpp"
#include "utils/MapCssUtils.hpp"

#include <vector>

namespace utymap { namespace builders {

// Specifies base roof builder functionality.
class FacadeBuilder
{
 public:
  FacadeBuilder(const utymap::builders::BuilderContext& builderContext,
                utymap::builders::MeshContext& meshContext)
      : builderContext_(builderContext), meshContext_(meshContext),
        minHeight_(0), height_(0), colorNoiseFreq_(0)
  {
  }

  // Sets facade height.
  inline FacadeBuilder& setHeight(double height) { height_ = height; return *this; }

  // Sets height above ground level.
  inline FacadeBuilder& setMinHeight(double minHeight) { minHeight_ = minHeight; return *this; }

  // Sets color freq.
  inline FacadeBuilder& setColorNoise(double freq) { colorNoiseFreq_ = freq; return *this; }

  // Gets gradient.
  inline const utymap::mapcss::ColorGradient& getColorGradient()
  {
    auto gradient = utymap::utils::getString(FacadeColorKey,
                                             builderContext_.stringTable,
                                             meshContext_.style);
    return builderContext_.styleProvider.getGradient(*gradient);
  }

  // Builds roof from polygon.
  virtual void build(utymap::meshing::Polygon& polygon) = 0;

 protected:
  const std::string FacadeColorKey = "facade-color";

  const utymap::builders::BuilderContext& builderContext_;
  utymap::builders::MeshContext& meshContext_;
  double height_, minHeight_, colorNoiseFreq_;
};

}}

#endif // SCENE_BUILDINGS_FACADES_FACADEBUILDER_HPP_DEFINED
