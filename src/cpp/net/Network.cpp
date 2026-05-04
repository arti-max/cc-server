#include "net/Network.hpp"
#include "net/WebSocketUtils.hpp"
#include "Logger.hpp"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#ifdef _WIN32
    #pragma comment(lib, "Ws2_32.lib")
#endif

void closeSocket(SocketType sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

Network::Network(int port, INetworkHandler* handler) 
    : port(port), handler(handler), listenSocket(INVALID_SOCKET), running(false) 
{
    // Резервируем слоты под 128 игроков
    sessions.resize(128); 
}

Network::~Network() {
    stop();
}

void Network::initSocketSystem() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

void Network::closeSocketSystem() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void Network::setNonBlocking(SocketType sock) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

bool Network::start() {
    initSocketSystem();

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        Logger::log(PREFIX_ERROR, "Failed to create socket\n");
        return false;
    }

    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        Logger::log(PREFIX_ERROR, "Bind failed. Port might be in use.\n");
        return false;
    }

    if (listen(listenSocket, 10) == SOCKET_ERROR) {
        Logger::log(PREFIX_ERROR, "Listen failed\n");
        return false;
    }

    setNonBlocking(listenSocket);
    running = true;
    Logger::logf(PREFIX_NETWORK, "WebSocket Server started on port %d\n", port);
    return true;
}

void Network::stop() {
    running = false;
    if (listenSocket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(listenSocket);
#else
        close(listenSocket);
#endif
    }
    
    // Закрываем всех клиентов
    for (auto& s : sessions) {
        if (s && s->active) {
#ifdef _WIN32
            closesocket(s->socket);
#else
            close(s->socket);
#endif
        }
    }
    
    closeSocketSystem();
}

void Network::wait(int timeoutMs) {
    if (!running) return;

    fd_set readfds;
    std::vector<ClientSession*> activeSessions;

    // Запоминаем время начала, чтобы корректно пересчитывать таймаут при перезапуске
    auto start = std::chrono::steady_clock::now();

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);
        SocketType maxfd = listenSocket;

        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            activeSessions.clear();
            for (auto& s : sessions) {
                if (s && s->active) {
                    activeSessions.push_back(s.get());
                    FD_SET(s->socket, &readfds);
                    if (s->socket > maxfd) maxfd = s->socket;
                }
            }
        }

        // Вычисляем оставшийся таймаут
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - start)
                           .count();
        long long remaining = timeoutMs - elapsed;
        if (remaining < 0) remaining = 0;

        struct timeval tv;
        tv.tv_sec = remaining / 1000;
        tv.tv_usec = (remaining % 1000) * 1000;

        int result = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);

        if (result < 0) {
            if (errno == EINTR) {
                // select прерван сигналом – просто пересчитываем таймаут и пробуем снова
                continue;
            }
            Logger::log(PREFIX_ERROR, "select() failed\n");
            return;
        }

        // Обработка событий
        if (result == 0) {
            // Таймаут – выходим
            return;
        }

        if (FD_ISSET(listenSocket, &readfds)) {
            handleNewConnections();
        }

        for (auto* session : activeSessions) {
            if (FD_ISSET(session->socket, &readfds)) {
                if (session->active) {
                    handleClientData(*session);
                }
            }
        }

        // События обработаны – выходим
        return;
    }
}

void Network::poll() {
    if (!running) return;

    // 1. Принимаем новых
    handleNewConnections();

    // 2. Читаем данные от существующих
    for (auto& session : sessions) {
        if (session && session->active) {
            handleClientData(*session);
        }
    }
}

void Network::setConfig(std::shared_ptr<Config> cfig) {
    this->cfg = std::move(cfig);
}

void Network::handleNewConnections() {
    sockaddr_in clientAddr;
#ifdef _WIN32
    int len = sizeof(clientAddr);
#else
    socklen_t len = sizeof(clientAddr);
#endif

    SocketType clientSock = accept(listenSocket, (sockaddr*)&clientAddr, &len);
    if (clientSock != INVALID_SOCKET) {
        setNonBlocking(clientSock);

        std::string ip = inet_ntoa(clientAddr.sin_addr);

        int maxConnPerIp = cfg ? cfg.get()->getInt("max-connections") : 3;
        if (this->ipConnectionCount[ip] >= maxConnPerIp) {
            const char* msg = "Too many connections from this IP";
            send(clientSock, msg, (int)strlen(msg), 0);
            createCloseFrame(clientSock, msg, 1008);
#ifdef _WIN32
            closesocket(clientSock);
#else
            close(clientSock);
#endif
            return;
        }

        int maxPlayers = cfg ? cfg.get()->getInt("max-players") : 16;
        int currOnline = 0;
        for (auto& s : sessions) {
            if (s && s->active) currOnline++;
        }
        if (currOnline > maxPlayers) {
            const char* msg = "Server Full";
            send(clientSock, msg, (int)strlen(msg), 0);
            createCloseFrame(clientSock, msg, 1013);
#ifdef _WIN32
            closesocket(clientSock);
#else
            close(clientSock);
#endif
            return;
        }

        // Ищем свободный ID (0..127)
        int newId = -1;
        for (int i = 0; i < 128; ++i) {
            if (!sessions[i] || !sessions[i]->active) {
                newId = i;
                break;
            }
        }


        if (newId != -1) {
            // Создаем сессию
            sessions[newId] = std::make_unique<ClientSession>();
            sessions[newId]->id = newId;
            sessions[newId]->socket = clientSock;
            sessions[newId]->active = true;
            sessions[newId]->state = SessionState::WAITING_HANDSHAKE;
            sessions[newId]->ip = ip;

            ipConnectionCount[ip]++;
            
            // Logger::logf(PREFIX_NETWORK, "New connection ID %d, waiting for handshake...", newId);
        } else {
            // Сервер полон
            const char* msg = "Server Full";
            send(clientSock, msg, (int)strlen(msg), 0);
            createCloseFrame(clientSock, msg, 1013);
#ifdef _WIN32
            closesocket(clientSock);
#else
            close(clientSock);
#endif
        }
    }
}

void Network::handleClientData(ClientSession& session) {
    uint8_t tempBuffer[4096];
    int bytesRead = recv(session.socket, (char*)tempBuffer, sizeof(tempBuffer), 0);

    if (bytesRead > 0) {
        session.recvBuffer.insert(session.recvBuffer.end(), tempBuffer, tempBuffer + bytesRead);

        if (session.state == SessionState::WAITING_HANDSHAKE) {
            // Ищем конец HTTP заголовков \r\n\r\n
            std::string dataStr(session.recvBuffer.begin(), session.recvBuffer.end());
            size_t headerEnd = dataStr.find("\r\n\r\n");
            
            if (headerEnd != std::string::npos) {
                // Парсим Sec-WebSocket-Key
                std::string keyHeader = "Sec-WebSocket-Key: ";
                size_t keyStart = dataStr.find(keyHeader);
                if (keyStart != std::string::npos) {
                    keyStart += keyHeader.length();
                    size_t keyEnd = dataStr.find("\r\n", keyStart);
                    std::string clientKey = dataStr.substr(keyStart, keyEnd - keyStart);

                    // Формируем ответ
                    std::string acceptKey = WebSocketUtils::generateAcceptKey(clientKey);
                    std::string response = 
                        "HTTP/1.1 101 Switching Protocols\r\n"
                        "Upgrade: websocket\r\n"
                        "Connection: Upgrade\r\n"
                        "Sec-WebSocket-Accept: " + acceptKey + "\r\n\r\n";

                    send(session.socket, response.c_str(), (int)response.length(), 0);
                    
                    session.state = SessionState::CONNECTED;
                    session.recvBuffer.erase(session.recvBuffer.begin(), session.recvBuffer.begin() + headerEnd + 4);
                    
                    // Logger::logf(PREFIX_NETWORK, "Handshake done for ID %d", session.id);
                    if (handler) handler->onConnect(session.id);
                }
            }
        } 
        else if (session.state == SessionState::CONNECTED) {
            // Парсинг WebSocket фреймов
            while (session.recvBuffer.size() >= 2) { // Мин. заголовок 2 байта
                uint8_t b1 = session.recvBuffer[0];
                uint8_t b2 = session.recvBuffer[1];

                uint8_t opcode = b1 & 0x0F;
                bool masked = (b2 & 0x80) != 0;
                uint64_t payloadLen = b2 & 0x7F;

                size_t headerSize = 2;
                if (payloadLen == 126) headerSize += 2;
                else if (payloadLen == 127) headerSize += 8;
                
                if (masked) headerSize += 4;

                // Ждем пока придет весь заголовок
                if (session.recvBuffer.size() < headerSize) break;

                // Читаем длину Payload
                size_t offset = 2;
                if (payloadLen == 126) {
                    payloadLen = (session.recvBuffer[2] << 8) | session.recvBuffer[3];
                    offset += 2;
                } else if (payloadLen == 127) {
                    // Упрощенно берем 64-бит длину (старшие байты игнорируем, если пакет < 4Гб)
                    // TODO: Swap64 if needed
                    offset += 8;
                }

                // Ждем пока придет весь Payload
                if (session.recvBuffer.size() < headerSize + payloadLen) break;

                // Читаем маску
                uint8_t maskingKey[4] = {0,0,0,0};
                if (masked) {
                    maskingKey[0] = session.recvBuffer[offset];
                    maskingKey[1] = session.recvBuffer[offset+1];
                    maskingKey[2] = session.recvBuffer[offset+2];
                    maskingKey[3] = session.recvBuffer[offset+3];
                    offset += 4;
                }

                // Декодируем данные (XOR)
                std::vector<uint8_t> payload;
                payload.resize(payloadLen);
                for (size_t i = 0; i < payloadLen; ++i) {
                    uint8_t byte = session.recvBuffer[headerSize + i];
                    payload[i] = masked ? (byte ^ maskingKey[i % 4]) : byte;
                }

                // Удаляем фрейм из буфера
                session.recvBuffer.erase(session.recvBuffer.begin(), session.recvBuffer.begin() + headerSize + payloadLen);

                // Обрабатываем Opcode
                if (opcode == 0x8) { // Close frame
                    disconnectClient(session.id, "Client closed connection");
                    return;
                }
                else if (opcode == 0x2) { // Binary frame
                    // Передаем пакет в обработчик
                    if (!payload.empty()) {
                        // Создаем пакет из данных (первый байт - ID)
                        Packet packet(payload);
                        if (handler) handler->onPacket(session.id, packet);
                    }
                }
            }
        }
    } else if (bytesRead == 0) {
        disconnectClient(session.id, "Connection Closed", 1000);
    } else {
        // Ошибка (проверяем EWOULDBLOCK)
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) disconnectClient(session.id, "Socket Error");
#else
        if (errno != EWOULDBLOCK && errno != EAGAIN) disconnectClient(session.id, "Socket Error");
#endif
    }
}

void Network::sendPacket(int clientId, const Packet& packet) {
    std::lock_guard<std::mutex> lock(sessionsMutex);   // защищаем доступ к sessions

    if (clientId < 0 || clientId >= sessions.size() || !sessions[clientId] || !sessions[clientId]->active || sessions[clientId]->state != SessionState::CONNECTED) {
        return;
    }

    const std::vector<uint8_t>& rawData = packet.getData();
    size_t payloadLen = rawData.size();

    std::vector<uint8_t> frame;
    frame.reserve(payloadLen + 10);
    frame.push_back(0x82); // FIN + Binary

    if (payloadLen <= 125) {
        frame.push_back((uint8_t)payloadLen);
    } else if (payloadLen <= 65535) {
        frame.push_back(126);
        frame.push_back((payloadLen >> 8) & 0xFF);
        frame.push_back(payloadLen & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((payloadLen >> (i * 8)) & 0xFF);
        }
    }
    frame.insert(frame.end(), rawData.begin(), rawData.end());

    // Гарантированно отправляем все байты
    bool ok = sendAllLocked(sessions[clientId]->socket, frame.data(), frame.size());
    if (!ok) {
        SocketType sock = sessions[clientId]->socket;
        closeSocket(sock);
        sessions[clientId]->active = false;
    }
}

void Network::disconnectClient(int clientId, const std::string& reason, uint16_t code) {
    sessionsMutex.lock();
    if (clientId < 0 || clientId >= 128 || !sessions[clientId] || !sessions[clientId]->active) {
        sessionsMutex.unlock();
        return;
    }

    bool shouldNotify = (sessions[clientId]->state == SessionState::CONNECTED);
    SocketType sock = sessions[clientId]->socket;

    if (shouldNotify) {
        createCloseFrame(sock, reason, code);
    }

    std::string ip = sessions[clientId]->ip;
    INetworkHandler* notifyHandler = handler;

    ipConnectionCount[ip]--;
    if (ipConnectionCount[ip] <= 0) {
        ipConnectionCount.erase(ip);
    }
    sessions[clientId]->active = false;
    sessions[clientId]->recvBuffer.clear();

    sessionsMutex.unlock();

    closeSocket(sock);
    if (notifyHandler) {
        notifyHandler->onDisconnect(clientId);
    }
}

void Network::createCloseFrame(SocketType sock, const std::string& reason, uint16_t code) {
    std::vector<uint8_t> frame;
        frame.push_back(0x88); // FIN + 0x8;

        size_t payloadLen = 2 + reason.size();

        if (payloadLen <= 125) {
            frame.push_back((uint8_t)payloadLen);
        } else if (payloadLen <= 65535) {
            frame.push_back(126);
            frame.push_back((payloadLen >> 8) & 0xFF);
            frame.push_back(payloadLen & 0xFF);
        } else {
            payloadLen = 2;
            frame.push_back(2);
        }
        frame.push_back((code >> 8) & 0xFF);
        frame.push_back(code & 0xFF);

        if (payloadLen > 2) {
            frame.insert(frame.end(), reason.begin(), reason.end());
        }

        send(sock, (const char*)frame.data(), (int)frame.size(), 0);
}

std::string Network::getClientIp(int clientId) {
    for (auto& s : sessions) {
        if (s && s->active && s->id == clientId) {
            return s->ip;
        }
    }
}

bool Network::sendAllLocked(SocketType sock, const void* data, size_t len) {
    const uint8_t* ptr = (const uint8_t*)data;
    size_t totalSent = 0;
    while (totalSent < len) {
        int chunk = send(sock, (const char*)(ptr + totalSent), (int)(len - totalSent), 0);
        if (chunk < 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                Sleep(1);          // подождать 1 мс и попробовать снова
                continue;
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);      // 1 мс
                continue;
            }
#endif
            return false;         // критическая ошибка
        }
        if (chunk == 0) {
            return false;         // соединение закрыто
        }
        totalSent += chunk;
    }
    return true;
}