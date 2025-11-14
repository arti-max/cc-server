#include "level/tile/FallingTile.hpp"

FallingTile::~FallingTile() {}

FallingTile::FallingTile(int id, int textureId) : Tile::Tile(id, textureId) {
    this->textureId = textureId;
    this->tileId = id;
}

void FallingTile::tryFall(Level* level, int x, int y, int z) {
    int finalY = y;

    while (level->getTile(x, finalY - 1, z) == 0 && finalY > 0) {
        finalY--;
    }

    if (finalY != y) {
        level->swap(x, y, z, x, finalY, z);
    }
}

void FallingTile::onBlockAdded(Level* level, int x, int y, int z) {
    tryFall(level, x, y, z);
}

void FallingTile::neighborChanged(Level* level, int x, int y, int z, int type) {
    tryFall(level, x, y, z);
}