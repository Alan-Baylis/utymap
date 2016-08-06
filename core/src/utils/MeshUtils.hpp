#ifndef UTILS_MESHUTILS_HPP_DEFINED
#define UTILS_MESHUTILS_HPP_DEFINED

#include "meshing/MeshTypes.hpp"

namespace utymap { namespace utils {

    // Copies mesh into existing one adjusting position.
    void copyMesh(const utymap::meshing::Vector3 position, const utymap::meshing::Mesh& source, utymap::meshing::Mesh& destination)
    {
        int startIndex = static_cast<int>(destination.vertices.size() / 3);

        // copy adjusted vertices
        for (std::size_t i = 0; i < source.vertices.size();) {
            destination.vertices.push_back(source.vertices[i++] + position.x);
            destination.vertices.push_back(source.vertices[i++] + position.z);
            destination.vertices.push_back(source.vertices[i++] + position.y);
        }

        // copy adjusted triangles
        std::transform(source.triangles.begin(), source.triangles.end(), std::back_inserter(destination.triangles), [&](int value) {
            return value + startIndex;
        });

        // copy colors
        std::copy(source.colors.begin(), source.colors.end(), std::back_inserter(destination.colors));
    }

}}

#endif // UTILS_GEOUTILS_HPP_DEFINED