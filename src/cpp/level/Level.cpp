#include "level/Level.hpp"
#include "level/tile/Tile.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>

Level::Level() :  random() {
    
    this->random = new Random();
    randValue = random->nextInt();
    unprocessed = 0;
}

void Level::initTransient() {
    this->lightDepths.assign(this->width * this->height, 0);
    this->calcLightDepths(0, 0, this->width, this->height);
    this->tickNextTickList.clear();

    if (this->xSpawn == 0 && this->ySpawn == 0 && this->zSpawn == 0) { 
        findSpawn(); 
    }
}

float Level::getGroundLevel() const {
    return (float)(this->depth / 2 - 2);
}

float Level::getWaterLevel() const {
    return (float)(this->depth / 2);
}


void Level::findSpawn() {
    Random* rnd = new Random();
    int i = 0;

    int x;
    int y;
    int z;
    do {
        ++i;
        x = rnd->nextInt(this->width / 2) + this->width / 4;
        z = rnd->nextInt(this->height / 2) + this->height / 4;
        y = this->getHighestTile(x, y) + 1;
        if (i == 1000) {
            this->xSpawn = x;
            this->ySpawn = -100;
            this->zSpawn = z;
            return;
        }

    } while((float)y <= this->getGroundLevel());

    this->xSpawn = x;
    this->ySpawn = y;
    this->zSpawn = z;
}

int Level::getHighestTile(int x, int z) {
    int y;
    for (y = this->depth; (this->getTile(x, y -1, z) == 0 || Tile::tiles[this->getTile(x, y-1, z)]->getLiquidType() != 0) && y > 0; --y) {
    }
    return y;
}

void Level::setSpawnPos(int x, int y, int z, int rot) {
    this->xSpawn = x;
    this->ySpawn = y;
    this->zSpawn = z;
    this->rotSpawn = rot;
}

void Level::setData(int w, int d, int h, const std::vector<uint8_t>& newBlocks) {
    width = w;
    height = h;
    depth = d;
    blocks = newBlocks;
    lightDepths.resize(w * h);
    calcLightDepths(0, 0, w, h);

    this->tickNextTickList.clear();
    this->findSpawn();
}

void Level::calcLightDepths(int x0, int z0, int x1, int z1) {
    for (int x = x0; x < x0 + x1; ++x) {
        for (int z = z0; z < z0 + z1; ++z) {
            
            int oldDepth = lightDepths[x + z * this->width];
            
            int depth = this->depth - 1;
            while (depth > 0 && !this->isLightBlocker(x, depth, z)) {
                --depth;
            }
            
            lightDepths[x + z * width] = depth + 1;
            
            if (oldDepth != depth) {
                int yl0 = oldDepth < depth ? oldDepth : depth;
                int yl1 = oldDepth > depth ? oldDepth : depth;
            }
        }
    }
}

void Level::tick() {
    tickCount++;

    if (tickCount % 5 == 0) {
        int count = tickNextTickList.size();
        if (count > 1000) count = 1000;

        for (int i = 0; i < count; ++i) {
            TickEntry entry = tickNextTickList.front();
            tickNextTickList.pop_front();

            if (getTile(entry.x, entry.y, entry.z) == entry.tileId) {
                Tile::tiles[entry.tileId]->tick(this, entry.x, entry.y, entry.z, random);
            }
        }
    }


    unprocessed += width * height * depth;
    int ticks = unprocessed / TILE_UPDATE_INTERVAL;
    unprocessed -= ticks * TILE_UPDATE_INTERVAL;
    
    for (int i = 0; i < ticks; ++i) {
        int x = random->nextInt(width);
        int y = random->nextInt(depth);
        int z = random->nextInt(height);
        
        int id = getTile(x, y, z);
        if (id != 0) {
            Tile* tile = Tile::tiles[id];
            if (tile) {
                tile->tick(this, x, y, z, random);
            }
        }
    }
}

bool Level::isTile(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= width || y >= depth || z >= height) {
        return false;
    }
    int index = (y * height + z) * width + x;
    return blocks[index] != 0;
}

bool Level::isSolidTile(int x, int y, int z) {
    Tile* tile = Tile::tiles[getTile(x, y, z)];
    return tile != nullptr && tile->isSolid();
}

bool Level::isLightBlocker(int x, int y, int z) {
    Tile* tile = Tile::tiles[getTile(x, y, z)];
    return tile != nullptr && tile->blocksLight();
}

float Level::getBrightness(int x, int y, int z) {
    return this->isLit(x, y, z) ? 1.0f : 0.5f;
}

bool Level::isLit(int x, int y, int z) {
    if (x >= 0 && y >= 0 && z >= 0 && x < width && y < depth && z < height) {
        return y >= lightDepths[x + z * width];
    }
    return true;
}

int Level::getTile(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= width || y >= depth || z >= height) {
        return 0;
    }
    int index = (y * height + z) * width + x;
    return blocks[index];
}

bool Level::setTile(int x, int y, int z, int type) {
    if (x < 0 || y < 0 || z < 0 || x >= width || y >= depth || z >= height) {
        return false;
    }

    if (type == Tile::water->id || type == Tile::calmWater->id) {
        for (int xx = x - 2; xx <= x + 2; xx++) {
            for (int yy = y - 2; yy <= y + 2; yy++) {
                for (int zz = z - 2; zz <= z + 2; zz++) {
                    if (this->getTile(xx, yy, zz) == 19) {
                        return false; 
                    }
                }
            }
        }
    }

    if (type == 0) {
        if (this->isBanned(x, y, z)) {
            this->removeBanned(x, y, z, type);
        }
        if (x == 0 || z == 0 || x == width - 1 || z == height - 1) {
            if (y >= this->getGroundLevel() && y < this->getWaterLevel()) {
                type = Tile::water->id;
            }
        }
    }
        
    int index = (y * height + z) * width + x;
    int oldType = blocks[index];
    
    if (type == oldType) {
        return false;
    }
    
    blocks[index] = static_cast<uint8_t>(type);
    
    if (type > 0 && Tile::tiles[type] != nullptr) {
        Tile::tiles[type]->onBlockAdded(this, x, y, z);
    }
    
    neighborChanged(x - 1, y, z, type);
    neighborChanged(x + 1, y, z, type);
    neighborChanged(x, y - 1, z, type);
    neighborChanged(x, y + 1, z, type);
    neighborChanged(x, y, z - 1, type);
    neighborChanged(x, y, z + 1, type);
    
    calcLightDepths(x, z, 1, 1);
    
    if (blockChangedListener) {
        blockChangedListener(x, y, z, type);
    }
    
    return true;
}

bool Level::setTileNoUpdate(int x, int y, int z, int type) {
    if (x >= 0 && y >= 0 && z >= 0 && x < width && y < depth && z < height) {
        int index = (y * height + z) * width + x;
        if (type == blocks[index]) {
            return false;
        }
        blocks[index] = static_cast<uint8_t>(type);

        if (blockChangedListener) {
            blockChangedListener(x, y, z, type);
        }

        return true;
    }
    return false;
}

void Level::swap(int x1, int y1, int z1, int x2, int y2, int z2) {
    int tile1 = getTile(x1, y1, z1);
    int tile2 = getTile(x2, y2, z2);

    setTileNoUpdate(x1, y1, z1, tile2);
    setTileNoUpdate(x2, y2, z2, tile1);

    neighborChanged(x1 - 1, y1, z1, tile2);
    neighborChanged(x1 + 1, y1, z1, tile2);
    neighborChanged(x1, y1 - 1, z1, tile2);
    neighborChanged(x1, y1 + 1, z1, tile2);
    neighborChanged(x1, y1, z1 - 1, tile2);
    neighborChanged(x1, y1, z1 + 1, tile2);

    neighborChanged(x2 - 1, y2, z2, tile1);
    neighborChanged(x2 + 1, y2, z2, tile1);
    neighborChanged(x2, y2 - 1, z2, tile1);
    neighborChanged(x2, y2 + 1, z2, tile1);
    neighborChanged(x2, y2, z2 - 1, tile1);
    neighborChanged(x2, y2, z2 + 1, tile1);
}

void Level::neighborChanged(int x, int y, int z, int type) {
    if (x >= 0 && y >= 0 && z >= 0 && x < width && y < depth && z < height) {
        Tile* tile = Tile::tiles[getTile(x, y, z)];
        if (tile != nullptr) {
            tile->neighborChanged(this, x, y, z, type);
        }
    }
}

bool Level::isLiquidTile(int tileId) {
    return 
        tileId == Tile::water->id || 
        tileId == Tile::calmWater->id || 
        tileId == Tile::lava->id || 
        tileId == Tile::calmLava->id;
}

bool Level::isActiveLiquidTile(int tileId) {
    return tileId == Tile::water->id || tileId == Tile::lava->id;
}

int Level::encodePosition(int x, int y, int z) {
    return (z << (maxBits * 2)) | (y << maxBits) | x;
}

void Level::decodePosition(int code, int& x, int& y, int& z) {
    int mask = (1 << maxBits) - 1;
    x = code & mask;
    y = (code >> maxBits) & mask;
    z = (code >> (maxBits * 2)) & mask;
}

void Level::addTick(int x, int y, int z) {
    ticking.insert(encodePosition(x, y, z));
}

void Level::removeTick(int x, int y, int z) {
    ticking.erase(encodePosition(x, y, z));
}

void Level::addToTickNextTick(int x, int y, int z, int tileId) {
    this->tickNextTickList.push_back({x, y, z, tileId});
}

bool Level::needsTick(int tileId) {
    return 
        tileId == Tile::water->id   ||
        tileId == Tile::lava->id
        ;
}

const std::vector<uint8_t>& Level::getBlocks() {
    return this->blocks;
}

void Level::setBlockChangedListener(std::function<void(int, int, int, int)> listener) {
    this->blockChangedListener = listener;
}

bool Level::save(const char* filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing." << std::endl;
        return false;
    }

    uint32_t magic = 0xDEADBEEF;
    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    file.write(reinterpret_cast<const char*>(&this->width), sizeof(this->width));
    file.write(reinterpret_cast<const char*>(&this->height), sizeof(this->height));
    file.write(reinterpret_cast<const char*>(&this->depth), sizeof(this->depth));

    file.write(reinterpret_cast<const char*>(&this->xSpawn), sizeof(this->xSpawn));
    file.write(reinterpret_cast<const char*>(&this->ySpawn), sizeof(this->ySpawn));
    file.write(reinterpret_cast<const char*>(&this->zSpawn), sizeof(this->zSpawn));
    file.write(reinterpret_cast<const char*>(&this->rotSpawn), sizeof(this->rotSpawn));

    file.write(reinterpret_cast<const char*>(this->blocks.data()), this->blocks.size());

    file.close();
    return true;
}

bool Level::load(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "File " << filename << " not found for reading." << std::endl;
        return false;
    }

    uint32_t magic, version;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != 0xDEADBEEF || version != 1) {
        std::cerr << "Invalid level file format or version." << std::endl;
        file.close();
        return false;
    }

    file.read(reinterpret_cast<char*>(&this->width), sizeof(this->width));
    file.read(reinterpret_cast<char*>(&this->height), sizeof(this->height));
    file.read(reinterpret_cast<char*>(&this->depth), sizeof(this->depth));
    
    file.read(reinterpret_cast<char*>(&this->xSpawn), sizeof(this->xSpawn));
    file.read(reinterpret_cast<char*>(&this->ySpawn), sizeof(this->ySpawn));
    file.read(reinterpret_cast<char*>(&this->zSpawn), sizeof(this->zSpawn));
    file.read(reinterpret_cast<char*>(&this->rotSpawn), sizeof(this->rotSpawn));

    size_t blockSize = this->width * this->height * this->depth;
    this->blocks.resize(blockSize);
    file.read(reinterpret_cast<char*>(this->blocks.data()), blockSize);
    
    file.close();
    
    this->initTransient();
    
    return true;
}

void Level::addBanned(int x, int y, int z, int id) {
    this->bannedTiles.push_back({x, y, z, id});
}

void Level::removeBanned(int x, int y, int z, int id) {
    auto it = this->bannedTiles.begin();
    while (it != this->bannedTiles.end()) {
        if (it->x == x && it->y == y && it->z == z && it->tileId == id) {
            it = this->bannedTiles.erase(it);
            return; 
        } else {
            ++it;
        }
    }
}

bool Level::isBanned(int x, int y, int z) {
    for (TickEntry tile : this->bannedTiles) {
        if (tile.x == x && tile.y == y && tile.z == z) {
            return true;
        }
    }
    return false;
}