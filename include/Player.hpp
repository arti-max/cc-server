// // Player.hpp
// class Player {
// public:
//     int id; // Внутренний ID (0-255)
//     std::string username;
//     float x, y, z;
//     float yaw, pitch;
//     bool admin;
    
//     // Буфер для входящих данных (TCP stream fragmentation)
//     std::vector<uint8_t> receiveBuffer; 

//     Player(int id, int socket) : id(id), admin(false) {}
// };

// // В Server.cpp при подключении:
// int getFreeId() {
//     for (int i = 0; i < 127; ++i) { // ОГРАНИЧИВАЕМ 127!
//         bool taken = false;
//         for (auto& p : players) {
//             if (p->id == i) { taken = true; break; }
//         }
//         if (!taken) return i;
//     }
//     return -1; // Сервер полон
// }
