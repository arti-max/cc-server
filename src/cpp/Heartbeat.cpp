#include "Heartbeat.hpp"
#include "Server.hpp"
#include "net/Protocol.hpp"
#include <fstream>

Heartbeat::Heartbeat(Server* server) : server(server) {}
Heartbeat::~Heartbeat() {}

void Heartbeat::heartbeatThreadMain() {
    while (server->running.load()) {
        std::string serverName = server->config->getString("server-name");
        std::string max = std::to_string(server->config->getInt("max-players"));
        std::string users = std::to_string(server->getPlayersCount());
        std::string publicServer = server->config->getString("public");
        std::string port = std::to_string(server->config->getInt("port"));
        std::string salt = server->getSalt();

        std::string fields = format("name=%s&users=%s&max=%s&public=%s&port=%s&salt=%s&version=%i", serverName, users, max, publicServer, port, salt, Protocol::VERSION);
        std::string url = serverUrl + "?" + fields;
        CURLcode res;
        std::string response = HTTPRequest::PostResponse(serverUrl, fields, res);
        
        if (res == CURLE_OK) {
            if (response != lastUrl) {
                lastUrl = response;
                Logger::logf(PREFIX_CC, "To connect directly to this server, surf to: %s\n", response.c_str());
                std::ofstream extFile("externalurl.txt");
                if (extFile) {
                    extFile << response << std::endl;
                    extFile.close();
                } else {
                    Logger::log(PREFIX_ERROR, "Failed to write externalurl.txt\n");
                }
            }
            Logger::logf(PREFIX_CC, "Heartbeat sent successfully.\n");
        }

        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}
