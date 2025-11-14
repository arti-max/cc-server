#include "level/tile/GrassTile.hpp"

GrassTile::~GrassTile() {}

GrassTile::GrassTile(int id) : Tile(id) {
    this->textureId = 4;
}

int GrassTile::getTexture(int face) {
    return face == 1 ? 0 : face == 0 ? 3 : 4;
}

void GrassTile::tick(Level* level, int x, int y, int z, Random* random) {
    if (level->isLit(x, y+1, z)) {
        for (int i = 0; i < 4; ++i) {
            int targetX = x + random->nextInt(3) - 1;
            int targetY = y + random->nextInt(5) - 3;
            int targetZ = z + random->nextInt(3) - 1;

            if (level->getTile(targetX, targetY, targetZ) == Tile::dirt->id && level->isLit(targetX, targetY+1, targetZ)) {
                level->setTile(targetX, targetY, targetZ, Tile::grass->id);
            }
        }
    } else {
        level->setTile(x, y, z, Tile::dirt->id);
    }
}