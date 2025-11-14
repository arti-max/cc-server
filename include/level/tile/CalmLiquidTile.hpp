#pragma once
#include "level/tile/LiquidTile.hpp"


class CalmLiquidTile : public LiquidTile {
public:
    CalmLiquidTile(int id, int liquidType);
    ~CalmLiquidTile();
    bool isCalmLiquid() override;
    void tick(Level* level, int x, int y, int z, Random* random) override;
    void neighborChanged(Level* level, int x, int y, int z, int type) override;
};