#pragma once
#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <map>

#include "Timer.hpp"
#include "Config.hpp"
#include "level/Level.hpp"
#include "net/Network.hpp"
#include "net/Packet.hpp"
#include "Heartbeat.hpp"
#include "util/HTTPRequest.hpp"
#include "CommandHandler.hpp"
#include "AdminSystem.hpp"
#include "util/Random.hpp"

struct Player {
    int id;
    std::string username;
    std::string sessionId;
    float x, y, z;
    float yaw, pitch;
    bool loggedIn = false;
    std::string ip;

    Player(int id, std::string name) : id(id), username(name), x(0), y(0), z(0), yaw(0), pitch(0) {}
};


class Server : public INetworkHandler {
public:
    explicit Server(std::shared_ptr<Config> cfg);
    ~Server();

    std::atomic<bool> running{false};
    std::shared_ptr<Heartbeat> heartbeat;
    std::shared_ptr<CommandHandler> commands;
    std::shared_ptr<AdminSystem> adminSystem;

    std::shared_ptr<Config> config;

    std::unique_ptr<Level> level;
    std::unique_ptr<Timer> timer;
    std::unique_ptr<Network> network;

    bool start();
    void stop();

    void onConnect(int clientId) override;
    void onDisconnect(int clientId) override;
    void onPacket(int clientId, Packet& packet) override;

    void sendBlockUpdate(int x, int y, int z, int type);
    void updateSpawnPosition();
    int getPlayersCount();
    std::tuple<float, float, float> getClientPos(int clientId);
    std::tuple<float, float> getClientRot(int clientId);
    std::string getClientUsername(int clientId);
    std::string getClientIp(int clientId);
    int findClientByUsername(std::string& uname);
    
    void sendMsgToPlayer(int clientId, std::string msg);
    void sendPacketToAll(Packet p);
    void sendMsgToAll(std::string msg);
    std::string getSalt();
    void toggleSolid();
    bool getSolid();

private:
    void serverThreadMain();
    void levelSaveThread();
    void serverInputThread();
    void tickLoopOnce();
    void initLevel();
    void saveLevel();

    // Packets
    void handleLogin(int clientId, Packet& packet);
    void handleBlockChange(int clientId, Packet& packet);
    void handlePosition(int clientId, Packet& packet);
    void handleChat(int clientId, Packet& packet);
    void handleRequestSpawn(int clientId);


    //Login
    void verifySession(std::string& username, std::string& session, int clientId);
    std::pair<bool, std::string> validateSession(std::string& username, std::string& session, int clientId);
    bool validateUsername(std::string& username);

    std::vector<uint8_t> compressLevelData(); // GZIP

private:
    std::thread serverThread;
    std::thread heartbeatThread;
    std::thread levelThread;
    std::thread inputThread;
    bool solid = false;

    std::string salt = "";

    std::vector<std::shared_ptr<Player>> players;
};
