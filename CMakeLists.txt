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

# Папки заголовков
include_directories(include)

# Собираем исходники (Рекурсивно, чтобы захватить net/, level/ и т.д.)
file(GLOB_RECURSE SOURCES 
    "src/cpp/*.cpp"
    "src/cpp/*.c"
)

add_executable(CrossCraftServer ${SOURCES})

# --- Зависимости ---
target_link_libraries(CrossCraftServer CURL::libcurl)

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
        "${CMAKE_BINARY_DIR}/bin/README.TXT"
    COMMENT "Copying README.TXT to output directory"
)

if(WIN32)
    # start server.bat for Windows
    add_custom_command(TARGET CrossCraftServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "start CrossCraftServer.exe" >
            "${CMAKE_BINARY_DIR}/bin/start server.bat"
        COMMENT "Creating start server.bat"
    )
else()
    # start server.sh for Linux/macOS
    add_custom_command(TARGET CrossCraftServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "#!/bin/sh\n./CrossCraftServer" >
            "${CMAKE_BINARY_DIR}/bin/start server.sh"
        COMMAND chmod +x "${CMAKE_BINARY_DIR}/bin/start server.sh"
        COMMENT "Creating start server.sh"
    )
endif()