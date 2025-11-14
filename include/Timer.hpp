#pragma once
#include <chrono>

class Timer {
private:
    static const long long NS_PER_SECOND = 1000000000LL;
    static const long long MAX_NS_PER_UPDATE = 1000000000LL;
    static const int MAX_TICKS_PER_UPDATE = 100;
    
    float ticksPerSecond;
    std::chrono::high_resolution_clock::time_point lastTime;

public:
    int ticks = 0;
    float partialTicks = 0.0f;
    float timeScale = 1.0f;
    float fps = 0.0f;
    float passedTime = 0.0f;

    Timer(float ticksPerSecond);
    void advanceTime();
    
private:
    long long getCurrentTimeNanos();
};
