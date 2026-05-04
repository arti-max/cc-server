#include "Server.hpp"
#include "Logger.hpp"
#include "level/LevelGen.hpp"
#include "level/tile/Tile.hpp"
#include "level/LevelLoaderListener.hpp"
#include "net/Protocol.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <cstring>

#include "miniz.h"

// --- Helpers ---
static uint64_t nowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

class ConsoleLevelLoaderListener : public LevelLoaderListener {
public:
    void beginLevelLoading(const char* title) override {
        m_title = title ? title : "";
    }
    void levelLoadUpdate(const char* status) override {
        m_status = status ? status : "";
    }
    void levelLoadProgress(int progress) override {
        if (progress % 10 == 0 || progress == 100)
            Logger::logf(PREFIX_CC, "[LEVELGEN] %s : %s %d%%\n", m_title.c_str(), m_status.c_str(), progress);
    }
private:
    std::string m_title;
    std::string m_status;
};

// --- GZIP Helper ---
std::vector<uint8_t> gzipCompress(const std::vector<uint8_t>& data) {
    if (data.empty()) return {};

    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        Logger::log(PREFIX_ERROR, "Failed to init raw deflate");
        return {};
    }

    std::vector<uint8_t> out;
    out.reserve(data.size() + 128); // Запас

    // GZIP Header
    // ID1, ID2, CM, FLG, MTIME (4), XFL, OS
    uint8_t gzipHeader[] = { 
        0x1f, 0x8b, // Magic
        0x08,       // Compression Method (Deflate)
        0x00,       // Flags
        0x00, 0x00, 0x00, 0x00, // MTime
        0x00,       // XFL
        0x03        // OS (Unix/Linux/Unknown)
    };
    out.insert(out.end(), std::begin(gzipHeader), std::end(gzipHeader));

    // Compress
    zs.next_in = (Bytef*)data.data();
    zs.avail_in = (uInt)data.size();

    const size_t BUFSIZE = 32768;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[BUFSIZE]);
    int ret;

    do {
        zs.next_out = buffer.get();
        zs.avail_out = BUFSIZE;

        ret = deflate(&zs, Z_FINISH);

        if (out.size() + (BUFSIZE - zs.avail_out) > out.capacity()) {
             out.reserve(out.size() * 2);
        }
        out.insert(out.end(), buffer.get(), buffer.get() + BUFSIZE - zs.avail_out);
    } while (ret == Z_OK);

    deflateEnd(&zs);

    // GZIP Trailer (8 байт: CRC32 + ISIZE)
    uint32_t crc = mz_crc32(0, data.data(), data.size());
    uint32_t len = (uint32_t)data.size();

    // Little Endian
    out.push_back(crc & 0xFF); out.push_back((crc >> 8) & 0xFF);
    out.push_back((crc >> 16) & 0xFF); out.push_back((crc >> 24) & 0xFF);

    out.push_back(len & 0xFF); out.push_back((len >> 8) & 0xFF);
    out.push_back((len >> 16) & 0xFF); out.push_back((len >> 24) & 0xFF);

    return out;
}

// --- Server Implementation ---

Server::Server(std::shared_ptr<Config> cfg) : config(cfg) {
    timer = std::make_unique<Timer>(20.0f);
    
    level = std::make_unique<Level>();
    level->server = this;

    Random* random = new Random();

    salt = std::to_string(random->nextInt());

    delete random;
    players.resize(128); 
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (running.load()) return true;

    Logger::log(PREFIX_CC, "Initializing level...\n");
    initLevel();

    int port = config.get()->getInt("port");
    Logger::logf(PREFIX_CC, "Starting network on port %d...\n", port);
    
    network = std::make_unique<Network>(port, this);
    heartbeat = std::make_unique<Heartbeat>(this);
    commands = std::make_unique<CommandHandler>(this);
    adminSystem = std::make_unique<AdminSystem>(this);

    if (!network->start()) {
        Logger::log(PREFIX_ERROR, "Failed to start network!\n");
        return false;
    }
    network->setConfig(this->config);

    running.store(true);
    serverThread = std::thread(&Server::serverThreadMain, this);
    heartbeatThread = std::thread(&Heartbeat::heartbeatThreadMain, heartbeat);
    levelThread = std::thread(&Server::levelSaveThread, this);
    inputThread = std::thread(&Server::serverInputThread, this);

    return true;
}

void Server::stop() {
    if (!running.exchange(false)) return;

    Logger::log(PREFIX_CC, "Stopping server...\n");

    if (network) {
        network->stop();
    }

    if (serverThread.joinable()) {
        serverThread.join();
    }

    if (heartbeatThread.joinable()) {
        heartbeatThread.join();
    }

    if (levelThread.joinable()) {
        levelThread.join();
    }

    if (inputThread.joinable()) {
        inputThread.join();
    }
    
    saveLevel();
    Logger::log(PREFIX_CC, "Server stopped.\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

}

void Server::serverThreadMain() {
    Logger::log(PREFIX_CC, "Server loop running.\n");

    while (running.load()) {
        try {
            tickLoopOnce();

            long long remaining = timer->msUntilNextTick();
            if (remaining < 0) remaining = 0;

            if (network) network->wait(static_cast<int>(remaining));
            // std::this_thread::sleep_for(std::chrono::milliseconds(remaining));
        } catch (const std::exception& e) {
            Logger::logf(PREFIX_ERROR, "Exception in server loop: %s\n", e.what());
        } catch (...) {
            Logger::log(PREFIX_ERROR, "Unknown exception in server loop.\n");
        }
    }
}

void Server::serverInputThread() {
    Logger::log(PREFIX_CC, "Server input running.\n");

    std::string msg = "";

    while (running.load()) {
        std::getline(std::cin, msg);
        auto start = msg.find_first_not_of(" \t");
        auto end   = msg.find_last_not_of(" \t");
        if (start == std::string::npos || msg.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        msg = msg.substr(start, end - start + 1);
        if (msg == "stop") {
            this->saveLevel();
            this->stop();
            return;
        }
        try {
            bool fail = this->commands->executeCommand(msg, 0, true);
            if (!fail) {
                Logger::logf(PREFIX_ERROR, "Unknown command %s\n", msg);
            } else {
                if (commands->getReturnText() != "") {
                    Logger::logf(PREFIX_CC, "%s", this->commands->getReturnText().c_str());
                }
            }
        } catch (const std::exception& e) {
            Logger::logf(PREFIX_ERROR, "Command error: %s\n", e.what());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

void Server::levelSaveThread() {
    Logger::logf(PREFIX_CC, "Level save thread started.\n");

    while(running.load()) {
        try {
            this->saveLevel();
        } catch (const std::exception& e) {
            Logger::logf(PREFIX_ERROR, "Level save error: %s\n", e.what());
        } catch (...) {
            Logger::log(PREFIX_ERROR, "Unknown level save error.\n");
        }
        std::this_thread::sleep_for(std::chrono::seconds(120));
    }
}

void Server::tickLoopOnce() {
    if (level->hasWork()) {
        timer->advanceTime();
        for (int i = 0; i < timer->ticks; ++i) {
            level->tick();
        }
    }
}

// --- Level Management ---

void Server::initLevel() {
    namespace fs = std::filesystem;
    std::string levelFile = "level.dat";

    bool loaded = false;
    if (fs::exists(levelFile)) {
        Logger::logf(PREFIX_CC, "Loading level from %s\n", levelFile.c_str());
        if (level->load(levelFile.c_str())) {
            loaded = true;
        } else {
            Logger::log(PREFIX_WARNING, "Failed to load level, generating new one.\n");
        }
    }

    if (!loaded) {
        ConsoleLevelLoaderListener listener;
        LevelGen gen(&listener);
        int w = 256; 
        int h = 256; 
        int d = 64; 
        
        // Генерируем
        gen.generateLevel(level.get(), "server", w, h, d);
    }
    
    level->initTransient();
}

void Server::saveLevel() {
    Logger::log(PREFIX_CC, "Saving level...\n");
    level->save("level.dat");
}

void Server::sendBlockUpdate(int x, int y, int z, int type) {
    Packet update(Protocol::Opcode::BLOCK_UPDATE);
    update.writeInt(x);
    update.writeInt(y);
    update.writeInt(z);
    update.writeByte(type);

    for (auto& p : players) {
        if (p && p->loggedIn) {
            network->sendPacket(p->id, update);
        }
    }
}

// --- INetworkHandler Implementation ---

void Server::onConnect(int clientId) {
    Packet ident(Protocol::Opcode::SERVER_IDENTIFICATION);
    ident.writeByte(Protocol::VERSION);
    ident.writeString(config.get()->getString("server-name"));
    ident.writeString(config.get()->getString("motd"));
    
    network->sendPacket(clientId, ident);
    
    
    // Logger::logf(PREFIX_CC, "Sent Server ID to client %d", clientId);
}

void Server::onDisconnect(int clientId) {
    if (clientId < 0 || clientId >= players.size()) return;

    auto p = players[clientId];
    if (p && p->loggedIn) {
        Logger::logf(PREFIX_CC, "%s left the game.\n", p->username.c_str());

        players[clientId].reset();

        Packet pkt(Protocol::Opcode::DESPAWN_PLAYER);
        pkt.writeByte(clientId);

        for (auto& other : players) {
            if (other && other->loggedIn) {
                network->sendPacket(other->id, pkt);
            }
        }
        
        // Chat message left
        Packet chat(Protocol::Opcode::SERVER_CHAT_MESSAGE);
        chat.writeString("&e" + p->username + " left the game");
        for (auto& other : players) {
            if (other && other->loggedIn) network->sendPacket(other->id, chat);
        }
    } else {
        players[clientId].reset();
    }
}

void Server::onPacket(int clientId, Packet& packet) {
    Protocol::Opcode type = packet.getType();
    try {
    // Logger::logf(PREFIX_DEBUG, "RX Packet: 0x%02X\n", (uint8_t)type);

        switch (type) {
            case Protocol::Opcode::LOGIN:
                handleLogin(clientId, packet);
                break;
            case Protocol::Opcode::BLOCK_CHANGE:
                // Logger::log(PREFIX_CC, "Block Change Packet received!\n"); // LOG
                handleBlockChange(clientId, packet);
                break;
            case Protocol::Opcode::CLIENT_CHAT_MESSAGE:
                // Logger::log(PREFIX_CC, "Chat Packet received!\n"); // LOG
                handleChat(clientId, packet);
                break;
            case Protocol::Opcode::POSITION_UPDATE: 
            case Protocol::Opcode::PLAYER_POSITION:
                handlePosition(clientId, packet);
                break;
            case Protocol::Opcode::REQUEST_SPAWN_POSITION:
                handleRequestSpawn(clientId);
                break;
            case Protocol::Opcode::REQUEST_LEVEL_DATA:
                handleRequestLevel(clientId);
                break;
            default:
                Logger::logf(PREFIX_WARNING, "Unhandled packet 0x%02X\n", (uint8_t)type);
                break;
        }
    } catch (const std::exception& e) {
        Logger::logf(PREFIX_ERROR, "Exception handling packet 0x%02X from client %d: %s\n",
                     (uint8_t)type, clientId, e.what());
    } catch (...) {
        Logger::logf(PREFIX_ERROR, "Unknown exception handling packet 0x%02X from client %d\n",
                     (uint8_t)type, clientId);
    }
}

// --- Packet Handlers ---

void Server::handleLogin(int clientId, Packet& packet) {
    int8_t version = packet.readSByte();
    std::string username = packet.readString();
    std::string session = packet.readString();

    Logger::logf(PREFIX_CC, "Login request: %s (v%d)\n", username.c_str(), version);

    if (version != Protocol::VERSION) {
        network->disconnectClient(clientId, "Incompatible protocol version!", 1002);
        return;
    }

    if (this->adminSystem->isIpBanned(network->getClientIp(clientId))) {
        network->disconnectClient(clientId, "Banned!", 1008);
        return;
    }

    if (username.empty()) username = "Player" + std::to_string(clientId);

    if (config.get()->getBool("verify-names")) {
        if (username.empty() || session.empty()) {
            Logger::logf(PREFIX_CC, "Username or session is empty on the client with id %i\n", clientId);
            network->disconnectClient(clientId, "Authentication required.", 1002);
            return;
        }

        this->verifySession(username, session, clientId);
    } else {
        if (username.empty()) {
            username = "Player" + std::to_string(clientId);
            session = "";
        }

        if (!this->validateUsername(username)) {
            username = "Player" + std::to_string(clientId);
        }
    }

    auto newPlayer = std::make_shared<Player>(clientId, username);
    newPlayer->sessionId = session;
    newPlayer->loggedIn = false;
    newPlayer->x = (float)level->xSpawn;
    newPlayer->y = (float)level->ySpawn;
    newPlayer->z = (float)level->zSpawn;
    newPlayer->ip = network->getClientIp(clientId);
    
    players[clientId] = newPlayer;

    if (this->adminSystem->isBanned(username)) {
        network->disconnectClient(clientId, "Banned!", 1008);
        return;
    }

    // Login Response (0x10)
    {
        Packet resp(Protocol::Opcode::LOGIN_RESPONSE);
        resp.writeByte(clientId); 
        resp.writeString(username);
        network->sendPacket(clientId, resp);
    }

    // Spawn Existing Players for New Player (0x20)
    for (auto& other : players) {
        if (other && other->loggedIn && other->id != clientId) {
            Packet spawn(Protocol::Opcode::SPAWN_PLAYER);
            spawn.writeByte(other->id);
            spawn.writeString(other->username);
            spawn.writeFloat(other->x);
            spawn.writeFloat(other->y);
            spawn.writeFloat(other->z);
            spawn.writeFloat(other->yaw);
            spawn.writeFloat(other->pitch);
            network->sendPacket(clientId, spawn);
        }
    }

    // Spawn New Player for Others
    {
        Packet spawnMe(Protocol::Opcode::SPAWN_PLAYER);
        spawnMe.writeByte(newPlayer->id);
        spawnMe.writeString(newPlayer->username);
        spawnMe.writeFloat(newPlayer->x);
        spawnMe.writeFloat(newPlayer->y);
        spawnMe.writeFloat(newPlayer->z);
        spawnMe.writeFloat(newPlayer->yaw);
        spawnMe.writeFloat(newPlayer->pitch);
        
        for (auto& other : players) {
            if (other && other->loggedIn && other->id != clientId) {
                network->sendPacket(other->id, spawnMe);
            }
        }
    }
}

void Server::handleRequestLevel(int clientId) {
    // Level Data (0x12)
    Logger::log(PREFIX_CC, "Compressing level data...\n");
    std::vector<uint8_t> compressed = gzipCompress(level->getBlocks());
    
    Packet levelPkt(Protocol::Opcode::LEVEL_DATA);
    levelPkt.writeShort(level->width);
    levelPkt.writeShort(level->height);
    levelPkt.writeShort(level->depth);
    levelPkt.writeByteArray(compressed);
    network->sendPacket(clientId, levelPkt);
    Logger::logf(PREFIX_CC, "Sent level data (%d bytes compressed)\n", compressed.size());

    players[clientId]->loggedIn = true;
}

void Server::handleBlockChange(int clientId, Packet& packet) {
    
    int x = packet.readInt();
    int y = packet.readInt();
    int z = packet.readInt();
    uint8_t type = packet.readByte();
    uint8_t mode = packet.readByte();

    // Logger::logf(PREFIX_DEBUG, "Mode: %i, type: %i", mode, type);

    uint8_t finalType = (mode == 1) ? type : 0;

    if (this->adminSystem->isAdmin(clientId) && this->solid && finalType == Tile::rock->id) {
        finalType = Tile::unbreakable->id;
    }
    
    if (level->getTile(x, y, z) == Tile::unbreakable->id && !this->adminSystem->isAdmin(clientId)) {
        return;
    }

    level->setTile(x, y, z, finalType);
    // if ((finalType == Tile::sand->id || finalType == Tile::gravel->id) && level->getTile(x, y-1, z) == 0) {
    //     this->sendBlockUpdate(x, y, z, 0);
    // }
    
}

void Server::handlePosition(int clientId, Packet& packet) {
    auto p = players[clientId];
    if (!p) return;

    int8_t pid = packet.readSByte(); 
    int8_t type = packet.readByte();
    float x = packet.readFloat();
    float y = packet.readFloat();
    float z = packet.readFloat();
    float yaw = packet.readFloat();
    float pitch = packet.readFloat();

    p->x = x; p->y = y; p->z = z; 
    p->yaw = yaw; p->pitch = pitch;

    Packet move(Protocol::Opcode::PLAYER_POSITION);
    move.writeByte(clientId);
    move.writeByte(2); // Type
    move.writeFloat(x);
    move.writeFloat(y);
    move.writeFloat(z);
    move.writeFloat(yaw);
    move.writeFloat(pitch);

    for (auto& other : players) {
        if (other && other->loggedIn && other->id != clientId) {
            network->sendPacket(other->id, move);
        }
    }
}

void Server::handleChat(int clientId, Packet& packet) {
    auto p = players[clientId];
    if (!p) return;

    std::string msg = packet.readString();
    Logger::logf(PREFIX_CC, "[CHAT] <%s> %s\n", p->username.c_str(), msg.c_str());

    std::string formatted = p->username + ": " + msg;

    // 0x30 SERVER_CHAT_MESSAGE
    Packet chat(Protocol::Opcode::SERVER_CHAT_MESSAGE);
    bool privateT = false;
    bool exec = false;

    if (commands->executeCommand(msg, clientId)) {
        chat.writeString("&e" + commands->getReturnText());
        privateT = commands->getTextVisible();
        exec = true;
    } else {
        chat.writeString(formatted);
    }

    if (!(commands->getReturnText() == "" && exec) || !exec) {
        for (auto& other : players) {
            if (privateT) {
                if (other && other->id == clientId && other->loggedIn) {
                    network->sendPacket(other->id, chat);
                }
            } else {
                if (other && other->loggedIn) {
                    network->sendPacket(other->id, chat);
                }
            }
        }
    }
}

void Server::sendMsgToPlayer(int clientId, std::string msg) {
    Packet chat(Protocol::Opcode::SERVER_CHAT_MESSAGE);
    chat.writeString(msg);
    for (auto& other : players) { 
        if (other && other->id == clientId && other->loggedIn) {
            network->sendPacket(other->id, chat);
        }
    }
}

void Server::sendMsgToAll(std::string msg) {
    Packet chat(Protocol::Opcode::SERVER_CHAT_MESSAGE);
    chat.writeString(msg);
    for (auto& other : players) { 
        if (other && other->loggedIn) {
            network->sendPacket(other->id, chat);
        }
    }
}

void Server::handleRequestSpawn(int clientId) {
    // 0x24 request -> 0x23 response
    
    Packet resp(Protocol::Opcode::SET_SPAWN_POSITION);
    resp.writeInt(level->xSpawn);
    resp.writeShort(level->ySpawn); 
    resp.writeInt(level->zSpawn);
    resp.writeInt(level->rotSpawn);

    network->sendPacket(clientId, resp);

    // Chat Join Message
    {
        Packet chat(Protocol::Opcode::SERVER_CHAT_MESSAGE); 
        chat.writeString("&e" + this->getClientUsername(clientId) + " joined the game");
        for (auto& other : players) {
             if (other && other->loggedIn) network->sendPacket(other->id, chat);
        }
    }
}

// Login methods:

std::pair<bool, std::string> Server::validateSession(std::string& username, std::string& session, int clientId) {
    // Logger::logf(PREFIX_DEBUG, "Before session length check\n");
    if (session.size() < 16 || session.empty()) {
        return {false, "Invalid session"};
    }
    // Logger::logf(PREFIX_DEBUG, "Before find session check\n");
    for (auto& p : players) {
        if (p && p->sessionId == session && p->username != username) {
            Logger::logf(PREFIX_DEBUG, "Finded, lol\n");
            return {false, "Session ID is already in use by another player."};
        }
    }
    // Logger::logf(PREFIX_DEBUG, "Before return check\n");
    return {true, ""};
}


void Server::verifySession(std::string& username, std::string& session, int clientId) {
    // Logger::logf(PREFIX_DEBUG, "Before validate session\n");
    auto [isValid, reason] = this->validateSession(username, session, clientId);

    if (!isValid) {
        Logger::logf(PREFIX_CC, "Login failed: missing username or session, clietId: %i\n", clientId);
        this->network.get()->disconnectClient(clientId, reason, 1002);
        return;
    }
    
    std::string url = "https://crosscraftweb.ddns.net/checkserver.jsp?user=" + username + "&serverId=" + session;
    CURLcode res;
    std::string response = HTTPRequest::Get(url, res);
    // Logger::logf(PREFIX_DEBUG, "Data poluchaem pravilno %s\n", response);

    while(!response.empty() && (response.back() == '\n' || response.back() == '\r' || response.back() == ' ')) {
        response.pop_back();
    }

    if (response != "YES") {
        Logger::logf(PREFIX_CC, "Login failed: Invalid session for %s\n", username.c_str());
        this->network.get()->disconnectClient(clientId, "Invalid session.", 1002);
        return;
    }

}

bool Server::validateUsername(std::string& username) {
    if (username.empty() || username.size() > 16) return false;
    for (char c : username) {
        if (!std::isalnum(c) && c != '_') return false;
    }
    return true;
}

int Server::getPlayersCount() {
    int count = 0;
    for (auto& p : players) {
        if (p) {
            if (p->loggedIn = true && (!p->username.empty())) {
                count++;
            }
        }
    }

    return count;
}

void Server::updateSpawnPosition() {
    for (auto& p : players) {
        if (p && p->loggedIn) {
            this->handleRequestSpawn(p->id);
        }
    }
}

// CLIENT GETTERS/SETTERS

std::tuple<float, float, float> Server::getClientPos(int clientId) {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    for (auto& p : players) {
        if (p && p->id == clientId && p->loggedIn) {
            x = p->x;
            y = p->y;
            z = p->z;
            break;
        }
    }

    return {x, y, z};
}

std::tuple<float, float> Server::getClientRot(int clientId) {
    float yaw = 0.0f;
    float pitch = 0.0f;

    for (auto& p : players) {
        if (p && p->id == clientId && p->loggedIn) {
            yaw = p->yaw;
            pitch = p->pitch;
            break;
        }
    }

    return {yaw, pitch};
}

std::string Server::getClientUsername(int clientId) {
    std::string username = "";

    for (auto& p : players) {
        if (p && p->id == clientId && p->loggedIn) {
            username = p->username;
            break;
        }
    }

    return username;
}

std::string Server::getClientIp(int clientId) {
    std::string ip = "";

    for (auto& p : players) {
        if (p && p->id == clientId && p->loggedIn) {
            ip = p->ip;
            break;
        }
    }

    return ip;
}

int Server::findClientByUsername(std::string& username) {
    int id = -1;

    for (auto& p : players) {
        if (p && p->username == username && p->loggedIn) {
            id = p->id;
            break;
        }
    }

    return id;
}

void Server::sendPacketToAll(Packet p) {
    for (auto& other : players) {
        if (other && other->loggedIn && other->id ) {
            network->sendPacket(other->id, p);
        }
    }
}

std::string Server::getSalt() {
    return this->salt;
}

void Server::toggleSolid() {
    this->solid = !this->solid;
}

bool Server::getSolid() {
    return this->solid;
}