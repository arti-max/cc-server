#pragma once
#include <random>
#include <iostream>
#include <climits>

class Random {
private:
    std::mt19937 generator;
    
public:
    Random() : generator(std::random_device{}()) {}
    Random(int seed) : generator(seed) {}
    
    int nextInt() {
        std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
        return dist(generator);
    }
    
    int nextInt(int max) {
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(generator);
    }
    
    int nextInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max - 1);
        return dist(generator);
    }
    
    double nextDouble() {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(generator);
    }
    
    float nextFloat() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(generator);
    }
    
    bool nextBool() {
        std::uniform_int_distribution<int> dist(0, 1);
        return dist(generator) == 1;
    }
    
    static float random() {
        static Random staticRandom;
        return staticRandom.nextFloat();
    }
};
