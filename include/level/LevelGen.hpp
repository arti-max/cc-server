#pragma once

#include "level/LevelLoaderListener.hpp"
#include "level/Level.hpp"
#include "level/synth/Noise.hpp"
#include "util/Random.hpp"
#include <vector>
#include <cstdint>
#include <memory>

class LevelGen {
private:
    LevelLoaderListener* listener;
    Random random;

    int width = 0;
    int height = 0;
    int depth = 0;
    std::vector<uint8_t> blocks;
    std::vector<int> heightmap;

    void raise(std::vector<int>& map);
    void erode(std::vector<int>& map);
    void soil(std::vector<int>& map);
    void carve();
    void addOres(int tileId, int count, int abundance);
    void addWaterAndLava();
    void addFlowersAndMushrooms(const std::vector<int>& map);
    void addTrees(const std::vector<int>& map);
    void growSurface(const std::vector<int>& map);
    
    void floodFill(int x, int y, int z, uint8_t targetBlock);
    void addVeins(int tileId, int abundance);

public:
    explicit LevelGen(LevelLoaderListener* listener);
    ~LevelGen();

    void generateLevel(Level* level, const char* username, int w, int h, int d);
};
