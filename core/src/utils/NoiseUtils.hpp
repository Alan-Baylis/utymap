#ifndef UTILS_NOISEUTILS_HPP_DEFINED
#define UTILS_NOISEUTILS_HPP_DEFINED

#include "meshing/MeshTypes.hpp"

namespace utymap { namespace utils {

// Provides noise generation functions.
// Ported from C# code from here : http ://catlikecoding.com/unity/tutorials/noise/
class NoiseUtils
{

public:

    // Calculates perlin 2D noise.
    static double perlin2D(double x, double y, double frequency);

    // Calculates perlin 3D noise.
    static double perlin3D(double x, double y, double z, double freq);

private:

    static inline double dot(const utymap::meshing::Vector3& g, double x, double y, double z)
    {
        return g.x*x + g.y*y + g.z*z;
    }

    static inline double dot(const utymap::meshing::Vector2& g, double x, double y)
    {
        return g.x*x + g.y*y;
    }

    static inline double smooth(double t)
    {
        return t*t*t*(t*(t * 6 - 15) + 10);
    }

    static const int HashMask = 255;
    static const int GradientsMask2D = 7;
    static const int GradientsMask3D = 15;
    static const utymap::meshing::Vector2 Gradients2D[];
    static const utymap::meshing::Vector3 Gradients3D[];
    static const int Hash[];
};
}}

#endif // UTILS_NOISEUTILS_HPP_DEFINED
