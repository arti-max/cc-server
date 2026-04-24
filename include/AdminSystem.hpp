#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <sstream>

#include "Logger.hpp"
#include "util/TinyFormat.hpp"

class Server;

class AdminSystem {
private:
    std::vector<std::string> admins;
    std::vector<std::string> bans;
    std::vector<std::string> banIps;
    Server* server;

    std::mutex ASmutex;

    std::string adminTxt = "admins.txt";
    std::string bansTxt = "banned.txt";
    std::string banIpsTxt = "banned-ip.txt";

    void loadConfigs();
    void saveConfigs();
    std::string trim(std::string& str);

public:
    AdminSystem(Server* server);
    ~AdminSystem();

    bool kick(std::string& username, std::string reason);
    bool ban(std::string& username);
    bool banip(std::string& username);
    bool op(std::string& username);
    bool deop(std::string& username);
    bool unban(std::string& username);

    bool isAdmin(std::string& username);
    bool isAdmin(int clientId);

    bool isBanned(std::string& username);
    bool isBanned(int clientId);

    bool isIpBanned(std::string& username);
    bool isIpBanned(std::string ip);
    bool isIpBanned(int clientId);
};