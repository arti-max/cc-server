#pragma once
#include "level/tile/Tile.hpp"

class GrassTile : public Tile {
protected:
    int getTexture(int face) override;
public:
    GrassTile(int id);
    ~GrassTile();
    void tick(Level* level, int x, int y, int z, Random* random) override;
};