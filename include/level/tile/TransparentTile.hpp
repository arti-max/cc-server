#pragma once
#include "level/tile/Tile.hpp"

class TransparentTile : public Tile {
public:
    TransparentTile(int id, int textureId);
    ~TransparentTile();
    bool isSolid() override;
    bool blocksLight() override;
};