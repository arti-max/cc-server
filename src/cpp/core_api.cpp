#include "core_api.h"
#include "level/LevelGen.hpp"
#include "level/Level.hpp"
#include "Timer.hpp"

class WorldGeneratorWrapper {
public:
    LevelGen* generator;
    Level* level;
    Timer* timer;
    BlockChangedCallback onBlockChanged;
    
    WorldGeneratorWrapper() {
        class DummyListener : public LevelLoaderListener {
        private:
            std::string title;
            std::string status;
        public:
            void beginLevelLoading(const char* title) override {
                this->title = title;
            }
            void levelLoadUpdate(const char* status) override {
                this->status = status;
            }
            void levelLoadProgress(int progress) override {
                std::cout << "[LEVELGEN] " + this->title + " : " + this->status + " " + std::to_string(progress) + "%" << std::endl;
            }
        };
        
        generator = new LevelGen(new DummyListener());
        level = new Level();
        timer = new Timer(20.0f);

        level->setBlockChangedListener([this](int x, int y, int z, int type) {
            if (this->onBlockChanged) {
                this->onBlockChanged(x, y, z, type);
            }
        });
    }
    
    ~WorldGeneratorWrapper() {
        delete generator;
        delete level;
    }
};

extern "C" {

    DLLEXPORT void* create_world_generator() {
        return new WorldGeneratorWrapper();
    }

    DLLEXPORT WorldData generate_world(void* gen_ptr, int width, int height, int depth) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(gen_ptr);
        
        wrapper->generator->generateLevel(wrapper->level, "server", width, height, depth);
        
        const std::vector<uint8_t>& block_vector = wrapper->level->getBlocks();
        size_t data_size = block_vector.size();
        
        uint8_t* data_copy = new uint8_t[data_size];
        std::copy(block_vector.begin(), block_vector.end(), data_copy);
        
        return { data_copy, data_size };
    }

    DLLEXPORT void free_world_data(WorldData data) {
        delete[] data.blocks;
    }

    DLLEXPORT void destroy_world_generator(void* gen_ptr) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(gen_ptr);
        delete wrapper;
    }

    DLLEXPORT void set_block_changed_callback(void* world_ptr, BlockChangedCallback callback) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(world_ptr);
        wrapper->onBlockChanged = callback;
    }

    DLLEXPORT void set_block(void* world_ptr, int x, int y, int z, int blockType) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(world_ptr);
        wrapper->level->setTile(x, y, z, blockType);
    }

    DLLEXPORT void tick_world(void* world_ptr) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(world_ptr);
        wrapper->timer->advanceTime();
        for (int i = 0; i < wrapper->timer->ticks; ++i) {
            wrapper->level->tick();
        }
    }

    DLLEXPORT bool save_world_to_file(void* world_ptr, const char* filename) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(world_ptr);
        return wrapper->level->save(filename);
    }

    DLLEXPORT bool load_world_from_file(void* world_ptr, const char* filename) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(world_ptr);
        return wrapper->level->load(filename);
    }

    DLLEXPORT WorldData get_world_data(void* world_ptr) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(world_ptr);
        if (!wrapper || !wrapper->level) {
            return { nullptr, 0 };
        }
        
        const std::vector<uint8_t>& block_vector = wrapper->level->getBlocks();
        size_t data_size = block_vector.size();
        
        uint8_t* data_copy = new uint8_t[data_size];
        std::copy(block_vector.begin(), block_vector.end(), data_copy);
        
        return { data_copy, data_size };
    }

    DLLEXPORT SpawnPosition get_spawn_position(void* world_ptr) {
        WorldGeneratorWrapper* wrapper = static_cast<WorldGeneratorWrapper*>(world_ptr);
        if (wrapper && wrapper->level) {
            return {
                wrapper->level->xSpawn,
                wrapper->level->ySpawn,
                wrapper->level->zSpawn,
                wrapper->level->rotSpawn
            };
        }
        return {0, 0, 0, 0};
    }
}
