#include "CommandHandler.hpp"
#include "Server.hpp"
CommandHandler::CommandHandler(Server* server) : server(server) {}
CommandHandler::~CommandHandler() {}

bool CommandHandler::executeCommand(std::string& text, int clientId, bool console) {
    std::vector<std::string> parts = this->parseText(text);

    if ((parts[0] == "/say" || parts[0] == "/broadcast") && parts.size() >= 2 && (server->adminSystem->isAdmin(clientId) || console)) {
        returnText = "";
        for (int i=1; i<parts.size(); ++i) {
            returnText += parts[i] + " ";
        }
        this->privateText = false;
        Packet chat(Protocol::Opcode::SERVER_CHAT_MESSAGE);
        chat.writeString("&e" + returnText);
        server->sendPacketToAll(chat);
        returnText = "";
        return true;
    }

    if (parts[0] == "/setspawn" && parts.size() == 1 && (server->adminSystem->isAdmin(clientId) || console)) {
        if (!console) {
            // Logger::logf(PREFIX_DEBUG, "%i called /setspawn\n");
            auto [x, y, z] = server->getClientPos(clientId);
            auto [yaw, pitch] = server->getClientRot(clientId);
            // Logger::logf(PREFIX_DEBUG, "Before level setspawn\n");
            server->level->setSpawnPos((int)x, (int)y, (int)z, (int)yaw);
            server->updateSpawnPosition();
            // Logger::logf(PREFIX_DEBUG, "After level setspawn\n");
            returnText = format("New spawn position set to %i, %i, %i", (int)x, (int)y, (int)z);
            // Logger::logf(PREFIX_DEBUG, "ReturnText: %s\n", returnText);
            this->privateText = true;
            // Logger::logf(PREFIX_DEBUG, "End setspawn\n");
        } else {
            Logger::logf(PREFIX_CC, "Can't set spawn from console!\n");
        }
        return true;
    }

    if (parts[0] == "/op" && parts.size() == 2) {
        this->privateText = true;
        if (server->adminSystem->isAdmin(clientId) || console) {
            if (server->adminSystem->op(parts[1])) {
                returnText = format("%s now operator", parts[1]);
                server->sendMsgToPlayer(server->findClientByUsername(parts[1]), "You now operator");
            } else {
                returnText = format("&c%s already operator", parts[1]);
            }
        } else {
            returnText = "&cYou cannot use this command.";
        }
        return true;
    }

    if (parts[0] == "/deop" && parts.size() == 2) {
        this->privateText = true;
        if (server->adminSystem->isAdmin(clientId) || console) {
            if (server->adminSystem->deop(parts[1])) {
                returnText = format("%s deopped", parts[1]);
                server->sendMsgToPlayer(server->findClientByUsername(parts[1]), "You are no longer an operator");
            } else {
                returnText = format("&c%s is not an operator", parts[1]);
            }
        } else {
            returnText = "&cYou cannot use this command.";
        }
        return true;
    }

    if (parts[0] == "/kick" && (parts.size() == 2 || parts.size() == 3 )) {
        this->privateText = true;
        if (server->adminSystem->isAdmin(clientId) || console) {
            if (server->adminSystem->kick(parts[1], parts.size() == 3 ? parts[2] : "")) {
                returnText = "";
                // returnText = format("%s kicked", parts[1]);
                server->sendMsgToPlayer(clientId, format("&e%s kicked", parts[1]));
                if (parts.size() == 3) server->sendMsgToPlayer(clientId, format("&eReason: %s", parts[2]));
            } else {
                returnText = format("&c%s cannot been kicked", parts[1]);
            }
        } else {
            returnText = "&cYou cannot use this command.";
        }
        return true;
    }

    if (parts[0] == "/ban" && parts.size() == 2) {
        this->privateText = true;
        if (server->adminSystem->isAdmin(clientId) || console) {
            if (server->adminSystem->ban(parts[1])) {
                returnText = format("%s banned", parts[1]);
            } else {
                returnText = format("&c%s already banned or bad username", parts[1]);
            }
        } else {
            returnText = "&cYou cannot use this command.";
        }
        return true;
    }

    if (parts[0] == "/banip" && parts.size() == 2) {
        this->privateText = true;
        if (server->adminSystem->isAdmin(clientId) || console) {
            if (server->adminSystem->banip(parts[1])) {
                returnText = format("%s banned by ip", parts[1]);
            } else {
                returnText = format("&c%s already banned by ip or bad username", parts[1]);
            }
        } else {
            returnText = "&cYou cannot use this command.";
        }
        return true;
    }

    if (parts[0] == "/tp" || parts[0] == "/teleport" && parts.size() == 2) {
        if (!console) {
            this->privateText = true;
            int otherId = server->findClientByUsername(parts[1]);
            auto [px, py, pz] = server->getClientPos(clientId);
            auto [ox, oy, oz] = server->getClientPos(otherId);
            auto [pyaw, ppitch] = server->getClientRot(clientId);
            auto [oyaw, opitch] = server->getClientRot(otherId);

            returnText = format("Teleporting to %s", parts[1]);

            Packet move2(Protocol::Opcode::PLAYER_POSITION);
            move2.writeByte(clientId);
            move2.writeByte(1); // Type
            move2.writeFloat(ox);
            move2.writeFloat(oy);
            move2.writeFloat(oz);
            move2.writeFloat(oyaw);
            move2.writeFloat(opitch);

            server->network->sendPacket(clientId, move2);

            return true;
        } else {
            Logger::logf(PREFIX_CC, "Can't teleport from console!\n");
        }
    }

    if (parts[0] == "/solid" && parts.size() == 1) {
        this->privateText = true;
        if (server->adminSystem->isAdmin(clientId) || console) {
            server->toggleSolid();
            if (server->getSolid()) {
                returnText = "Now placing unbreakable stone";
            } else {
                returnText = "Now placing normal stone";
            }
        } else {
            returnText = "&cYou cannot use this command.";
        }
        return true;
    }

    return false;
}


std::vector<std::string> CommandHandler::parseText(std::string& text) {
    std::stringstream ss(text);
    std::string token;
    std::vector<std::string> tokens;

    while (ss >> token) {
        tokens.push_back(token);
    }

    return tokens; // return splitted text (array)
}

std::string CommandHandler::getReturnText() {
    return this->returnText;
}

bool CommandHandler::getTextVisible() {
    return this->privateText; // true - only for client, who execute command ; false - for everyone
}