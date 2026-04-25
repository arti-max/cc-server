#include "AdminSystem.hpp"
#include "Server.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <ctime>

AdminSystem::AdminSystem(Server* server) : server(server) {
    this->loadConfigs();
    this->saveConfigs();
}
AdminSystem::~AdminSystem() {}

void AdminSystem::loadConfigs() {
    std::lock_guard<std::mutex> lock(ASmutex);

    // admins.txt
    std::ifstream file(adminTxt);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);

            if (line.empty() || line[0] == '#') continue;

            admins.push_back(line);
        }
        file.close();
        Logger::logf(PREFIX_CC, "Loaded %s\n", adminTxt.c_str());
    } else {
        Logger::logf(PREFIX_WARNING, "File %s not found, creating default...\n", adminTxt.c_str());
    }

    // banned.txt
    std::ifstream file2(bansTxt);
    if (file2.is_open()) {
        std::string line;
        while (std::getline(file2, line)) {
            line = trim(line);

            if (line.empty() || line[0] == '#') continue;

            bans.push_back(line);
        }
        file2.close();
        Logger::logf(PREFIX_CC, "Loaded %s\n", bansTxt.c_str());
    } else {
        Logger::logf(PREFIX_WARNING, "File %s not found, creating default...\n", bansTxt.c_str());
    }

    // banned-ip.txt
    std::ifstream file3(banIpsTxt);
    if (file3.is_open()) {
        std::string line;
        while (std::getline(file3, line)) {
            line = trim(line);

            if (line.empty() || line[0] == '#') continue;

            bans.push_back(line);
        }
        file3.close();
        Logger::logf(PREFIX_CC, "Loaded %s\n", banIpsTxt.c_str());
    } else {
        Logger::logf(PREFIX_WARNING, "File %s not found, creating default...\n", banIpsTxt.c_str());
    }
}

void AdminSystem::saveConfigs() {
    std::lock_guard<std::mutex> lock(ASmutex);
    // admins.txt
    std::ofstream ofile(adminTxt);
    if (ofile.is_open()) {
        for (std::string admin : admins) {
            ofile << admin << '\n';
        }
        ofile.close();
        // Logger::logf(PREFIX_CC, "Saved %s\n", adminTxt.c_str());
    } else {
        Logger::logf(PREFIX_ERROR, "Failed to save %s\n", adminTxt.c_str());
    }

    // banned.txt
    std::ofstream ofile2(bansTxt);
    if (ofile2.is_open()) {
        for (std::string banned : bans) {
            ofile2 << banned << '\n';
        }
        ofile2.close();
        // Logger::logf(PREFIX_CC, "Saved %s\n", bansTxt.c_str());
    } else {
        Logger::logf(PREFIX_ERROR, "Failed to save %s\n", bansTxt.c_str());
    }

    // banned-ip.txt
    std::ofstream ofile3(banIpsTxt);
    if (ofile3.is_open()) {
        for (std::string bannedip : banIps) {
            ofile3 << bannedip << '\n';
        }
        ofile3.close();
        // Logger::logf(PREFIX_CC, "Saved %s\n", banIpsTxt.c_str());
    } else {
        Logger::logf(PREFIX_ERROR, "Failed to save %s\n", banIpsTxt.c_str());
    }
}

bool AdminSystem::kick(std::string& username, std::string reason) {
    if (reason.empty()) reason = "Kicked from server";
    int id = server->findClientByUsername(username);
    if (id == -1) return false;
    server->network->disconnectClient(id, reason, 1000);
    return true;
}

bool AdminSystem::ban(std::string& username) {
    if (username.empty()) return false;
    bool addToban = false;
    if (!this->isBanned(username)) addToban = true;
    if (!this->kick(username, "Banned from server")) return false;
    if (addToban) this->bans.push_back(username);
    this->saveConfigs();
    return true;
}

bool AdminSystem::op(std::string& username) {
    if (username.empty()) return false;
    if (!this->isAdmin(username)) {
        this->admins.push_back(username);
        this->saveConfigs();
        return true;
    }
    return false;
}

bool AdminSystem::deop(std::string& username) {
    if (username.empty()) return false;
    if (this->isAdmin(username)) {
        for (int i = 0; i<this->admins.size(); ++i) {
            if (this->admins[i] == username) {
                this->admins.erase(this->admins.begin() + i);
                this->saveConfigs();
                return true;
            }
        }
    }
    return false;
}

bool AdminSystem::banip(std::string& username) {
    if (username.empty()) return false;
    int id = server->findClientByUsername(username);
    if (id == -1) return false;
    std::string ip = server->getClientIp(id);
    if (ip.empty()) return false;
    bool addToban = false;
    if (!this->isIpBanned(std::string(username))) addToban = true;
    if (!this->kick(username, "Banned from server")) return false;
    if (addToban) this->banIps.push_back(username);
    this->saveConfigs();
    return true;
}

std::string AdminSystem::trim(std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool AdminSystem::isAdmin(std::string& username) {
    if (username.empty()) return false;
    for (std::string admin : this->admins) {
        if (username == admin) return true;
    }
    return false;
}

bool AdminSystem::isAdmin(int clientId) {
    std::string username = server->getClientUsername(clientId);
    for (std::string admin : this->admins) {
        if (username == admin) return true;
    }
    return false;
}

bool AdminSystem::isBanned(std::string& username) {
    if (username.empty()) return false;
    for (std::string ban : this->bans) {
        if (username == ban) return true;
    }
    return false;
}

bool AdminSystem::isBanned(int clientId) {
    std::string username = server->getClientUsername(clientId);
    for (std::string ban : this->bans) {
        if (username == ban) return true;
    }
    return false;
}

bool AdminSystem::isIpBanned(std::string& username) {
    if (username.empty()) return false;
    int id = server->findClientByUsername(username);
    std::string ip = server->getClientIp(id);
    for (std::string ban : this->banIps) {
        if (ip == ban) return true;
    }
    return false;
}

bool AdminSystem::isIpBanned(std::string ip) {
    if (ip.empty()) return false;
    for (std::string ban : this->banIps) {
        if (ip == ban) return true;
    }
    return false;
}

bool AdminSystem::isIpBanned(int clientId) {
    std::string ip = server->getClientIp(clientId);
    for (std::string ban : this->banIps) {
        if (ip == ban) return true;
    }
    return false;
}