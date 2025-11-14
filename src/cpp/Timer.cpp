#include "Timer.hpp"
#include <algorithm>

Timer::Timer(float ticksPerSecond) 
    : ticksPerSecond(ticksPerSecond) {
    lastTime = std::chrono::high_resolution_clock::now();
}

void Timer::advanceTime() {
    auto now = std::chrono::high_resolution_clock::now();
    
    auto duration = now - lastTime;
    long long passedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    
    lastTime = now;
    
    if (passedNs < 0LL) {
        passedNs = 0LL;
    }
    
    if (passedNs > MAX_NS_PER_UPDATE) {
        passedNs = MAX_NS_PER_UPDATE;
    }
    
    if (passedNs > 0) {
        fps = static_cast<float>(NS_PER_SECOND) / static_cast<float>(passedNs);
    } else {
        fps = 0.0f;
    }
    
    passedTime += static_cast<float>(passedNs) * timeScale * ticksPerSecond / 1.0e9f;
    
    ticks = static_cast<int>(passedTime);
    
    if (ticks > MAX_TICKS_PER_UPDATE) {
        ticks = MAX_TICKS_PER_UPDATE;
    }
    
    passedTime -= static_cast<float>(ticks);
    partialTicks = passedTime;
}

long long Timer::getCurrentTimeNanos() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}
