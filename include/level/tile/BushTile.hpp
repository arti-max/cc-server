#pragma once
#include "level/tile/Tile.hpp"
#include <cmath>

class Bush : public Tile {
public:
    Bush(int id, int textureId);
    ~Bush();
    void tick(Level* level, int x, int y, int z, Random* random) override;
    bool blocksLight() override;
    bool isSolid() override;
};