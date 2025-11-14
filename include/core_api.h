#pragma once
#include <cstddef>
#include <cstdint>

#ifdef _WIN32
    #define DLLEXPORT __declspec(dllexport)
#else
    #define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*BlockChangedCallback)(int x, int y, int z, int blockType);

struct WorldData {
    uint8_t* blocks;
    size_t size;
};

struct SpawnPosition {
    int x, y, z, rot;
};

DLLEXPORT void* create_world_generator();
DLLEXPORT WorldData generate_world(void* generator, int width, int height, int depth);
DLLEXPORT void free_world_data(WorldData data);
DLLEXPORT void destroy_world_generator(void* generator);
DLLEXPORT void set_block_changed_callback(void* world_ptr, BlockChangedCallback callback);
DLLEXPORT void tick_world(void* world_ptr);
DLLEXPORT void set_block(void* world_ptr, int x, int y, int z, int blockType);
DLLEXPORT bool save_world_to_file(void* world_ptr, const char* filename);
DLLEXPORT bool load_world_from_file(void* world_ptr, const char* filename);
DLLEXPORT WorldData get_world_data(void* world_ptr);
DLLEXPORT SpawnPosition get_spawn_position(void* world_ptr);

#ifdef __cplusplus
}
#endif
