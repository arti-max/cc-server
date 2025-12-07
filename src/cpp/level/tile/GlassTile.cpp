#include "level/tile/GlassTile.hpp"

GlassTile::GlassTile(int id) : TransparentTile(id, 49) {
}

bool GlassTile::shouldRenderFace(Level* level, int x, int y, int z, int layer, int face) {
    int neighborId = level->getTile(x, y, z);
    
    if (neighborId == this->id) {
        return false;
    }
    
    return TransparentTile::shouldRenderFace(level, x, y, z, layer, face);
}
