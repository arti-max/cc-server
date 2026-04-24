#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "Protocol.hpp"

// Утилиты для свапа байт (Big Endian <-> Host)
inline uint16_t swap16(uint16_t v) { return (v << 8) | (v >> 8); }
inline uint32_t swap32(uint32_t v) { return (v << 24) | ((v << 8) & 0x00FF0000) | ((v >> 8) & 0x0000FF00) | (v >> 24); }

// Вспомогательные функции для Float <-> Int (битовое копирование)
inline uint32_t floatToBits(float f) { uint32_t i; std::memcpy(&i, &f, 4); return i; }
inline float bitsToFloat(uint32_t i) { float f; std::memcpy(&f, &i, 4); return f; }

class Packet {
protected:
    std::vector<uint8_t> data;
    size_t readPos = 0;
    Protocol::Opcode type;

public:
    // Конструктор для создания нового пакета (на отправку)
    explicit Packet(Protocol::Opcode type) : type(type) {
        // Первый байт - ID пакета
        data.push_back((uint8_t)type);
    }
    
    // Конструктор для чтения пакета (из сети)
    // rawData уже содержит ID первым байтом
    Packet(const std::vector<uint8_t>& rawData) : data(rawData), readPos(1), type((Protocol::Opcode)rawData[0]) {}

    Protocol::Opcode getType() const { return type; }
    const std::vector<uint8_t>& getData() const { return data; }

    // === WRITE (Big Endian) ===

    void writeByte(uint8_t val) {
        data.push_back(val);
    }

    void writeSByte(int8_t val) {
        data.push_back((uint8_t)val);
    }

    void writeBool(bool val) {
        data.push_back(val ? 1 : 0);
    }

    void writeShort(int16_t val) {
        uint16_t v = swap16((uint16_t)val);
        const uint8_t* p = (const uint8_t*)&v;
        data.push_back(p[0]); data.push_back(p[1]);
    }

    void writeInt(int32_t val) {
        uint32_t v = swap32((uint32_t)val);
        const uint8_t* p = (const uint8_t*)&v;
        data.push_back(p[0]); data.push_back(p[1]); data.push_back(p[2]); data.push_back(p[3]);
    }

    void writeFloat(float val) {
        // Сначала во float биты, потом swap32
        uint32_t v = swap32(floatToBits(val));
        const uint8_t* p = (const uint8_t*)&v;
        data.push_back(p[0]); data.push_back(p[1]); data.push_back(p[2]); data.push_back(p[3]);
    }

    void writeString(const std::string& str) {
        writeShort((int16_t)str.length());
        const uint8_t* ptr = (const uint8_t*)str.data();
        data.insert(data.end(), ptr, ptr + str.length());
    }

    void writeByteArray(const std::vector<uint8_t>& arr) {
        writeInt((int32_t)arr.size());
        data.insert(data.end(), arr.begin(), arr.end());
    }

    // === READ (Big Endian) ===

    uint8_t readByte() {
        if (readPos >= data.size()) return 0;
        return data[readPos++];
    }
    
    int8_t readSByte() { return (int8_t)readByte(); }

    bool readBool() { return readByte() != 0; }

    int16_t readShort() {
        if (readPos + 2 > data.size()) return 0;
        uint16_t v;
        std::memcpy(&v, &data[readPos], 2);
        readPos += 2;
        return (int16_t)swap16(v);
    }

    int32_t readInt() {
        if (readPos + 4 > data.size()) return 0;
        uint32_t v;
        std::memcpy(&v, &data[readPos], 4);
        readPos += 4;
        return (int32_t)swap32(v);
    }

    float readFloat() {
        if (readPos + 4 > data.size()) return 0.0f;
        uint32_t v;
        std::memcpy(&v, &data[readPos], 4);
        readPos += 4;
        v = swap32(v); // Свапаем обратно в Host Order
        return bitsToFloat(v);
    }

    std::string readString() {
        int16_t len = readShort();
        if (len <= 0 || readPos + len > data.size()) return "";
        std::string s((const char*)&data[readPos], len);
        readPos += len;
        return s;
    }

    // Читает массив байт (int len + bytes)
    std::vector<uint8_t> readByteArray() {
        int32_t len = readInt();
        if (len <= 0 || readPos + len > data.size()) return {};
        std::vector<uint8_t> arr;
        arr.reserve(len);
        arr.insert(arr.end(), data.begin() + readPos, data.begin() + readPos + len);
        readPos += len;
        return arr;
    }
};
