#include "level/tile/BushTile.hpp"

Bush::~Bush() {}

Bush::Bush(int id, int textureId) : Tile(id) {
    this->textureId = textureId;
}

void Bush::tick(Level* level, int x, int y, int z, Random* random) {
    int tileIdBelow = level->getTile(x, y-1, z);

    if (!level->isLit(x, y, z) || (tileIdBelow != Tile::grass->id && tileIdBelow != Tile::dirt->id)) {
        level->setTile(x, y, z, 0);
    }
}

bool Bush::blocksLight() {
    return false;
}

bool Bush::isSolid() {
    return false;
}