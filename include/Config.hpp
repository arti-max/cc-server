#pragma once
#include <string>
#include <map>
#include <vector>
#include <mutex>

class Config {
public:
    void load(const std::string& filename = "server.properties");
    
    void save();

    std::string getString(const std::string& key);
    int getInt(const std::string& key);
    bool getBool(const std::string& key);

    void set(const std::string& key, const std::string& value);

private:
    std::string filename;
    std::map<std::string, std::string> properties;
    std::mutex configMutex;

    // Save key order
    const std::vector<std::string> configOrder = {
        "server-name", "motd", "port", "public", "verify-names", 
        "max-players", "max-connections"
    };

    // Default
    const std::map<std::string, std::string> defaults = {
        {"server-name", "CrossCraft Server"},
        {"motd", "A CrossCraft Server"},
        {"port", "25565"},
        {"public", "true"},
        {"verify-names", "true"},
        {"max-players", "16"},
        {"max-connections", "3"}
    };

    std::string trim(const std::string& str);
};
