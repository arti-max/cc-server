#pragma once

class Noise {
public:
    virtual ~Noise() = default;
    virtual double compute(double x, double y) = 0;
};
