#pragma once
#include "level/tile/Tile.hpp"

class LogTile : public Tile {
protected:
    int getTexture(int face) override;
public:
    LogTile(int id);
    ~LogTile();
};