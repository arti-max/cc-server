#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <cstdio>

#define PREFIX_CC 1
#define PREFIX_DEBUG 2
#define PREFIX_ERROR 3
#define PREFIX_NETWORK 4
#define PREFIX_WARNING 5

#define TRUE 1
#define FALSE 0

class Logger {
private:
    static constexpr size_t BUFFER_SIZE = 2048;
    static int DEBUG;
    
    static const char* getPrefix(int id) {
        switch (id) {
            case PREFIX_CC:
                return "[CC]";
            case PREFIX_DEBUG:
                return "[DEBUG]";
            case PREFIX_ERROR:
                return "[ERROR]";
            case PREFIX_NETWORK:
                return "[NET]";
            case PREFIX_WARNING:
                return "[WARNING]";
            default:
                return "[NULL]";
        }
    }
    
    template<typename T>
    static void buildMessage(std::ostringstream& oss, T&& arg) {
        oss << std::forward<T>(arg);
    }
    
    template<typename T, typename... Args>
    static void buildMessage(std::ostringstream& oss, T&& first, Args&&... rest) {
        oss << std::forward<T>(first);
        buildMessage(oss, std::forward<Args>(rest)...);
    }
    
public:
    Logger() = default;

    static void log(int id, const char* str) {
        std::cout << getPrefix(id) << " " << str << std::endl;
    }

    static void enableDebug() {
        DEBUG = TRUE;
    }

    static void disableDebug() {
        DEBUG = FALSE;
    }
    
    static void logf(int id, const char* format, ...) {
        char buffer[BUFFER_SIZE];
        
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        std::cout << getPrefix(id) << " " << buffer;
    }
    
    template<typename... Args>
    static void log(int id, Args&&... args) {
        std::ostringstream oss;
        buildMessage(oss, std::forward<Args>(args)...);
        std::cout << getPrefix(id) << " " << oss.str() << std::endl;
    }
};
