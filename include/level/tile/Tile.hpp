#pragma once
#include <array>
#include "level/Level.hpp"
#include "util/Random.hpp"

class Tile {

private:

protected:
    float minX, minY, minZ, maxX, maxY, maxZ;
    virtual void setShape(float x0, float y0, float z0, float x1, float y1, float z1);
    virtual bool shouldRenderFace(Level* level, int x, int y, int z, int layer, int face);
    virtual int getTexture(int face);
public:
    Tile(int id);
    Tile (int id, int textureId);
    ~Tile();
    static std::array<Tile*, 256> tiles;
    static const Tile* empty;
    static const Tile* rock;
    static const Tile* grass;
    static const Tile* dirt;
    static const Tile* cobblestone;
    static const Tile* wood;
    static const Tile* bush;
    static const Tile* water;
    static const Tile* unbreakable;
    static const Tile* calmWater;
    static const Tile* lava;
    static const Tile* calmLava;
    static const Tile* gravel;
    static const Tile* sand;
    static const Tile* log;
    static const Tile* leaves;
    static const Tile* goldOre;
    static const Tile* ironOre;
    static const Tile* coalOre;
    static const Tile* sponge;
    static const Tile* glass;

    int textureId;
    int id;

    virtual void tick(Level* level, int x, int y, int z, Random* random);
    virtual bool mayPick();
    virtual bool blocksLight();
    virtual bool isSolid();
    virtual void neighborChanged(Level* level, int x, int y, int z, int type);
    virtual int getLiquidType();
    virtual bool isCalmLiquid();
    virtual void onBlockAdded(Level* level, int x, int y, int z);
    virtual float getBrightness(Level* level, int x, int y, int z);
};
