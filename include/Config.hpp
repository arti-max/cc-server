#pragma once
#include <string>
#include <map>
#include <vector>
#include <mutex>

class Config {
public:
    // Загружает настройки из файла (или создает дефолтные)
    void load(const std::string& filename = "server.properties");
    
    // Сохраняет текущие настройки в файл
    void save();

    // Геттеры
    std::string getString(const std::string& key);
    int getInt(const std::string& key);
    bool getBool(const std::string& key);

    // Сеттер (если нужно менять настройки в рантайме)
    void set(const std::string& key, const std::string& value);

private:
    std::string filename;
    std::map<std::string, std::string> properties;
    std::mutex configMutex;

    // Порядок ключей для сохранения (как в Python)
    const std::vector<std::string> configOrder = {
        "server-name", "motd", "port", "public", "verify-names", 
        "max-players", "max-connections"
    };

    // Дефолтные значения
    const std::map<std::string, std::string> defaults = {
        {"server-name", "CrossCraft Server"},
        {"motd", "A CrossCraft Server"},
        {"port", "25565"},
        {"public", "true"},
        {"verify-names", "true"},
        {"max-players", "16"},
        {"max-connections", "3"}
    };

    // Вспомогательные функции для парсинга
    std::string trim(const std::string& str);
};
