#pragma once
#include "level/tile/Tile.hpp"

class FallingTile : public Tile {
private:
    int tileId;
    int prevTileId = 0;
    static void tryFall(Level* level, int x, int y, int z);
public:
    FallingTile(int id, int textureId);
    ~FallingTile();
    void onBlockAdded(Level* level, int x, int y, int z) override;
    void neighborChanged(Level* level, int x, int y, int z, int type) override;
};