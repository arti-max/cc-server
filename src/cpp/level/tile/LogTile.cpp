#include "level/tile/LogTile.hpp"

LogTile::LogTile(int id) : Tile(id) {
    this->textureId = 20;
}

LogTile::~LogTile() {}

int LogTile::getTexture(int face) {
    return face == 1 ? 21 : face == 0 ? 21 : 20;
}