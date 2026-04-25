#pragma once
#include <iostream>
#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <map>

#include "util/HTTPRequest.hpp"
#include "util/TinyFormat.hpp"

class Server;

class Heartbeat {
private:
    std::string serverUrl = "https://crosscraftweb.ddns.net/heartbeat.jsp";
    Server* server;
    std::string lastUrl;

public:
    Heartbeat(Server* server);
    ~Heartbeat();
    void heartbeatThreadMain();

};