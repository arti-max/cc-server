#include <iostream>
#include <string>
#include <csignal>
#include "Server.hpp"
#include "Config.hpp"
#include "Logger.hpp"

int main(int argc, char** argv) {
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    auto config = std::make_shared<Config>();
    config->load("server.properties");

    int port = config->getInt("port");
    Logger::logf(PREFIX_CC, "Starting server on port %d...\n", port);

    Server server(config);

    if (!server.start()) {
        Logger::log(PREFIX_ERROR, "Failed to start server\n");
        return 1;
    }

    std::cout << "Input 'stop' to stop...\n";
    while(server.running.load()) {
        // paass
    }

    // std::cout << "Press Enter to stop...\n";
    // std::cin.get();

    // server.stop();
    return 0;
}

