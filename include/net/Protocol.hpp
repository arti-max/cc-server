#pragma once
#include <cstdint>

namespace Protocol {
    const int VERSION = 4; // Protocol v4

    enum class Opcode : uint8_t {
        // --- Client -> Server ---
        LOGIN                   = 0x00,
        POSITION_UPDATE         = 0x01,
        BLOCK_CHANGE            = 0x02,
        CHAT_MESSAGE_C2S        = 0x03,

        // --- Server -> Client ---
        LOGIN_RESPONSE          = 0x10,
        SERVER_IDENTIFICATION   = 0x11,
        LEVEL_DATA              = 0x12,
        BLOCK_UPDATE            = 0x17,
        CHAT_BROADCAST          = 0x18,
        SPAWN_PLAYER            = 0x20,
        PLAYER_POSITION         = 0x21,
        DESPAWN_PLAYER          = 0x22,
        SET_SPAWN_POSITION      = 0x23,
        REQUEST_SPAWN_POSITION  = 0x24,

        SERVER_CHAT_MESSAGE     = 0x30,
        CLIENT_CHAT_MESSAGE     = 0x31,

        // --- Other ---
        PING                    = 0xFE,
        DISCONNECT              = 0xFF
    };
}
