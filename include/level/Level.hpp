#pragma once
#include <vector>
#include <string>
#include <set>
#include <deque>
#include <cmath>
#include <functional>
#include "util/Random.hpp"

class Server;
class Entity;

struct TickEntry {
    int x, y, z, tileId;
};

class Level {
private:
    static const int TILE_UPDATE_INTERVAL = 200;
    static const int MAX_RANDOM_TICKS_PER_FRAME = 1000;
    static const int multiplier = 1664525;
    static const int addend = 1013904223;
    
    Random* random;
    std::set<int> ticking;
    std::deque<TickEntry> tickNextTickList;
    std::vector<TickEntry> bannedTiles;
    static const int maxBits = 10;
    int randValue;
    int unprocessed = 0;
    
    void calcLightDepths(int x0, int z0, int x1, int z1);
    void neighborChanged(int x, int y, int z, int type);
    bool isLiquidTile(int tileId);
    bool isActiveLiquidTile(int tileId);
    int encodePosition(int x, int y, int z);
    void decodePosition(int code, int& x, int& y, int& z);

    std::function<void(int, int, int, int)> blockChangedListener;

public:
    int width, height, depth;
    int tickCount = 0;
    std::vector<uint8_t> blocks;
    std::vector<int> lightDepths;
    std::vector<Entity*> entities;
    std::string name;
    std::string creator;
    long long creationTime = 0;

    bool enableRandomTicks = true;


    Server* server = nullptr;

    int xSpawn;
    int ySpawn;
    int zSpawn;
    int rotSpawn;

    bool isRemote = false;

    Level();
    ~Level() = default;
    void generateCaves();
    void setData(int w, int d, int h, const std::vector<uint8_t>& blocks);
    bool save(const char* filename);
    bool load(const char* filename);
    const std::vector<uint8_t>& getBlocks();
    float getGroundLevel() const;
    float getWaterLevel() const;
    void tick();
    bool isTile(int x, int y, int z);
    bool isSolidTile(int x, int y, int z);
    bool isLightBlocker(int x, int y, int z);
    float getBrightness(int x, int y, int z);
    bool isLit(int x, int y, int z);
    int getTile(int x, int y, int z);
    bool setTile(int x, int y, int z, int type);
    bool setTileNoUpdate(int x, int y, int z, int type);
    void addTick(int x, int y, int z);
    void removeTick(int x, int y, int z);
    bool needsTick(int tileId);
    void addToTickNextTick(int x, int y, int z, int tileId);
    void swap(int x1, int y1, int z1, int x2, int y2, int z2);
    void initTransient();
    void findSpawn();
    void setSpawnPos(int x, int y, int z, int rot);
    int getHighestTile(int x, int z);
    void fillOcean(int x, int y, int z);
    void setBlockChangedListener(std::function<void(int, int, int, int)> listener);
    void addBanned(int x, int y, int z, int id);
    void removeBanned(int x, int y, int z, int id);
    bool isBanned(int x, int y, int z);
    bool hasWork() const {
        return !tickNextTickList.empty();
    }

private:
    void generateMap();
};
