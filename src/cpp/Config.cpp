#include "Config.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <ctime>
#include "Logger.hpp"

void Config::load(const std::string& fname) {
    this->filename = fname;
    std::lock_guard<std::mutex> lock(configMutex);

    // Сначала заполняем дефолтными значениями
    properties = defaults;

    std::ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Удаляем пробелы
            line = trim(line);
            
            // Пропускаем пустые строки и комментарии
            if (line.empty() || line[0] == '#') continue;

            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = trim(line.substr(0, delimiterPos));
                std::string value = trim(line.substr(delimiterPos + 1));
                properties[key] = value;
            }
        }
        file.close();
        Logger::logf(PREFIX_CC, "Loaded %s\n", filename.c_str());
    } else {
        Logger::logf(PREFIX_WARNING, "File %s not found, creating default.\n", filename.c_str());
    }

    // Сохраняем обратно, чтобы обновить файл (добавить новые ключи, если их нет)
    // Но делать это нужно аккуратно, в Python это делалось сразу.
    // Вызовем save() без блокировки, так как мы уже держим lock
    // (пришлось бы выносить логику save в private метод, но для простоты скопируем суть)
    
    // Лучше просто сохранить:
    // save() нельзя вызывать тут из-за deadlock рекурсии mutex, 
    // поэтому просто пишем логику сохранения ниже или убираем mutex из load/save если вызовы однопоточные при старте.
    
    // В данном случае просто запишем файл вручную, чтобы структура сохранилась
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << "#CrossCraft server properties\n";
        
        // Время (как в Python os.path.getmtime, но тут просто текущее)
        std::time_t now = std::time(nullptr);
        char buf[100];
        // Форматируем время или просто пишем timestamp
        outfile << "#" << now << "\n"; 
        
        // 1. Пишем ключи в заданном порядке
        for (const auto& key : configOrder) {
            if (properties.count(key)) {
                outfile << key << "=" << properties[key] << "\n";
            }
        }

        // 2. Пишем остальные ключи, которых нет в configOrder
        for (const auto& [key, value] : properties) {
            bool inOrder = false;
            for (const auto& orderKey : configOrder) {
                if (key == orderKey) {
                    inOrder = true;
                    break;
                }
            }
            if (!inOrder) {
                outfile << key << "=" << value << "\n";
            }
        }
        outfile.close();
        Logger::logf(PREFIX_CC, "Saved %s\n", filename.c_str());
    }
}

void Config::save() {
    std::lock_guard<std::mutex> lock(configMutex);
    // Логика сохранения дублируется из конца load, но для рантайм-сохранений
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        Logger::logf(PREFIX_ERROR, "Failed to save %s\n", filename.c_str());
        return;
    }

    outfile << "#CrossCraft server properties\n";
    for (const auto& key : configOrder) {
        if (properties.count(key)) {
            outfile << key << "=" << properties[key] << "\n";
        }
    }
    for (const auto& [key, value] : properties) {
        bool inOrder = false;
        for (const auto& orderKey : configOrder) {
            if (key == orderKey) {
                inOrder = true;
                break;
            }
        }
        if (!inOrder) {
            outfile << key << "=" << value << "\n";
        }
    }
    Logger::logf(PREFIX_CC, "Saved %s\n", filename.c_str());
}

std::string Config::getString(const std::string& key) {
    std::lock_guard<std::mutex> lock(configMutex);
    if (properties.count(key)) return properties[key];
    if (defaults.count(key)) return defaults.at(key);
    return "";
}

int Config::getInt(const std::string& key) {
    std::string val = getString(key);
    try {
        return std::stoi(val);
    } catch (...) {
        return 0; // Или дефолтное значение
    }
}

bool Config::getBool(const std::string& key) {
    std::string val = getString(key);
    // Приводим к нижнему регистру для сравнения
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    return val == "true";
}

void Config::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(configMutex);
    properties[key] = value;
}

std::string Config::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}
