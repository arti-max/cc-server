#include "level/tile/TransparentTile.hpp"

TransparentTile::TransparentTile(int id, int textureId) : Tile(id) {
    this->textureId = textureId;
}

TransparentTile::~TransparentTile() {}

bool TransparentTile::isSolid() {
    return false;
}

bool TransparentTile::blocksLight() {
    return false;
}