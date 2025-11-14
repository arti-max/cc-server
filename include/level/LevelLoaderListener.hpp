#pragma once


class LevelLoaderListener {
public:
    virtual ~LevelLoaderListener() = default;

    virtual void beginLevelLoading(const char str[]) = 0;
    virtual void levelLoadUpdate(const char str[]) = 0;
    virtual void levelLoadProgress(int progress) = 0;
};