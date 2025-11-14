#pragma once

#include "level/synth/Noise.hpp"
#include "level/synth/ImprovedNoise.hpp"
#include "util/Random.hpp"
#include <vector>
#include <memory>

class OctaveNoise : public Noise {
private:
    std::vector<std::unique_ptr<ImprovedNoise>> noises;
    int octaveCount;

public:
    OctaveNoise(Random& random, int octaves) : octaveCount(octaves) {
        noises.reserve(octaveCount);
        for (int i = 0; i < octaveCount; ++i) {
            noises.push_back(std::make_unique<ImprovedNoise>(random));
        }
    }

    double compute(double x, double y) override {
        double total = 0.0;
        double amplitude = 1.0;

        for (int i = 0; i < octaveCount; ++i) {
            total += noises[i]->compute(x / amplitude, y / amplitude) * amplitude;
            amplitude *= 2.0;
        }

        return total;
    }
};
