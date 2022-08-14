cmake_minimum_required(VERSION 3.0.0)
project(chlib)

set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-enet CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(${PROJECT_NAME}
    src/conversation.cpp
    src/enet.cpp
    src/host.cpp
    src/map.cpp
    src/player.cpp
    src/quest.cpp
    src/sdl.cpp
    src/server.cpp
    src/tileset.cpp
    src/world.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        unofficial::enet::enet
        nlohmann_json nlohmann_json::nlohmann_json
        SDL2::SDL2 SDL2::SDL2main
        spdlog::spdlog
)
