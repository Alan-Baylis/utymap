#ifndef CALLBACKS_HPP_DEFINED
#define CALLBACKS_HPP_DEFINED

#include <cstdint>

/// Callback which is called when mesh is built.
typedef void OnMeshBuilt(const char* name,                       // name 
                         const double* vertices, int vertexSize, // vertices (x, y, elevation)
                         const int* triangles, int triSize,      // triangle indices
                         const int* colors, int colorSize,       // rgba colors
                         const double* uvs, int uvSize,          // absolute texture uvs
                         const int* uvMap, int uvMapSize);       // map with info about used atlas and texture region

/// Callback which is called when element is loaded.
typedef void OnElementLoaded(std::uint64_t id,                       // element id
                             const char** tags, int tagsSize,        // tags
                             const double* vertices, int vertexSize, // vertices (x, y, elevation)
                             const char** style, int styleSize);     // mapcss styles (key, value)

/// Callback which is called when operation is completed.
typedef void OnError(const char* errorMessage);

#endif // CALLBACKS_HPP_DEFINED
