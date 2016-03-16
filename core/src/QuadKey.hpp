#ifndef QUADKEY_HPP_DEFINED
#define QUADKEY_HPP_DEFINED

namespace utymap {

// Represents quadkey: a node of quadtree
struct QuadKey
{
    // Level of details (zoom).
    int levelOfDetail;
    // Tile x at given LOD
    int tileX;
    // Tile y at given LOD
    int tileY;
};

}
#endif // QUADKEY_HPP_DEFINED
