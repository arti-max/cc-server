#pragma once

#include "level/synth/Noise.hpp"
#include "util/Random.hpp"
#include <vector>
#include <numeric>
#include <cmath>

class ImprovedNoise : public Noise {
private:
    std::vector<int> p;

    static double fade(double t) {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    static double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    static double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    explicit ImprovedNoise(Random& random) {
        p.resize(512);
        std::iota(p.begin(), p.begin() + 256, 0);

        for (int i = 0; i < 256; ++i) {
            int j = random.nextInt(256 - i) + i;
            int temp = p[i];
            p[i] = p[j];
            p[j] = temp;
        }

        for (int i = 0; i < 256; ++i) {
            p[i + 256] = p[i];
        }
    }

    double compute(double x, double y) override {
        double z = 0.0;

        int floorX = static_cast<int>(std::floor(x)) & 255;
        int floorY = static_cast<int>(std::floor(y)) & 255;
        int floorZ = static_cast<int>(std::floor(z)) & 255;

        double relX = x - std::floor(x);
        double relY = y - std::floor(y);
        double relZ = z - std::floor(z);

        double u = fade(relX);
        double v = fade(relY);
        double w = fade(relZ);

        int A = p[floorX] + floorY;
        int AA = p[A] + floorZ;
        int AB = p[A + 1] + floorZ;
        int B = p[floorX + 1] + floorY;
        int BA = p[B] + floorZ;
        int BB = p[B + 1] + floorZ;

        return lerp(v, lerp(u, grad(p[AA], relX, relY, relZ),
                               grad(p[BA], relX - 1.0, relY, relZ)),
                       lerp(u, grad(p[AB], relX, relY - 1.0, relZ),
                               grad(p[BB], relX - 1.0, relY - 1.0, relZ)));
    }
};
