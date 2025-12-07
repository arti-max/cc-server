#pragma once
#include "level/tile/TransparentTile.hpp"

class GlassTile : public TransparentTile {
public:
    GlassTile(int id);
    bool shouldRenderFace(Level* level, int x, int y, int z, int layer, int face) override;
};