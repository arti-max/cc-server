#pragma once
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>
#include "Packet.hpp"
#include "Config.hpp"

// Кроссплатформенные сокеты
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET SocketType;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    typedef int SocketType;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

// Интерфейс для callback-ов сервера
class INetworkHandler {
public:
    virtual void onConnect(int clientId) = 0;
    virtual void onDisconnect(int clientId) = 0;
    virtual void onPacket(int clientId, Packet& packet) = 0;
};

enum class SessionState {
    WAITING_HANDSHAKE,
    CONNECTED
};

struct ClientSession {
    int id;
    SocketType socket;
    bool active;
    SessionState state;
    std::vector<uint8_t> recvBuffer; // Буфер для склейки TCP данных
    std::string ip;
};

class Network {
public:
    Network(int port, INetworkHandler* handler);
    ~Network();

    bool start();
    void stop();

    std::shared_ptr<Config> cfg;
    
    // Вызывать в каждом тике сервера (обрабатывает входящие данные)
    void poll(); 

    void sendPacket(int clientId, const Packet& packet);
    void disconnectClient(int clientId, const std::string& reason, uint16_t code=1000);

    void setConfig(std::shared_ptr<Config> cfg);
    std::string getClientIp(int clienId);

private:

    void initSocketSystem();
    void closeSocketSystem();
    void setNonBlocking(SocketType sock);
    
    void handleNewConnections();
    void handleClientData(ClientSession& session);

    void createCloseFrame(SocketType sock, const std::string& reason, uint16_t code);

private:
    int port;
    INetworkHandler* handler;
    SocketType listenSocket;
    std::atomic<bool> running;

    // Массив сессий. ID клиента = индекс в массиве (0..127)
    std::vector<std::unique_ptr<ClientSession>> sessions; 
    std::unordered_map<std::string, int> ipConnectionCount;
};
