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
// Смысла в этом не было, плюс код был написан нейронкой. Под конец уже перестал писат ьсервер с помощью вайбкода, но какие-то старые части кода всё равно остались ею написаны или закомментированны
