#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>

#include "Logger.hpp"
#include "util/TinyFormat.hpp"

class Server;

class CommandHandler {
private:
    std::vector<std::string> parseText(std::string& text);

    Server* server = nullptr;
    std::string returnText = "";
    bool privateText = true;

public:
    CommandHandler(Server* server);
    ~CommandHandler();

    bool executeCommand(std::string& text, int clientId, bool console = false);
    std::string getReturnText();
    bool getTextVisible();
};