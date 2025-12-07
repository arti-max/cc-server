#include "level/tile/LiquidTile.hpp"

LiquidTile::~LiquidTile() {}

LiquidTile::LiquidTile(int id, int liquidType) : Tile::Tile(id) {
    this->liquidType = liquidType;
    if (this->liquidType == 2) {
        this->textureId = 30;
    } else {
        this->textureId = 14;
    }

    if (this->liquidType == 1) {
        this->tickRate = 40;
    } else {
        this->tickRate = 5;
    }

    this->tileId = id;
    this->calmTileId = id++;
    float dd = 0.1f;
    this->setShape(0.0f, 0.0f - dd, 0.0f, 1.0f, 1.0f - dd, 1.0f);
}

bool LiquidTile::tryFlow(Level* level, int x, int y, int z) {
    if (level->getTile(x, y, z) == 0 && this->liquidType == 1) {
        for (int xx = x - 2; xx <= x + 2; xx++) {
            for (int yy = y - 2; yy <= y + 2; yy++) {
                for (int zz = z - 2; zz <= z + 2; zz++) {
                   int tileId = level->getTile(xx, yy, zz);
                   if (tileId == Tile::sponge->id) { 
                       return false;
                   }
                }
            }
        }

        if (level->setTile(x, y, z, this->tileId)) {
            level->addToTickNextTick(x, y, z, this->tileId);
        }
    } else if (level->getTile(x, y, z) == 0 && this->liquidType == 2) {
        if (level->setTile(x, y, z, this->tileId)) {
            level->addToTickNextTick(x, y, z, this->tileId);
        }
    }
    return false;
}

float LiquidTile::getBrightness(Level* level, int x, int y, int z) {
    return this->liquidType == 2 ? 100.0f : level->getBrightness(x, y, z);
}

void LiquidTile::tick(Level* level, int x, int y, int z, Random* random) {
    bool hasChanged = false;
    int originalY = y;

    bool flowedDown;
    do {
        y--;
        if (level->getTile(x, y, z) != 0) break;
        
        flowedDown = level->setTile(x, y, z, this->tileId);
        if (flowedDown) hasChanged = true;
        
    } while (flowedDown && this->liquidType != 2);
    
    y = originalY;

    if (this->liquidType == 1 || !hasChanged) {
        tryFlow(level, x - 1, y, z);
        tryFlow(level, x + 1, y, z);
        tryFlow(level, x, y, z - 1);
        tryFlow(level, x, y, z + 1);
    }
}



bool LiquidTile::shouldRenderFace(Level* level, int x, int y, int z, int layer, int face) {
    if (x < 0 || y < 0 || z < 0 || x >= level->width || y >= level->depth) {
        return false;
    }
    if (layer != 1 && this->liquidType == 1) {
        return false;
    }
    
    int neighborId = level->getTile(x, y, z);
    
    if (neighborId != this->tileId && neighborId != this->calmTileId) {
        return face != 1 || level->getTile(x-1, y, z) != 0 && level->getTile(x+1, y, z != 0) && level->getTile(x, y, z-1) != 0 && level->getTile(x, y, z+1) != 0 ? Tile::shouldRenderFace(level, x, y, z, -1, face) : true;
    }
    
    return false;
}

bool LiquidTile::mayPick() {
    return false;
}

bool LiquidTile::blocksLight() {
    return true;
}

bool LiquidTile::isSolid() {
    return false;
}

int LiquidTile::getLiquidType() {
    return this->liquidType;
}

void LiquidTile::neighborChanged(Level* level, int x, int y, int z, int neighborTileId) {
    if (this->liquidType == 1 && (neighborTileId == Tile::lava->id || neighborTileId == Tile::calmLava->id)) {
        level->setTileNoUpdate(x, y, z, Tile::rock->id);
    }
    if (this->liquidType == 2 && (neighborTileId == Tile::water->id || neighborTileId == Tile::calmWater->id)) {
        level->setTileNoUpdate(x, y, z, Tile::rock->id);
    }

    level->addToTickNextTick(x, y, z, this->tileId);
}