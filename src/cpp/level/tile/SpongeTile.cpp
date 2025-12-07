#include "level/tile/SpongeTile.hpp"

SpongeTile::SpongeTile(int id) : Tile(id) {
    this->textureId = 48;
}

void SpongeTile::onBlockAdded(Level* level, int x, int y, int z) {
    checkSpongeEffect(level, x, y, z);
}

void SpongeTile::neighborChanged(Level* level, int x, int y, int z, int type) {
    checkSpongeEffect(level, x, y, z);
}

void SpongeTile::checkSpongeEffect(Level* level, int x, int y, int z) {
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            for (int dz = -2; dz <= 2; dz++) {
                if (level->getTile(x + dx, y + dy, z + dz) == Tile::water->id || level->getTile(x + dx, y + dy, z + dz) == Tile::calmWater->id) {
                    level->setTile(x + dx, y + dy, z + dz, 0);
                }
            }
        }
    }
}