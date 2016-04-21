#ifndef UTILS_GRADIENTUTILS_HPP_DEFINED
#define UTILS_GRADIENTUTILS_HPP_DEFINED

#include "mapcss/Color.hpp"
#include "mapcss/ColorGradient.hpp"
#include "utils/NoiseUtils.hpp"

#include <cstdint>
#include <string>
#include <regex>
#include <unordered_map>

namespace utymap { namespace utils {

// Provides the way to work with color gradient and color data.
class GradientUtils
{
public:

    // Parses color from string.
    static utymap::mapcss::Color parseColor(const std::string& colorStr);

    // Parses color gradient from string.
    static utymap::mapcss::ColorGradient parseGradient(const std::string& gradientStr);

    // Gets color for specific coordinate using coherent noise function
    static inline utymap::mapcss::Color getColor(const utymap::mapcss::ColorGradient& gradient,
                                                 double x, double y, double noise)
    {
        double colorTime = (NoiseUtils::perlin2D(x, y, noise) + 1) / 2;
        return gradient.evaluate(colorTime);
    }

private:
    static const std::regex gradientRegEx;
};

}}

#endif // UTILS_GRADIENTUTILS_HPP_DEFINED
