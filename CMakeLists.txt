cmake_minimum_required(VERSION 3.15)
project(CrossCraftServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)

if (WIN32)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
else()
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    curl
    URL
    https://github.com/curl/curl/releases/download/curl-8_7_1/curl-8.7.1.tar.gz
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(curl)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# Папки заголовков
include_directories(include)

# Собираем исходники (рекурсивно, чтобы захватить net/, level/ и т.д.)
file(GLOB_RECURSE SOURCES 
    "src/cpp/*.cpp"
    "src/cpp/*.c"     # Для miniz.c, если решишь его добавить
)

add_executable(CrossCraftServer ${SOURCES})

# --- Зависимости ---
target_link_libraries(CrossCraftServer CURL::libcurl)
target_link_libraries(CrossCraftServer OpenSSL::SSL OpenSSL::Crypto)

if(WIN32)
    target_link_libraries(CrossCraftServer ws2_32)
else()
    target_link_libraries(CrossCraftServer pthread)
endif()

# Настройка вывода
set_target_properties(CrossCraftServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(TARGET CrossCraftServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/README.TXT"
        "${CMAKE_BINARY_DIR}/bin/Release/README.TXT"
    COMMENT "Copying README.TXT to output directory"
)

if(WIN32)
    file(WRITE "${CMAKE_BINARY_DIR}/bin/Release/start_server.bat" "start CrossCraftServer.exe\n")
else()
    file(WRITE "${CMAKE_BINARY_DIR}/bin/start_server.sh" "#!/bin/sh\n./CrossCraftServer\n")
    execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/bin/start_server.sh")
endif()