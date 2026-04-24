#include "Heartbeat.hpp"
#include "Server.hpp"


Heartbeat::Heartbeat(Server* server) : server(server) {}
Heartbeat::~Heartbeat() {}

void Heartbeat::heartbeatThreadMain() {
    while (server->running.load()) {
        std::string serverName = server->config->getString("server-name");
        std::string max = std::to_string(server->config->getInt("max-players"));
        std::string users = std::to_string(server->getPlayersCount());
        std::string publicServer = server->config->getString("public");
        std::string port = std::to_string(server->config->getInt("port"));

        std::string fields = format("name=%s&users=%s&max=%s&public=%s&port=%s", serverName, users, max, publicServer, port);
        std::string url = serverUrl + "?" + fields;
        CURLcode res;
        HTTPRequest::Post(serverUrl, fields, res);
        
        if (res == CURLE_OK) {
            Logger::logf(PREFIX_CC, "Heartbeat sent successfully.\n");
        }

        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}
