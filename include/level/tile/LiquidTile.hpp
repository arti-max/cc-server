#pragma once
#include "level/tile/Tile.hpp"

class LiquidTile : public Tile {
protected:
    int liquidType;
    int calmTileId;
    int tileId;
    int tickRate;

    bool shouldRenderFace(Level* level, int x, int y, int z, int layer, int face) override;
private:
    bool tryFlow(Level* level, int x, int y, int z);
public:
    LiquidTile(int id, int liquidType);
    ~LiquidTile();
    void tick(Level* level, int x, int y, int z, Random* random) override;
    void neighborChanged(Level* level, int x, int y, int z, int type) override;
    bool mayPick() override;
    bool blocksLight() override;
    bool isSolid() override;
    int getLiquidType() override;
    float getBrightness(Level* level, int x, int y, int z) override;
};