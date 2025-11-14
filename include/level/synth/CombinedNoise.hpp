#pragma once

#include "level/synth/Noise.hpp"
#include <memory>
#include <utility>

class CombinedNoise : public Noise {
private:
    std::unique_ptr<Noise> noise1;
    std::unique_ptr<Noise> noise2;

public:
    CombinedNoise(std::unique_ptr<Noise> n1, std::unique_ptr<Noise> n2)
        : noise1(std::move(n1)), noise2(std::move(n2)) {}

    double compute(double x, double y) override {
        return noise1->compute(x + noise2->compute(x, y), y);
    }
};
