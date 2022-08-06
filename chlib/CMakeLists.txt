cmake_minimum_required(VERSION 3.0.0)
project(chlib)

set(CMAKE_CXX_STANDARD 20)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2-image CONFIG REQUIRED)
find_package(SDL2-mixer CONFIG REQUIRED)
find_package(SDL2-ttf CONFIG REQUIRED)

add_library(${PROJECT_NAME}
    src/client_list.cpp
    src/conversation.cpp
    src/map.cpp
    src/player.cpp
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
        nlohmann_json nlohmann_json::nlohmann_json
        SDL2::SDL2 SDL2::SDL2main
        SDL2::SDL2_image
        SDL2::SDL2_mixer
        SDL2::SDL2_ttf
)
